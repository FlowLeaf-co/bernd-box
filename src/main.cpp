/**
 * \author Moritz Ulmer <moritz.ulmer@posteo.de>
 * \date 2018
 * \copyright Apache License 2.0
 */

#define _TASK_STATUS_REQUEST
#define _TASK_OO_CALLBACKS

#include <TaskScheduler.h>
#include <Timer.h>

// io.h has to be included before mqtt.h
#include "io.h"

#include "configuration.h"
#include "mqtt.h"
#include "network.h"

#include "tasks/connectivity.h"
#include "tasks/dallas_temperature.h"
#include "tasks/pump.h"

//----------------------------------------------------------------------------
// Global instances
WiFiClient wifiClient;
Timer timer;
Scheduler scheduler;

bernd_box::Io io;
bernd_box::Mqtt mqtt(wifiClient, bernd_box::client_id, bernd_box::mqtt_server);
bernd_box::Network network(bernd_box::ssid, bernd_box::password);

//----------------------------------------------------------------------------
// TaskScheduler tasks

bernd_box::tasks::Pump pumpTask(&scheduler, io, mqtt);
bernd_box::tasks::CheckConnectivity checkConnectivity(
    &scheduler, network, mqtt, io, bernd_box::wifi_connect_timeout,
    bernd_box::mqtt_connection_attempts);
bernd_box::tasks::DallasTemperature dallasTemperatureTask(&scheduler, io, mqtt);

//----------------------------------------------------------------------------
// List of available tasks
int8_t readAnalogSensorsId;
void readAnalogSensors();

int8_t readSelectAnalogSensorsId;
void readSelectAnalogSensors();

namespace update_acidity_sensor {
int8_t id;
void callback();
bool is_first_run = true;
}  // namespace update_acidity_sensor

int8_t readAirSensorsId;
void readAirSensors();

int8_t readLightSensorsId;
void readLightSensors();

namespace measurement_report {
int8_t id;
void callback();
}  // namespace measurement_report

//----------------------------------------------------------------------------
// Setup and loop functions

void setup() {
  Serial.begin(115200);

  checkConnectivity.now();
  checkConnectivity.setInterval(std::chrono::milliseconds(100).count());
  checkConnectivity.enable();

  // Try to configure the IO devices, else restart
  if (io.init() != bernd_box::Result::kSuccess) {
    Serial.println("IO: Initialization failed. Restarting\n");
    ESP.restart();
  }

  pumpTask.setDuration(std::chrono::seconds(20));
  pumpTask.enable();

  // readAnalogSensorsId = timer.every(1000, readAnalogSensors);
  // readSelectAnalogSensorsId = timer.every(1000, readSelectAnalogSensors);
  // readAirSensorsId = timer.every(10000, readAirSensors);
  // readLightSensorsId = timer.every(10000, readLightSensors);
  // updateAciditySensorId = timer.every(30, updateAciditySensor);
  // update_dallas_temperature_sample::id =
  //     timer.every(1000, update_dallas_temperature_sample::callback);
  measurement_report::id = timer.every(1000, measurement_report::callback);
}

// Update both schedulers
void loop() {
  timer.update();
  scheduler.execute();
}

//----------------------------------------------------------------------------
// Implementations of available tasks

// Read and then print the analog sensors
void readAnalogSensors() {
  io.setStatusLed(true);

  // Read all sensors. Then print and send them over MQTT
  Serial.printf("\n%-10s|%-4s|%-15s|%s\n", "Sensor", "Pin", "Value", "Unit");
  Serial.printf("----------|----|---------------|----\n");
  const auto& sensor = io.adcs_.find(bernd_box::Sensor::kTotalDissolvedSolids);
  float value = io.readAnalog(sensor->first);
  Serial.printf("%-10s|%-4i|%-15f|%s\n", sensor->second.name.c_str(),
                sensor->second.pin_id, value, sensor->second.unit.c_str());
  mqtt.send(sensor->second.name.c_str(), value);

  // Measure all
  for (auto& it : io.adcs_) {
    float value = io.readAnalog(it.first);

    Serial.printf("%-10s|%-4i|%-15f|%s\n", it.second.name.c_str(),
                  it.second.pin_id, value, it.second.unit.c_str());
    mqtt.send(it.second.name.c_str(), value);
  }

  io.setStatusLed(false);
}

// Read and then print selected analog sensors
void readSelectAnalogSensors() {
  io.setStatusLed(true);

  // Read all sensors. Then print and send them over MQTT
  Serial.printf("\n%-10s|%-4s|%-15s|%s\n", "Sensor", "Pin", "Value", "Unit");
  Serial.printf("----------|----|---------------|----\n");
  const auto& sensor = io.adcs_.find(bernd_box::Sensor::kTotalDissolvedSolids);
  float value = io.readAnalog(sensor->first);
  Serial.printf("%-10s|%-4i|%-15f|%s\n", sensor->second.name.c_str(),
                sensor->second.pin_id, value, sensor->second.unit.c_str());
  mqtt.send(sensor->second.name.c_str(), value);

  io.setStatusLed(false);
}

// Take multiple acidity readings and average them. Task stops after enough
// measurements have been collected.
namespace update_acidity_sensor {
void callback() {
  io.setStatusLed(true);

  if (is_first_run) {
    io.enableAnalog(bernd_box::Sensor::kAciditiy);
    io.clearAcidityMeasurements();
    is_first_run = false;
  }

  io.takeAcidityMeasurement();

  // Once enough measurements have been taken, stop the task and send
  if (io.isAcidityMeasurementFull()) {
    io.disableAnalog(bernd_box::Sensor::kAciditiy);
    is_first_run = true;

    timer.stop(id);
  }

  io.setStatusLed(false);
}
}  // namespace update_acidity_sensor

// Reads, prints and then sends all air sensor parameters
void readAirSensors() {
  io.setStatusLed(true);

  for (const auto& bme : io.bme280s_) {
    float value = io.readBme280Air(bme.first);
    Serial.printf("The %s is %f %s\n", bme.second.name.c_str(), value,
                  bme.second.unit.c_str());
    mqtt.send(bme.second.name.c_str(), value);
  }

  io.setStatusLed(false);
}

// Reads, prints and then sends all light sensors
void readLightSensors() {
  io.setStatusLed(true);

  for (const auto& max44009 : io.max44009s_) {
    float value = io.readMax44009Light(max44009.first);
    Serial.printf("Ambient brightness (ID: %u) is %f %s\n",
                  static_cast<int>(max44009.first), value,
                  max44009.second.unit.c_str());
    mqtt.send(max44009.second.name.c_str(), value);
  }

  io.setStatusLed(false);
}

namespace measurement_report {

bool is_state_finished = true;
bool is_report_finished = true;
std::chrono::milliseconds start_time;

std::chrono::milliseconds pump_start_time;
std::chrono::seconds pump_duration(20);

std::chrono::milliseconds tds_start_time;
std::chrono::seconds tds_duration(20);

const float acidity_factor_v_to_ph = 3.5;
float acidity_offset = 0.4231628418;

std::chrono::milliseconds temperature_request_time;

std::chrono::milliseconds turbidity_start_time;
std::chrono::seconds turbidity_duration(10);

bernd_box::Sensor report_list[] = {
    bernd_box::Sensor::kUnknown, bernd_box::Sensor::kPump,
    bernd_box::Sensor::kAciditiy, bernd_box::Sensor::kUnknown};

int report_index = 0;

void callback() {
  Serial.printf("Measurement report. sensor_task: %i\n",
                int(report_list[report_index]));

  switch (report_list[report_index]) {
    case bernd_box::Sensor::kUnknown: {
      // Initial state. Select first state to go to
      if (is_report_finished) {
        Serial.println("Starting measurement report");
        start_time = std::chrono::milliseconds(millis());
        is_state_finished = true;
        is_report_finished = false;
        report_index++;
      } else {
        // TODO: End report
        timer.stop(id);
        Serial.printf("Measurement report finished after %llus\n",
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::milliseconds(millis()) - start_time)
                          .count());
        report_index = 0;
      }
    } break;
    case bernd_box::Sensor::kPump: {
      if (is_state_finished) {
        Serial.println("Starting pumping");
        is_state_finished = false;

        io.setPumpState(true);

        pump_start_time = std::chrono::milliseconds(millis());
      }

      if (pump_duration <
          std::chrono::milliseconds(millis()) - pump_start_time) {
        Serial.println("Finished pumping");

        io.setPumpState(false);

        is_state_finished = true;
        report_index++;
      }
    } break;
    case bernd_box::Sensor::kTotalDissolvedSolids: {
      const bernd_box::Sensor tds_id = bernd_box::Sensor::kTotalDissolvedSolids;
      if (is_state_finished) {
        Serial.println("Starting total dissolved solids measurement");
        is_state_finished = false;

        io.enableAnalog(tds_id);
        dallasTemperatureTask.enable();

        tds_start_time = std::chrono::milliseconds(millis());
      }

      float raw_analog = io.readAnalog(tds_id);
      float analog_v =
          raw_analog * io.analog_reference_v_ / io.analog_raw_range_;
      float temperature_c = dallasTemperatureTask.getLastSample().value;
      float temperature_coefficient = 1 + 0.02 * (temperature_c - 25);
      float compensation_v = analog_v / temperature_coefficient;
      float tds =
          (133.42 * std::pow(compensation_v, 3) -
           255.86 * std::pow(compensation_v, 2) + 857.39 * compensation_v) *
          0.5;
      Serial.printf(
          "TDS = %fmg/L, Compensation = %f, TempCoef = %f, °C = %f, Analog V = "
          "%f\n",
          tds, compensation_v, temperature_coefficient, temperature_c,
          analog_v);

      // Exit condition
      if (tds_duration < std::chrono::milliseconds(millis()) - tds_start_time) {
        Serial.println("Finished pumping");

        dallasTemperatureTask.disable();
        io.disableAnalog(tds_id);

        is_state_finished = true;
        report_index++;
      }
    } break;
    case bernd_box::Sensor::kAciditiy: {
      // Execute once at start of task
      if (is_state_finished) {
        Serial.println("Starting acidity measurement");

        is_state_finished = false;
        io.enableAnalog(bernd_box::Sensor::kAciditiy);

        io.clearAcidityMeasurements();
        update_acidity_sensor::id =
            timer.every(1000, update_acidity_sensor::callback);
      }

      // Exit condition, once enough samples have been collected
      if (io.isAcidityMeasurementFull()) {
        Serial.println("Finishing acidity measurement");

        float raw_analog = io.getMedianAcidityMeasurement();
        float analog_v =
            raw_analog * io.analog_reference_v_ / io.analog_raw_range_;
        float acidity_ph = analog_v * acidity_factor_v_to_ph - acidity_offset;

        Serial.printf("Acidity is %f pH\n", acidity_ph);
        mqtt.send("acidity_ph", acidity_ph);

        is_state_finished = true;
        report_index++;
      }

    } break;
    case bernd_box::Sensor::kWaterTemperature: {
      // Start the update dallas temperature thread
      if (is_state_finished) {
        temperature_request_time = std::chrono::milliseconds(millis());

        dallasTemperatureTask.enable();
        is_state_finished = false;
      }

      // End once an update has been saved
      const bernd_box::Measurement& measurement =
          dallasTemperatureTask.getLastSample();
      if (measurement.timestamp > temperature_request_time) {
        dallasTemperatureTask.disable();

        Serial.printf("Temperature is %f °C\n", measurement.value);
        mqtt.send("water_temperature_c", measurement.value);

        is_state_finished = true;
        report_index++;
        // TODO: check the sensor ID
      }

    } break;
    case bernd_box::Sensor::kTurbidity: {
      if (is_state_finished) {
        turbidity_start_time = std::chrono::milliseconds(millis());

        is_state_finished = false;
      }

      float turbidity_v = io.readAnalog(bernd_box::Sensor::kTurbidity) *
                          io.analog_reference_v_ / io.analog_raw_range_;
      Serial.printf("Turbidity is %fV\n", turbidity_v);

      // End once an update has been saved
      if (turbidity_duration <
          std::chrono::milliseconds(millis()) - turbidity_start_time) {
        Serial.printf("Turbidity is %fV\n", turbidity_v);
        mqtt.send(io.adcs_.at(bernd_box::Sensor::kTurbidity).name, turbidity_v);

        is_state_finished = true;
        report_index++;
        // TODO: check the sensor ID
      }

    } break;
    default: {
      mqtt.sendError("Measurement Report",
                     "Transition to unhandled state" +
                         String(int(report_list[report_index])),
                     true);

      report_index++;
      is_report_finished = true;
    }
  }
}
}  // namespace measurement_report

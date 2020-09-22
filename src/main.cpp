/**
 * \author Moritz Ulmer <moritz.ulmer@posteo.de>
 * \date 2018
 * \copyright MIT License
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <TaskScheduler.h>

#include "configuration.h"
#include "managers/io.h"
#include "managers/mqtt.h"
#include "managers/network.h"
#include "managers/services.h"
#include "utils/ota.h"
// #include "peripherals/peripheral_factory.h"
// #include "peripherals/peripheral_manager.h"
#include "peripheral/capabilities/get_value.h"
// #include "tasks/acidity_sensor.h"
// #include "tasks/air_sensors.h"
// #include "tasks/analog_sensors.h"
#include "tasks/connectivity.h"
// #include "tasks/dallas_temperature.h"
// #include "tasks/dissolved_oxygen_sensor.h"
// #include "tasks/light_sensors.h"
// #include "tasks/measurement_protocol.h"
// #include "tasks/pump.h"
#include "tasks/system_monitor.h"
#include "utils/setupNode.h"

//----------------------------------------------------------------------------
// Global instances
Scheduler& scheduler = bernd_box::Services::getScheduler();

// bernd_box::Io io(bernd_box::Services::getMqtt());

//----------------------------------------------------------------------------
// TaskScheduler tasks and report list
/*std::vector<bernd_box::tasks::ReportItem> report_list{
    {bernd_box::tasks::Action::kWaterTemperature},
    {bernd_box::tasks::Action::kDissolvedOxygen},
    {bernd_box::tasks::Action::kTotalDissolvedSolids, std::chrono::seconds(10)},
    {bernd_box::tasks::Action::kAcidity},
    {bernd_box::tasks::Action::kTurbidity, std::chrono::seconds(10)},
    {bernd_box::tasks::Action::kSleep, std::chrono::minutes(15)},
};*/

bernd_box::tasks::CheckConnectivity checkConnectivity(
    &scheduler, bernd_box::wifi_connect_timeout,
    bernd_box::mqtt_connection_attempts);
/*bernd_box::tasks::DallasTemperature dallasTemperatureTask(
    &scheduler, io, bernd_box::Services::getMqtt());
bernd_box::tasks::AnalogSensors analogSensorsTask(
    &scheduler, io, bernd_box::Services::getMqtt());
bernd_box::tasks::AciditySensor aciditySensorTask(
    &scheduler, io, bernd_box::Services::getMqtt());
bernd_box::tasks::LightSensors lightSensorsTask(&scheduler, io,
                                                bernd_box::Services::getMqtt());
bernd_box::tasks::AirSensors airSensorsTask(&scheduler, io,
                                            bernd_box::Services::getMqtt());
bernd_box::tasks::DissolvedOxygenSensor dissolvedOxygenSensorTask(
    &scheduler, io, bernd_box::Services::getMqtt());
bernd_box::tasks::MeasurementProtocol measurementProtocol(
    &scheduler, bernd_box::Services::getMqtt(), io, report_list, pumpTask,
    dallasTemperatureTask, dissolvedOxygenSensorTask, aciditySensorTask);*/
bernd_box::tasks::SystemMonitor systemMonitorTask(&scheduler);
// bernd_box::tasks::L293dMotors l293d_motors(&scheduler, io,
// bernd_box::Services::getMqtt());

//----------------------------------------------------------------------------
// Setup and loop functions
void setup() {
  if (!bernd_box::setupNode()) {
    Serial.println(F("Node setup failed. Restarting"));
    delay(1000);
    ESP.restart();
  }

  checkConnectivity.enable();
  systemMonitorTask.enable();

  // const std::set<String>& get_value_types =
  //     bernd_box::peripheral::capabilities::GetValue::getTypes();
  // DynamicJsonDocument get_value_types_doc(
  //     JSON_ARRAY_SIZE(get_value_types.size()));
  // JsonArray get_value_types_array = get_value_types_doc.to<JsonArray>();

  // for (const auto& type : get_value_types) {
  //   get_value_types_array.add(type.c_str());
  // }

  // bernd_box::Services::getMqtt().send("capability::GetValue",
  //                                     get_value_types_doc);

  sdg::setup_ota("bernd_box_a", "sdg");
}

long last_update_ms = 0;
long update_period_ms = 500;

void loop() {
  if (millis() - last_update_ms > update_period_ms) {
    last_update_ms = millis();
    Serial.print("h");
  }

  sdg::handle_ota();
  scheduler.execute();
}

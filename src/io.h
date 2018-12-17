/**
 * Read the inputs from the various sensors
 *
 * \author Moritz Ulmer <moritz.ulmer@posteo.de>
 * \date 2018
 * \copyright Apache License 2.0
 */

#ifndef BERND_BOX_IO_H
#define BERND_BOX_IO_H

#include <Arduino.h>

// STD C++ includes placed after Arduino.h
#include <algorithm>
#include <cmath>
#include <map>
#include <string>

#include "sensor_types.h"

namespace bernd_box {

enum class Result {
  kSuccess = 0,             // Operation completed successfully
  kFailure = 1,             // Catch-all error state
  kNotReady = 2,            // Device not ready to be used
  kDeviceDisconnected = 3,  // Device could not be found
  kInvalidPin = 4           // Invalid pin configuration
};

class Io {
 public:
  /// Pin to the status on-board LED
  const uint status_led_pin_ = 2;

  /// Pin to the pump
  const uint pump_pin_ = 18;

  // List of connected Dallas temperature sensors (DS18B20)
  const uint one_wire_pin_ = 27;
  std::map<Sensor, DallasSensor> dallases_ = {
      {{Sensor::kWaterTemperature},
       {{0}, DallasResolution::b12, "water_temperature", "°C"}},
  };

  /// List of connected BH1750 and MAX44009 light sensors
  const uint i2c_scl_pin_ = 22;
  const uint i2c_sda_pin_ = 21;

  /// List of connected BME280 sensor paramters
  const std::map<Sensor, Bme280Sensor> bme280s_ = {}; /*
      {{Sensor::kAirTemperature},
       {0x77, Bme280Parameter::kTemperatureC, "air_temperature", "°C"}},
      {{Sensor::kAirPressure},
       {0x77, Bme280Parameter::kPressure, "air_pressure", "Pa"}},
      {{Sensor::kAirHumidity},
       {0x77, Bme280Parameter::kHumidity, "air_humidity", "%"}},
  };*/
  /// List of connected BME280 sensors
  std::map<uint, BME280> bme280sensors_ = {/*{{0x77}, {BME280()}}*/};

  /// List of BH1750 sensors
  std::map<Sensor, Bh1750Sensor> bh1750s_ = {}; /*
       {{Sensor::kLightLevel},
        {BH1750::CONTINUOUS_LOW_RES_MODE, BH1750(0x23), "light_level", "lx"}},
       {{Sensor::kLightLevel2},
        {BH1750::ONE_TIME_LOW_RES_MODE, BH1750(0x5C), "light_level2", "lx"}},
   };*/

  std::map<Sensor, Max44009Sensor> max44009s_ = {}; /*
      {{Sensor::kLightLevel3}, {Max44009(0x4A), "ambient_brightness", "lx"}},
  };*/

  /// List of connected analog peripherials
  const std::map<Sensor, AdcSensor> adcs_ = {
      {{Sensor::kTurbidity}, {35, "turbidity", 1.0, "NTU", -1}},
      {{Sensor::kAciditiy}, {32, "acidity", 1.0, "pH", 33}},
      {{Sensor::kTotalDissolvedSolids},
       {39, "total_dissolved_solids", 1.0, "mg/l", 26}},
      {{Sensor::kDissolvedOxygen}, {36, "dissolved_oxygen", 1.0, "SO2", 39}},
      {{Sensor::kConductivity}, {34, "conductivity", 1.0, "mS/cm", 35}},
  };
  const uint analog_raw_range_ = 4096;
  const float analog_reference_v_ = 3.3;

  // Configure acidity related measurement
  static const uint aciditiy_sample_count_ = 30;
  static const uint temperature_sample_count_ = 5;

  Io();

  virtual ~Io();

  /**
   * Performs initialization of the connected sensors
   *
   * \return True if it was successful
   */
  Result init();

  /**
   * Turns the status LED on or off
   *
   * \param state True to turn the LED on
   */
  void setStatusLed(bool state);

  /**
   * Reads the value from the specified sensor
   *
   * \param sensor_id ID of the sensor to read out the value
   * \return Value read from the sensor, NAN if an error occured
   */
  float read(Sensor sensor_id);

  /**
   * Reads the analog value of the sensor with the given ID
   *
   * \param sensor_id The id of the sensor
   * \return Value in specified unit. NAN on error or not found
   */
  float readAnalog(Sensor sensor_id);

  void enableAnalog(Sensor sensor_id);
  void enableAnalog(const AdcSensor& adc);
  void enableAllAnalog();

  void disableAnalog(Sensor sensor_id);
  void disableAnalog(const AdcSensor& adc);
  void disableAllAnalog();

  Result setPumpState(bool state);

  /**
   * Gets the temperature in sync mode (~800ms blocking)
   *
   * \param sensor_id ID of the temperature sensor
   * \return Temperature in Celsius, NAN if sensor is not found
   */
  float readDallasTemperature(Sensor sensor_id);

  /**
   * Gets last stored temperature from temperature buffer
   *
   * \return Temperature in °C
   */
  Measurement getDallasTemperatureSample();

  /**
   * Sets the temperature in the temperature ring buffer
   *
   * \param temperature The temperature in °C
   */
  void setDallasTemperatureSample(const float temperature_c, Sensor sensorId);

  void clearDallasTemperatureSamples();

  /**
   * Requests the selected Dallas sensor to acquire the current temperature
   *
   * The duration of the process depends on the set resolution (100ms - 800ms)
   *
   * \param sensor The one-wire address of the Dallas sensor
   * \param millisToWait Returns the expected wait time depending on the
   * resolution
   * \return kSuccess on success, kDeviceDisconnected if the sensor could not be
   * found
   */
  Result requestDallasTemperatureUpdate(const DallasSensor& sensor,
                                        int& millisToWait);

  /**
   * Returns the temperature in °C is possible from selected sensor
   *
   * On error, the temperature_c variable will be set to NAN.
   *
   * \param sensor The sensor from which the temperature should be read
   *
   * \param asyncMode If true, requestTemperatureUpdate() has to have been
   * called and the appropriate time waited. It will not block. If it is false,
   * it will block until the sensor has acquired the temperature. This can take
   * up to 800ms.
   *
   * \param temperature_c The variable
   * where the temperature will be returned
   *
   * \return kSuccess on success, kNotReady if the read was too soon after the
   * request, kDeviceDisconnected if the device could not be found
   */
  Result readDallasTemperature(const DallasSensor& sensor, bool asyncMode,
                               float& temperature_c);

  /**
   * Converts the 8 byte Dallas DeviceAddress uint8_t array to a 64bit integer
   *
   * \param address Pointer to the 8 byte uint8_t array
   * \return Integer representation of the address
   */
  long dallasAddressToInt(const uint8_t* address);

  /**
   * Gets the light level of the Bh1750 light sensors
   *
   * Error values can be 0 or 65536, but are also valid readings. To avoid
   * false-positives, don't check for these values.
   *
   * \param sensor_id ID of the light sensor
   * \return Light level in lux, NAN if an error occured
   */
  float readBh1750Light(Sensor sensor_id);

  /**
   * Gets the light level of the MAX44009 light sensors
   *
   * \param sensor_id ID of the light sensor
   * \return Light level in lux, NAN if an error occured
   */
  float readMax44009Light(Sensor sensor_id);

  float readBme280Air(Sensor sensor_id);

  /**
   * Takes an acidity measurement and saves it in the ring buffer
   */
  void takeAcidityMeasurement();

  /**
   * Clears all acidity measurements
   */
  void clearAcidityMeasurements();

  /**
   * Gets the median value of the acidity measurements
   */
  float getMedianAcidityMeasurement();

  /**
   * Returns true if the acidity measurement buffer is full
   */
  bool isAcidityMeasurementFull();

 private:
  /**
   * Checks if the sensor naming in the maps do not overlap
   *
   * \return True if they are valid
   */
  bool isSensorIdNamingValid();

  /// Returned when name of invalid sensor is requested
  const std::string empty_ = "";

  // Interface to Dallas temperature sensors
  OneWire one_wire_;
  DallasTemperature dallas_;

  // Buffer for the last temperature measurement <°C, millis timestamp>
  std::array<Measurement, temperature_sample_count_> temperature_samples_{};
  uint temperature_sample_index_ = 0;

  // Buffer for the acidity samples
  std::array<float, aciditiy_sample_count_> acidity_samples_;
  uint acidity_sample_index_ = 0;
};

}  // namespace bernd_box

#endif

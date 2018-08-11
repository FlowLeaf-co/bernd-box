#include "io.h"

namespace bernd_box {

Io::Io() : one_wire_(one_wire_pin_) { acidity_samples_.fill(NAN); }

Io::~Io() {}

bool Io::init() {
  bool success = true;
  // Set the status LED GPIO to output
  pinMode(status_led_pin_, OUTPUT);

  // Start Wire for the BH1750 light sensors, then set the sense mode
  Wire.begin(i2c_sda_pin_, i2c_scl_pin_);
  for (auto& it : bh1750s_) {
    it.second.interface.begin(it.second.mode);
  }

  // Start up the Dallas Temperature library
  dallas_.begin();
  // locate devices on the bus
  uint dallas_count = dallas_.getDeviceCount();
  Serial.printf("Found %d temperature senors\n", dallas_count);
  if (dallas_count == dallases_.size()) {
    uint index = 0;
    for (auto it = dallases_.begin(); it != dallases_.end(); ++it) {
      dallas_.getAddress(it->second.address, index);
      index++;
    }
  } else {
    Serial.printf("Expected to find %d sensors\n", dallases_.size());
    success = false;
  }

  // Check if no sensor IDs are reused among different types
  if (!isSensorIdNamingValid()) {
    success = false;
  }

  return success;
}

void Io::setStatusLed(bool state) {
  if (state == true) {
    digitalWrite(status_led_pin_, HIGH);
  } else {
    digitalWrite(status_led_pin_, LOW);
  }
}

float Io::read(Sensor sensor_id) {
  // If no matching sensors are found, return NAN
  float value = NAN;

  // If no sensor has been found, check analog sensors
  if (value == NAN) {
    value = readAnalog(sensor_id);
  }

  // If no sensor has been found, check the temperature sensors
  if (value == NAN) {
    value = readTemperature(sensor_id);
  }

  // If no sensor has been found, check the light sensors
  if (value == NAN) {
    value = readBh1750Light(sensor_id);
  }

  // If no sensor has been found, check the light sensors
  if (value == NAN) {
    value = readMax44009Light(sensor_id);
  }

  return value;
}

float Io::readAnalog(Sensor sensor_id) {
  float value = NAN;

  auto it = adcs_.find(sensor_id);
  if (it != adcs_.end()) {
    value = analogRead(it->second.pin_id);
    value *= it->second.scaling_factor;
  }

  return value;
}

float Io::readTemperature(Sensor sensor_id) {
  float temperature_c = NAN;

  auto it = dallases_.find(sensor_id);

  // If the sensor was found in the registered list
  if (it != dallases_.end()) {
    temperature_c = dallas_.getTempC(it->second.address);

    // If the value is invalid, print its ID and address
    if (temperature_c == DEVICE_DISCONNECTED_C) {
      Serial.printf("Dallas sensor (%d) not connected: ",
                    static_cast<uint>(sensor_id));
      for (uint i = 0; i < sizeof(DeviceAddress); i++) {
        Serial.print(it->second.address[i]);
      }
      Serial.println();
      temperature_c = NAN;
    }
  }

  return temperature_c;
}

float Io::readBh1750Light(Sensor sensor_id) {
  float lux = NAN;

  const auto& it = bh1750s_.find(sensor_id);

  // If the sensor was found in the registered list
  if (it != bh1750s_.end()) {
    lux = it->second.interface.readLightLevel();
  }

  return lux;
}

float Io::readMax44009Light(Sensor sensor_id) {
  float lux = NAN;

  const auto& it = max44009s_.find(sensor_id);

  // If the sensor was found in the registered list
  if (it != max44009s_.end()) {
    lux = it->second.interface.getLux();
    int error = it->second.interface.getError();
    if (error != 0) {
      Serial.printf("MAX44009 error while reading: %d\n", error);
    }
  }

  return lux;
}

void Io::takeAcidityMeasurement() {
  if (acidity_sample_index_ < acidity_samples_.size()) {
    acidity_samples_[acidity_sample_index_] = readAnalog(Sensor::kAciditiy);
    acidity_sample_index_++;

    if (acidity_sample_index_ >= acidity_samples_.size()) {
      acidity_sample_index_ = 0;
    }
  } else {
    Serial.printf(
        "Error updating acidity sensor. acidity_sample_index_ (%d) greater "
        "than acidity_samples_ size (%d)\n",
        acidity_sample_index_, acidity_samples_.size());
  }
}

void Io::clearAcidityMeasurements() {
  for (auto& it : acidity_samples_) {
    it = NAN;
  }

  acidity_sample_index_ = 0;
}

float Io::getMedianAcidityMeasurement() {
  size_t samples_count;

  // Find how many measurements have been stored
  if (isAcidityMeasurementFull()) {
    samples_count = acidity_samples_.size();
  } else {
    samples_count = 0;

    // Find the first element before a NaN element
    while (acidity_samples_[samples_count] != NAN &&
           samples_count < acidity_samples_.size()) {
      samples_count++;
    }
  }

  // Copy all valid measurements to a temporary buffer
  std::vector<float> samples(samples_count);
  std::copy(acidity_samples_.begin(), &(acidity_samples_.at(samples_count)),
            samples.begin());

  // Sorts the lower half of the buffer
  std::nth_element(samples.begin(), samples.begin() + samples.size() / 2,
                   samples.end());

  // Returns the middle element
  return samples.at(samples.size() / 2);
}

bool Io::isAcidityMeasurementFull() { return acidity_samples_.back() == NAN; }

bool Io::isSensorIdNamingValid() {
  bool valid = true;

  // Compare adcs with dallases and bh1750s
  for (auto& i : adcs_) {
    for (auto& j : dallases_) {
      if (i.first == j.first) {
        valid = false;
        Serial.printf("Same ID (%d) used by adcs and dallases\n",
                      static_cast<uint>(i.first));
      }
    }
    for (auto& j : bh1750s_) {
      if (i.first == j.first) {
        valid = false;
        Serial.printf("Same ID (%d) used by adcs and bh1750s\n",
                      static_cast<uint>(i.first));
      }
    }
  }

  // Compare dallases_ with bh1750s
  for (auto& i : dallases_) {
    for (auto& j : bh1750s_) {
      if (i.first == j.first) {
        valid = false;
        Serial.printf("Same ID (%d) used by dallases and bh1750s\n",
                      static_cast<uint>(i.first));
      }
    }
  }
  return valid;
}

}  // namespace bernd_box

#include "capacitive_sensor.h"

namespace bernd_box {
namespace peripheral {
namespace peripherals {
namespace capacative_sensor {

CapacitiveSensor::CapacitiveSensor(const JsonObjectConst& parameter) {
  JsonVariantConst sense_pin = parameter[sense_pin_key_];
  if (!sense_pin.is<unsigned int>()) {
    setInvalid(sense_pin_key_error_);
    return;
  }

  sense_pin_ = sense_pin;
}

const String& CapacitiveSensor::getType() const { return type(); }

const String& CapacitiveSensor::type() {
  static const String name{"CapacitiveSensor"};
  return name;
}

capabilities::ValueUnit CapacitiveSensor::getValue() {
  return capabilities::ValueUnit{static_cast<float>(touchRead(sense_pin_)),
                                 "raw"};
}

const __FlashStringHelper* CapacitiveSensor::sense_pin_key_ = F("sense_pin");
const __FlashStringHelper* CapacitiveSensor::sense_pin_key_error_ =
    F("Missing property: sense_pin (unsigned int)");

std::shared_ptr<Peripheral> CapacitiveSensor::factory(
    const JsonObjectConst& parameter) {
  return std::make_shared<CapacitiveSensor>(parameter);
}

bool CapacitiveSensor::registered_ =
    PeripheralFactory::registerFactory(type(), factory);

bool CapacitiveSensor::capability_get_value_ =
    capabilities::GetValue::registerType(type());

}  // namespace capacative_sensor
}  // namespace peripherals
}  // namespace peripheral
}  // namespace bernd_box

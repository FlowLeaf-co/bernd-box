#include "led.h"

namespace bernd_box {
namespace peripheral {
namespace peripherals {
namespace led {

Led::Led(const JsonObjectConst& parameters) {
  JsonVariantConst led_pin = parameters[led_pin_key_];

  if (!led_pin.is<unsigned int>()) {
    Services::getServer().sendError(
        type(),
        String(F("Missing property: ")) + led_pin_key_ + F(" (unsigned int)"));
    setInvalid();
    return;
  }

  bool error = setup(led_pin);
  if (error) {
    Services::getServer().sendError(
        type(), String(F("No remaining LED channels available")));
    setInvalid();
    return;
  }
}

Led::~Led() { freeResources(); }

const String& Led::getType() { return type(); }

const String& Led::type() {
  static const String name{"LED"};
  return name;
}

void Led::setValue(capabilities::ValueUnit value_unit) {
  if (value_unit.unit != String(set_value_unit_)) {
    Services::getServer().sendError(
        type(), "Mismatching unit. Got: " + value_unit.unit + " instead of " +
                    set_value_unit_);
    return;
  }

  // Bound value as a percentage between 0 and 1
  if (value_unit.value < 0) {
    value_unit.value = 0;
  } else if (value_unit.value > 1) {
    value_unit.value = 1;
  }

  const uint max_value = (2 << resolution_) - 1;
  ledcWrite(led_channel_, value_unit.value * max_value);
}

const __FlashStringHelper* Led::led_pin_key_ = F("pin");

bool Led::setup(uint pin, uint freq, uint resolution) {
  // If the LED has already been setup, free it
  if (led_channel_ != -1 || led_pin_ != -1) {
    freeResources();
  }

  // Checks if there are any free channels remaining
  if (busy_led_channels_.all()) {
    return true;
  }

  // Find the first free channel
  for (int i = 0; i < busy_led_channels_.size(); i++) {
    if (!busy_led_channels_.test(i)) {
      led_channel_ = i;
      break;
    }
  }

  // Reserve the channel as well as saving the pin and resolution
  busy_led_channels_[led_channel_] = true;
  led_pin_ = pin;
  resolution_ = resolution;

  // Setup the channel
  ledcSetup(led_channel_, freq, resolution);

  // Attach the pin to the configured channel
  ledcAttachPin(led_pin_, led_channel_);

  return false;
}

void Led::freeResources() {
  if (led_channel_ >= 0 && led_channel_ < busy_led_channels_.size()) {
    busy_led_channels_[led_channel_] = false;
  }
  ledcDetachPin(led_pin_);
  led_pin_ = -1;
  led_channel_ = -1;
  resolution_ = -1;
}

std::shared_ptr<Peripheral> Led::factory(const JsonObjectConst& parameters) {
  return std::make_shared<Led>(parameters);
}

bool Led::registered_ = PeripheralFactory::registerFactory(type(), factory);

bool Led::capability_set_value_ = capabilities::SetValue::registerType(type());

std::bitset<16> Led::busy_led_channels_;

const __FlashStringHelper* Led::set_value_unit_ = F("%");

}  // namespace led
}  // namespace peripherals
}  // namespace peripheral
}  // namespace bernd_box

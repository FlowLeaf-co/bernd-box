#include "air_sensors.h"

namespace bernd_box {
namespace tasks {

AirSensors::AirSensors(Scheduler* scheduler, Io& io, Mqtt& mqtt)
    : Task(scheduler), io_(io), mqtt_(mqtt) {
  setIterations(TASK_FOREVER);
  Task::setInterval(std::chrono::milliseconds(default_period_).count());
}

AirSensors::~AirSensors() {}

bool AirSensors::OnEnable() {
  mqtt_.send("air_sensors_active", "true");

  return true;
}

bool AirSensors::Callback() {
  io_.setStatusLed(true);

  for (const auto& bme : io_.bme280s_) {
    float value = io_.readBme280Air(bme.first);
    Serial.printf("The %s is %f %s\n", bme.second.name.c_str(), value,
                  bme.second.unit.c_str());
    mqtt_.send(bme.second.name.c_str(), value);
  }

  io_.setStatusLed(false);

  return true;
}

void AirSensors::OnDisable() { mqtt_.send("air_sensors_active", "false"); }

}  // namespace tasks
}  // namespace bernd_box
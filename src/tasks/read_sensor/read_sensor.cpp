#include "read_sensor.h"

namespace bernd_box {
namespace tasks {

ReadSensor::ReadSensor(const JsonObjectConst& parameters, Scheduler& scheduler)
    : GetValuesTask(parameters, scheduler) {
  if(!isValid()) {
    return;
  }

  // Perform one iteration, then exit
  setIterations(1);
  enable();
}

const String& ReadSensor::getType() const { return type(); }

const String& ReadSensor::type() {
  static const String name{"ReadSensor"};
  return name;
}

bool ReadSensor::Callback() {
  // Create the JSON doc
  DynamicJsonDocument telemetry_doc(BB_JSON_PAYLOAD_SIZE);
  JsonObject telemetry_obj = telemetry_doc.to<JsonObject>();

  // Insert the value units and peripheral UUID
  makeTelemetryJson(telemetry_obj);
  
  // Send the result to the server
  Services::getServer().sendTelemetry(getTaskID(), telemetry_obj);

  return true;
}

bool ReadSensor::registered_ = TaskFactory::registerTask(type(), factory);

BaseTask* ReadSensor::factory(const JsonObjectConst& parameters,
                              Scheduler& scheduler) {
  return new ReadSensor(parameters, scheduler);
}

}  // namespace tasks
}  // namespace bernd_box

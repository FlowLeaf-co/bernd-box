#pragma once

#include <memory>

#include "ArduinoJson.h"
#include "periphery/capabilities/get_value.h"
#include "tasks/get_value_task/get_value_task.h"

namespace bernd_box {
namespace tasks {

class AlertSensor : public GetValueTask {
 public:
  enum class TriggerType { kRising, kFalling, kEither };

  AlertSensor(const JsonObjectConst& parameters, Scheduler& scheduler,
              BaseTask::RemoveCallback remover);
  virtual ~AlertSensor() = default;

  const __FlashStringHelper* getType() final;
  static const __FlashStringHelper* type();

  bool OnEnable() final;
  bool Callback() final;

  /**
   * Sets the trigger type
   *
   * Used to evaluate which type of crossing the threshold will cause an alert
   * to be sent. Sets kInvalid type on unknown type.
   *
   * @param trigger_type The type as string [rising, falling, either]
   * @return true on valid trigger type
   */
  bool setTriggerType(const String& trigger_type);

  /**
   * Set the Trigger Type object
   *
   *
   * @see setTriggerType(const String&)
   * @param type The type as a trigger type enum
   */
  void setTriggerType(const TriggerType type);

  /**
   * Returns the current trigger type
   *
   * @see setTriggerType(const String&)
   * @return TriggerType
   */
  TriggerType getTriggerType();

  TriggerType triggerType2Enum(const String& trigger_type);
  const __FlashStringHelper* triggerType2String(TriggerType trigger_type);

 private:
  bool sendAlert(TriggerType trigger_type);
  bool isRisingThreshold(const float value);
  bool isFallingThreshold(const float value);

  static bool registered_;
  static std::unique_ptr<BaseTask> factory(
      const JsonObjectConst& parameters, Scheduler& scheduler,
      BaseTask::RemoveCallback remove_callback);

  static const std::map<TriggerType, const __FlashStringHelper*>
      trigger_type_strings_;

  /// Default interval to poll the sensor with
  const std::chrono::milliseconds default_interval_{100};

  /// The direction of the sensor value crossing the trigger to send an alert
  TriggerType trigger_type_;

  /// The threshold to create an alert for
  float threshold_;

  /// Last measured sensor value
  float last_value_;
};  // namespace tasks

}  // namespace tasks
}  // namespace bernd_box

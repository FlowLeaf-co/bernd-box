#include "library.h"

#include "managers/services.h"

namespace bernd_box {
namespace library {

Library::Library(Mqtt& mqtt, periphery::PeripheryFactory& periphery_factory,
                 periphery::PeripheryTaskFactory& periphery_task_factory)
    : mqtt_(mqtt),
      periphery_factory_(periphery_factory),
      periphery_task_factory_(periphery_task_factory) {}

Result Library::add(const JsonObjectConst& doc) {
  const __FlashStringHelper* who = F(__PRETTY_FUNCTION__);

  JsonVariantConst name = doc[F("name")];
  if (name.isNull() || !name.is<char*>()) {
    mqtt_.sendError(who, F("Missing property: name (string)"));
    return Result::kFailure;
  }

  // Check if element is present
  if (peripheries_.count(name) > 0) {
    mqtt_.sendError(who, F("This name has already been added."));
    return Result::kFailure;
  }

  // Not present, so create new
  std::shared_ptr<periphery::Periphery> periphery(
      periphery_factory_.createPeriphery(doc));
  if (periphery->isValid() == false) {
    return Result::kFailure;
  }

  peripheries_.emplace(name.as<String>(), periphery);

  return Result::kSuccess;
}

Result Library::remove(const JsonObjectConst& doc) {
  const char* who = __PRETTY_FUNCTION__;

  JsonVariantConst name = doc[F("name")];
  if (name.isNull() || !name.is<char*>()) {
    mqtt_.sendError(who, String(F("Missing property: name (string)")));
    return Result::kFailure;
  }

  auto iterator = peripheries_.find(name.as<String>());
  if (iterator != peripheries_.end()) {
    if (iterator->second.use_count() > 1) {
      mqtt_.sendError(
          who, String(F("Object is still in use. Try again later for ")) +
                   name.as<String>());
      return Result::kFailure;
    }
    peripheries_.erase(iterator);
  }

  return Result::kSuccess;
}

Result Library::execute(const JsonObjectConst& doc) {
  const char* who = __PRETTY_FUNCTION__;

  JsonVariantConst name = doc[F("name")];
  if (name.isNull() || !name.is<char*>()) {
    mqtt_.sendError(who, "Missing property: name (string)");
    return Result::kFailure;
  }

  auto iterator = peripheries_.find(name.as<String>());
  if (iterator == peripheries_.end()) {
    mqtt_.sendError(who, "No object found with name " + name.as<String>());
    return Result::kFailure;
  }
  periphery::TaskFactory& task_factory = iterator->second->getTaskFactory(doc);

  JsonVariantConst parameter = doc[F("parameter")];
  if (parameter.isNull()) {
    return Result::kFailure;
  }

  periphery::PeripheryTask& peripheryTask =
      task_factory.createTask(iterator->second, parameter);
  Services::getScheduler().addTask(peripheryTask);
  peripheryTask.enable();
  return Result::kSuccess;
}

std::shared_ptr<periphery::Periphery> Library::getPeriphery(
    const String& name) {
  return peripheries_.find(name)->second;
}

void Library::handleCallback(char* topic, uint8_t* payload,
                             unsigned int length) {
  const char* who = __PRETTY_FUNCTION__;

  char* command = strrchr(topic, '/');
  command++;

  DynamicJsonDocument doc(BB_MQTT_JSON_PAYLOAD_SIZE);
  const DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    mqtt_.sendError(who, String(F("Deserialize failed: ")) + error.c_str());
    return;
  }

  int action_topic_add = strncmp(command, BB_MQTT_TOPIC_ADD_SUFFIX,
                                 strlen(BB_MQTT_TOPIC_ADD_SUFFIX));
  if (action_topic_add == 0) {
    add(doc.as<JsonVariantConst>());
    return;
  }
  int action_topic_remove = strncmp(command, BB_MQTT_TOPIC_REMOVE_PREFIX,
                                    strlen(BB_MQTT_TOPIC_REMOVE_PREFIX));
  if (action_topic_remove == 0) {
    remove(doc.as<JsonVariantConst>());
    return;
  }
  int action_topic_execute = strncmp(command, BB_MQTT_TOPIC_EXECUTE_PREFIX,
                                     strlen(BB_MQTT_TOPIC_EXECUTE_PREFIX));
  if (action_topic_execute == 0) {
    execute(doc.as<JsonVariantConst>());
    return;
  }
  mqtt_.sendError(
      who, String(F("Unknown action [add, remove, execute]: ")) + command);
  return;
}

void Library::taskCallback(char* topic, uint8_t* payload, unsigned int length) {
  const char* who = __PRETTY_FUNCTION__;

  char* command = strrchr(topic, '/');
  command++;
  String command_str(command);

  DynamicJsonDocument doc(BB_MQTT_JSON_PAYLOAD_SIZE);
  const DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    mqtt_.sendError(who, String(F("Deserialize failed: ")) + error.c_str());
    return;
  }

  if (command_str.equals(task_add_suffix)) {
    JsonVariantConst name = doc[F("name")];
    if (name.isNull() || !name.is<char*>()) {
      mqtt_.sendError(who, "Missing property: name (string)");
      return;
    }

    auto periphery = peripheries_.find(name.as<String>());
    if (periphery == peripheries_.end()) {
      mqtt_.sendError(who, "No object found with name " + name.as<String>());
      return;
    }

    periphery_task_factory_.createTask(periphery->second, doc.as<JsonVariantConst>());
    return;
  }

  if (command_str.equals(task_remove_suffix)) {
    periphery_task_factory_.stopTask(doc.as<JsonVariantConst>());
    return;
  }

  mqtt_.sendError(who, String(F("Unknown action [add, remove]: ")) + command);
  return;
}

}  // namespace library
}  // namespace bernd_box

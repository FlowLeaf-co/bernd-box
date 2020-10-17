#include "task_factory.h"

namespace bernd_box {
namespace tasks {

TaskFactory::TaskFactory(Server& server, Scheduler& scheduler)
    : server_(server), scheduler_(scheduler) {}

const String& TaskFactory::type() {
  static const String name{"TaskFactory"};
  return name;
}

bool TaskFactory::registerTask(const String& type, Factory factory) {
  return getFactories().insert({type, factory}).second;
}

BaseTask* TaskFactory::createTask(const JsonObjectConst& parameters) {
  JsonVariantConst type = parameters[type_key_];
  if (type.isNull() || !type.is<char*>()) {
    return new InvalidTask(scheduler_, type_key_error_);
  }

  // Check if a factory for the type exists. Then try to create such a task
  const auto& factory = getFactories().find(type);
  if (factory != getFactories().end()) {
    // Create a task via the respective task factory
    return factory->second(parameters, scheduler_);
  } else {
    // Factory type not found, so return an invalid task
    return new InvalidTask(scheduler_,
                           invalidFactoryTypeError(type.as<char*>()));
  }
}

const std::vector<String> TaskFactory::getFactoryNames() {
  std::vector<String> names;
  for (const auto& factory : getFactories()) {
    names.push_back(factory.first);
  }
  return names;
}

std::map<String, TaskFactory::Factory>& TaskFactory::getFactories() {
  static std::map<String, Factory> factories;
  return factories;
}

String TaskFactory::invalidFactoryTypeError(const String& type) {
  String error(F("Could not find the factory type: "));
  error += type;
  return error;
}

}  // namespace tasks
}  // namespace bernd_box

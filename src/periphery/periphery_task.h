#ifndef BERND_BOX_PERIPHERY_TASK_H
#define BERND_BOX_PERIPHERY_TASK_H

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#include <memory>

// #include "managers/io.h"
#include "managers/io_types.h"
#include "periphery.h"

namespace bernd_box {
namespace periphery {

class PeripheryTask : public Task {
 private:
  std::shared_ptr<Periphery> periphery_;
  Scheduler& scheduler_;

 public:
  PeripheryTask(std::shared_ptr<Periphery> periphery);
  virtual ~PeripheryTask() = default;

  virtual void OnDisable() final;
  virtual void OnTaskDisable(); 

  std::shared_ptr<Periphery> getPeriphery();
};

class TaskFactory {
 public:
  virtual PeripheryTask& createTask(
      std::shared_ptr<Periphery> periphery,
      const JsonObjectConst& parameter) = 0;
};

}  // namespace periphery
}  // namespace bernd_box

#endif

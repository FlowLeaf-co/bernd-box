#pragma once

#include "peripheral/peripheral.h"
#include "peripheral/peripheral_factory.h"

namespace bernd_box {
namespace peripheral {

class InvalidPeripheral : public Peripheral {
 public:
  InvalidPeripheral();
  InvalidPeripheral(const String& error);
  virtual ~InvalidPeripheral() = default;

  const String& getType() const final;
  static const String& type();

 private:
  static std::shared_ptr<Peripheral> factory(const JsonObjectConst&);

  static bool registered_;
};

}  // namespace peripheral
}  // namespace bernd_box
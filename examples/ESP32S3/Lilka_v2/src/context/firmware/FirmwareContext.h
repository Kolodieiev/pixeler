#pragma once
#include <stdint.h>

#include "context/IContext.h"

using namespace pixeler;

class FirmwareContext : public IContext
{
public:
  FirmwareContext();
  virtual ~FirmwareContext() {}

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_NAVBAR = 1,
    ID_TITLE,
    ID_AUTHOR,
    ID_VERSION,
  };

  void showUpdating();
};
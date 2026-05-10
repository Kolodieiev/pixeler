#pragma once
#include <stdint.h>

#include "../src/context/IContext.h"

using namespace pixeler;

class SplashContext : public IContext
{
public:
  SplashContext();
  virtual ~SplashContext() {};

protected:
  virtual bool loop() override;
  virtual void update() override;
  //

private:
  unsigned long _start_time;
  uint8_t _widget_id{1};
};
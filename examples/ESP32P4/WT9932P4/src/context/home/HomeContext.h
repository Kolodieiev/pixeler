#pragma once

#include "context/IContext.h"
#include "lib/neo_pixel/Adafruit_NeoPixel.h"

using namespace pixeler;

class HomeContext : public IContext
{
public:
  HomeContext();
  virtual ~HomeContext();

protected:
  virtual bool loop() override;
  virtual void update() override;
  //
private:
  Adafruit_NeoPixel _strip;
};

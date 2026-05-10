#pragma once

#include "context/IContext.h"
#include "widget/progress/ProgressBar.h"

using namespace pixeler;

class PrefBrightContext : public IContext
{
public:
  PrefBrightContext();
  virtual ~PrefBrightContext() {}

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_NAVBAR = 2,
    ID_HEADER,
    ID_PROGRESSBAR
  };

  uint8_t _old_bright;
  ProgressBar* _progress;
};
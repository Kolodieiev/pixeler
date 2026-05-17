#include "context/resources/ch32_pins_def.h"
#include "Pixeler.h"
#include "manager/CoprocessorManager.h"
#include "input_config.h"
using namespace pixeler;

void setup()
{
  // Запустити виконання Pixeler.
  Pixeler::begin(80);
}

void loop()
{
  vTaskDelete(NULL);
}

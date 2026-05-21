#include "Pixeler.h"
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

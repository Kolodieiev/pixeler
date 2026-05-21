#include "context/resources/ch32_pins_def.h"
#include "input_config.h"
#include "Pixeler.h"
#include "manager/CoprocessorManager.h"
using namespace pixeler;

void setup()
{
  // Перевірити з'єднання з допоміжним МК.
  if (!pixeler::_ccpu.connect())
    esp_restart();

  // Увімкнути живлення дисплея з допоміжного МК.
  uint8_t ccpu_cmd_data[2]{CCPU_CMD_PIN_ON, CH_PIN_DISPLAY_PWR};
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data));

  // Увімкнути всі кнопки на допоміжному МК.
  ccpu_cmd_data[0] = CCPU_CMD_BTN_ON;

  ccpu_cmd_data[1] = BtnID::BTN_OK;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = BtnID::BTN_BACK;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = BtnID::BTN_LEFT;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = BtnID::BTN_RIGHT;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = BtnID::BTN_UP;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = BtnID::BTN_DOWN;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  // Вимкнути всю периферію на допоміжному МК.
  ccpu_cmd_data[0] = CCPU_CMD_PIN_OFF;

  ccpu_cmd_data[1] = CH_PIN_MIC_PWR;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = CH_PIN_LORA_PWR;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  ccpu_cmd_data[1] = CH_PIN_SPK_PWR;
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data));

  // Запустити виконання Pixeler.
  Pixeler::begin(80);
}

void loop()
{
  vTaskDelete(NULL);
}

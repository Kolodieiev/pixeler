#include "HomeContext.h"

#include "manager/FileManager.h"

#define LED_COUNT 1
#define LED_DATA_PIN 51

HomeContext::HomeContext() : _strip{LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800}
{
  _fs.mount();

  _strip.setBrightness(50);
  _strip.begin();
  _strip.clear();
  _strip.setPixelColor(0, Adafruit_NeoPixel::Color(255, 0, 255));
  _strip.show();
}

HomeContext::~HomeContext()
{
}

bool HomeContext::loop()
{
  // Використовуємо функцію часу millis(), щоб отримати плавне значення від 0.0 до 1.0
  // Число 2000 — це швидкість (чим більше, тим повільніше змінюється колір)
  float progress = (sin(millis() / 1000.0) + 1.0) / 2.0;

  // Лінійна інтерполяція (плавний перехід) між двома точками:
  // Червоний змінюється від 100 (синьо-фіолетовий) до 200 (темно-рожевий)
  int r = 100 + (progress * (200 - 100));

  // Синій змінюється від 255 (синьо-фіолетовий) до 50 (темно-рожевий)
  int b = 255 + (progress * (50 - 255));

  // Виводимо колір на перший світлодіод
  _strip.setPixelColor(0, Adafruit_NeoPixel::Color(r, 0, b));
  _strip.show();
  return true;
}

void HomeContext::update()
{
}

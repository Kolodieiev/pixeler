# Підключення дисплея

Pixeler підтримує взаємодію з дисплеєм через модифіковану бібліотеку `Arduino_GFX`. 
Налаштування бібліотеки та дисплея знаходяться в файлі `config/graphics_config.h`.
<br>
<br>

Для уніфікованого підключення різних драйверів дисплеїв та шин, створення об'єкту драйвера відбувається за допомогою макросів.

```cpp
BUS_TYPE _bus{BUS_PARAMS};
```

Тому в `graphics_config.h` необхідно коректно налаштувати ці макроси, щоб відповідні значення були підставлені в код на етапі компіляції.

```cpp
// Клас шини.
#define BUS_TYPE Arduino_ESP32SPI                                                         
// Парапор, який вказує чи є шина спільною для декількох пристроїв. Наразі завжди false.
#define IS_COMMON_BUS false                                                               
// Параметри класу шини.
#define BUS_PARAMS TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, SPI_PORT, IS_COMMON_BUS  
```
<br>
<br>

Відповідно для об'єкта виводу зображення

```cpp
Arduino_GFX* _output = new DISP_DRIVER_TYPE(&_bus, DISP_DRIVER_PARAMS);
```

Додаємо такі макроси

```cpp
// Клас драйвера дисплея.
#define DISP_DRIVER_TYPE Arduino_ST7789
// Параметри класу драйвера дисплея БЕЗ адреси шини. 
// Адреса об'єкта шини підставляється в коді ініціалізації.                                           
#define DISP_DRIVER_PARAMS TFT_RST, DISPLAY_ROTATION, IS_IPS_DISPLAY, DISPLAY_WIDTH, DISPLAY_HEIGHT  
```
<br>

З параметрами та їх порядком передачі в конструктор можна ознайомитися самостійно у відповідних файлах драйверів дисплеїв бібліотеки `Arduino_GFX`.
<br>
<br>

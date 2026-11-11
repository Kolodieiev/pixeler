### Lua

У Pixeler вбудовано повноцінну віртуальну машину **Lua 5.4.8** (з виправленнями помилок), доповнену нативним C++ API для взаємодії з апаратною частиною мікроконтролера та графічною підсистемою фреймворку.

Використання Lua дозволяє запускати застосунки, утиліти та ігри безпосередньо з SD-карти пам'яті або з мережі без необхідності перекомпільовувати та перепрошивати основну прошивку пристрою.

#### Особливості реалізації LuaVM у Pixeler

* **Розумний розподіл пам'яті (RAM / PSRAM):** Кастомний алокатор віртуальної машини контролює вільну оперативну пам'ять. Він використовує швидку внутрішню SRAM мікроконтролера, доки в ній залишається щонайменше **40 КБ** вільного місця для потреб самої системи, після чого автоматично перемикається на виділення пам'яті у зовнішній **PSRAM** (якщо вона присутня на платі).
* **Автоматичне прибирання сміття та захист від зависань:** Кожні 10 000 інструкцій ВМ спрацьовує системний хук, який перевіряє обсяг зайнятої пам'яті (при перевищенні 30 КБ примусово запускається Garbage Collector) і періодично віддає керування планувальнику FreeRTOS (`delay(1)`), щоб важкі цикли в Lua не викликали спрацьовування Watchdog-таймера.
* **Економія ресурсів через ліниве завантаження:** Базові типи віджетів та важкі периферійні модулі (SD, Wi-Fi, I2C, UART тощо) не засмічують глобальну таблицю Lua за замовчуванням — вони підключаються лише за запитом скрипта через `initType()` та `require()`, і можуть бути вивантажені під час роботи через `unrequire()`.
* **Автоматичне очищення ресурсів:** Під час закриття `LuaContext` усі завантажені скриптом зображення (`loadImg`) та взяті на облік віджети (`context.manageWidget`) автоматично видаляються з пам'яті.

<br>

#### Інтеграція `LuaContext` та запуск скрипта з C++

У прошивці за роботу віртуальної машини відповідає спеціалізований клас контексту **`LuaContext`** (`#include "lib/lua/context/LuaContext.h"`). Найчастіше він використовується як **вкладений субконтекст** (наприклад, коли користувач обирає файл `.lua` у файловому менеджері, і після завершення скрипта керування повертається назад у список файлів).

Для завантаження та старту скрипта викликається метод:

```cpp
/**
 * @brief Завантажує та виконує початкову ініціалізацію скрипта у віртуальній машині Lua.
 *
 * @param lua_script Нуль-термінований рядок з вихідним кодом скрипта.
 * @return true — якщо скрипт успішно скомпільовано, виконано та знайдено функцію update().
 * @return false — якщо виникла синтаксична/рантайм помилка або відсутня функція update().
 */
bool execScript(const char* lua_script);
```

Після того як метод `execScript()` повернув `true`, вихідний текстовий буфер скрипта у C++ можна одразу звільнити (`free`), оскільки байт-код уже завантажено у віртуальну машину.

##### Зчитування помилок виконання (`getMsg()`)

Якщо `execScript()` повернув `false` (помилка під час запуску) або якщо помилка сталася пізніше під час покадрового виконання функції `update()`, `LuaContext` автоматично перехоплює стек помилки, зберігає текст причини та викликає `releaseContext()` для завершення своєї роботи.

Отримати опис помилки в батьківському контексті можна за допомогою методу:

```cpp
/**
 * @brief Повертає повідомлення від LuaVM, якщо під час запуску або виконання скрипта сталася помилка.
 * Якщо скрипт завершився штатно через context.exit(), повертає порожній рядок "".
 */
String getMsg() const;
```

##### Повний приклад запуску `.lua` файлу з SD-карти (у ролі субконтексту)

```cpp
#include "lib/lua/context/LuaContext.h"

void FilesContext::executeScript()
{
  String file_name = makePathFromBreadcrumbs() + "/" + _files_list->getCurrItemText();

  size_t f_size = _fs.getFileSize(file_name.c_str());
  if (f_size == 0)
  {
    log_e("Скрипт порожній");
    return;
  }

  // Виділяємо тимчасовий буфер під текст скрипта (бажано в PSRAM)
  char* text_buf = psramInit() ? static_cast<char*>(ps_malloc(f_size + 1)) 
                               : static_cast<char*>(malloc(f_size + 1));
  if (!text_buf)
  {
    log_e("Помилка виділення пам'яті");
    return;
  }

  hideContextMenu();

  // Вичитуємо скрипт із карти пам'яті
  _fs.readFile(file_name.c_str(), text_buf, f_size);
  text_buf[f_size] = '\0';

  // Створюємо екземпляр LuaContext і передаємо скрипт на виконання
  _lua_context = new LuaContext();
  if (!_lua_context->execScript(text_buf))
  {
    // Якщо скрипт не запустився — виводимо повідомлення про помилку
    String msg = _lua_context->getMsg();
    _notification->setMsgText(msg);
    _mode = MODE_NOTIFICATION;
    showNotification(_notification);
    delete _lua_context;
    _lua_context = nullptr;
  }
  else
  {
    // Переводимо батьківський контекст у режим виконання субконтексту Lua
    _mode = MODE_LUA;
  }

  // Текстовий буфер більше не потрібен
  free(text_buf);
}

// У методі loop() батьківського контексту передаємо керування субконтексту Lua:
bool FilesContext::loop()
{
  if (_mode == MODE_LUA)
  {
    if (_lua_context->isReleased())
    {
      // Скрипт завершив роботу (штатно або через помилку в update)
      String msg = _lua_context->getMsg();
      if (!msg.equals(""))
      {
        _mode = MODE_NOTIFICATION;
        _notification->setMsgText(msg);
        showNotification(_notification);
        getLayout()->drawForced();
      }
      else
      {
        _mode = MODE_NAVIGATION;
        getLayout()->drawForced();
      }

      delete _lua_context;
      _lua_context = nullptr;
    }
    else
    {
      // Виконуємо один тік LuaContext і повертаємо false, 
      // щоб батьківський контекст не перехоплював ввід та рендеринг
      _lua_context->tick();
      return false;
    }
  }
  return true;
}
```

<br>

#### Структура Lua-скрипта

Кожен скрипт для Pixeler **обов'язково повинен містити визначення глобальної функції `update()`**. Якщо функції `update()` у скрипті немає, `LuaContext` відхилить запуск із відповідним повідомленням.

* Увесь код, написаний у корені скрипта (поза межами функцій), виконується **один раз під час старту** (аналог конструктора або `setup()`).
* Функція **`update()`** викликається фреймворком автоматично на кожному кадрі. У ній розміщується опитування кнопок/тачскріна та оновлення стану інтерфейсу.

```lua
-- Код ініціалізації (виконується один раз на старті)
local BTN_EXIT <const> = 2
showToast("Скрипт запущено!")

-- Обов'язкова покадрова функція
function update()
    if input.is_pressed(BTN_EXIT) then
        context.exit() -- Штатний вихід зі скрипта
    end
end
```

<br>

#### Глобальні системні функції та таблиці

Наведені нижче функції та таблиці реєструються автоматично і доступні в будь-якому скрипті одразу без додаткового підключення.

##### 1. Глобальні допоміжні функції

| Функція в Lua | Опис |
| :--- | :--- |
| **`initType("TypeName")`** | Ініціалізує C++ клас віджета для використання у скрипті (автоматично підтягує всі батьківські типи-залежності). |
| **`unrequire("module_name")`** | Викликає деструктор модуля (`__unload`), вивантажує раніше підключений через `require()` модуль із пам'яті та очищує його глобальне ім'я. |
| **`showToast(text, [time])`** | Показує спливаюче повідомлення `text`. Другий необов'язковий аргумент задає тривалість показу. |
| **`showNotification(msg)`** <br> **`showNotification(title, msg, left_btn, right_btn)`** | Відображає модальне вікно сповіщення (у спрощеному варіанті з 1 аргументом або з повним налаштуванням заголовка та підписів кнопок). |
| **`hideNotification()`** | Ховає активне вікно сповіщення. |
| **`loadImg("/path/to/img.bin")`** | Завантажує бінарний ресурс зображення з SD-карти у менеджер ресурсів та повертає його цілочисельний `id` (`> 0` у разі успіху). Автоматично очищується при виході зі скрипта. |
| **`deleteImg(id)`** | Вручну видаляє раніше завантажене зображення з оперативної пам'яті за його `id`. |

<br>

##### 2. Таблиця `context` (Керування поточним контекстом)

| Метод | Опис |
| :--- | :--- |
| **`context.exit()`** | Штатно завершує роботу Lua-скрипта та звільняє `LuaContext`. |
| **`context.getLayout()`** | Повертає вказівник на кореневий контейнер інтерфейсу (`IWidgetContainer*`). Перед використанням методів контейнера обов'язково викличте `initType("IWidgetContainer")`. |
| **`context.manageWidget(widget)`** | Передає віджет під автоматичне керування пам'яттю `LuaContext` (він буде гарантовано видалений при закритті скрипта, якщо не доданий до іншого контейнера). |

<br>

##### 3. Таблиця `input` (Користувацький ввід)

| Метод / Константа | Опис |
| :--- | :--- |
| **`input.is_pressed([btn_id])`** | Повертає `true`, якщо кнопку `btn_id` утримують тривалий час (довге затискання). Якщо викликано без аргументів (за увімкненого `TOUCHSCREEN_SUPPORT`) — перевіряє довгий дотик до тачскріна. |
| **`input.is_holded([btn_id])`** | Повертає `true`, якщо кнопку `btn_id` (або тачскрін при виклику без аргументів) затиснуто в поточний момент. |
| **`input.is_released([btn_id])`** | Повертає `true`, якщо кнопку `btn_id` (або тачскрін при виклику без аргументів) було натиснуто і відпущено (клік). |
| **`input.enable_btn(btn_id)`** | Вмикає опитування вказаної кнопки. |
| **`input.disable_btn(btn_id)`** | Тимчасово вимикає опитування вказаної кнопки. |
| **`input.getTouchX()`**, **`input.getTouchY()`** | Повертають координати `X` та `Y` останнього дотику до сенсорного екрана *(доступно за `TOUCHSCREEN_SUPPORT`)*. |
| **`input.getSwipe()`** | Повертає розпізнаний жест свайпу. Порівнюється з глобальними константами: `SWIPE_NONE`, `SWIPE_L`, `SWIPE_R`, `SWIPE_U`, `SWIPE_D` *(доступно за `TOUCHSCREEN_SUPPORT`)*. |

##### 4. Таблиця `mcu` (Керування мікроконтролером)

| Метод | Опис |
| :--- | :--- |
| **`mcu.millis()`** | Повертає час у мілісекундах, що минув із моменту запуску мікроконтролера. |
| **`mcu.delay(ms)`** | Призупиняє виконання поточного потоку на вказану кількість мілісекунд. |
| **`mcu.setCpuFreq(mhz)`** | Встановлює робочу частоту процесора в МГц (наприклад, `80`, `160`, `240`). |

<br>

##### 5. Таблиця `pin` (Робота з GPIO)

| Метод | Опис |
| :--- | :--- |
| **`pin.input(pin_num)`** | Переводить вказаний пін у режим цифрового входу (`INPUT`). |
| **`pin.output(pin_num)`** | Переводить вказаний пін у режим цифрового виходу (`OUTPUT`). |
| **`pin.pullup(pin_num)`** | Переводить пін у режим входу з внутрішньою підтяжкою до живлення (`INPUT_PULLUP`). |
| **`pin.toggle(pin_num)`** | Інвертує логічний рівень на цифровому виході піна. |
| **`pin.read(pin_num)`** | Зчитує цифровий стан піна (`0` або `1`). |
| **`pin.analogRead(pin_num)`** | Зчитує аналогове значення з АЦП (ADC) на вказаному піні. |

<br>

##### 6. Таблиця `gl` (Пряме малювання та параметри дисплея)

Таблиця `gl` надає доступ до параметрів екрана та примітивів прямого малювання на полотні (`Canvas`). Також у глобальному просторі імен автоматично доступні базові константи кольорів (`COLOR_BLACK`, `COLOR_WHITE`, `COLOR_RED`, `COLOR_GREEN`, `COLOR_BLUE`, `COLOR_YELLOW`, `COLOR_ORANGE`, `COLOR_CYAN`, `COLOR_LIME`, `COLOR_MAGENTA`, `COLOR_GREY`).

| Метод | Опис |
| :--- | :--- |
| **`gl:width()`**, **`gl.width()`** | Повертає ширину робочої області екрана в пікселях. |
| **`gl:height()`**, **`gl.height()`** | Повертає висоту робочої області екрана в пікселях. |
| **`gl.drawRect(x, y, w, h, color)`** | Малює контур прямокутника. |
| **`gl.fillRect(x, y, w, h, color)`** | Малює зафарбований прямокутник. |
| **`gl.drawRoundRect(x, y, w, h, r, color)`** | Малює контур прямокутника зі скругленими кутами радіуса `r`. |
| **`gl.fillRoundRect(x, y, w, h, r, color)`** | Малює зафарбований прямокутник зі скругленими кутами радіуса `r`. |
| **`gl.setFont(font)`** | Встановлює поточний шрифт для прямого виводу тексту. |
| **`gl.setTextSize(size)`** | Встановлює масштаб шрифту. |
| **`gl.setTextColor(color)`** | Встановлює колір тексту. |
| **`gl.print(text, x, y)`** | Виводить рядок тексту у вказаних координатах полотна. |

<br>

#### Динамічні модулі (`require` / `unrequire`)

Щоб економити оперативну пам'ять, спеціалізовані модулі для роботи з периферією та мережею підключаються динамічно за допомогою стандартної функції `require("module_name")`. Коли модуль більше не потрібен у скрипті, його ресурси можна звільнити викликом `unrequire("module_name")`.

##### 1. Модуль `sd` — Карта пам'яті (`require("sd")`)

Надає доступ до файлового менеджера Pixeler зі скриптів Lua:

| Метод | Опис |
| :--- | :--- |
| **`sd.fileExist(path)`** | Перевіряє, чи існує файл за вказаним шляхом. |
| **`sd.dirExist(path)`** | Перевіряє, чи існує директорія за вказаним шляхом. |
| **`sd.fileSize(path)`** | Повертає розмір файлу в байтах. |
| **`sd.open(path, mode)`** | Відкриває файл у вказаному режимі (`"r"`, `"w"`, `"a"` тощо) та повертає дескриптор. |
| **`sd.close(file)`** | Закриває раніше відкритий дескриптор файлу. |
| **`sd.available(file)`** | Перевіряє, скільки байтів доступно для читання з відкритого файлу. |
| **`sd.read(path, [len], [seek])`** | Одноразово читає дані з файлу за шляхом без ручного відкриття. |
| **`sd.readFrom(file, len, [seek])`** | Читає порцію даних із раніше відкритого дескриптора файлу. |
| **`sd.write(path, data)`** | Створює/перезаписує файл за шляхом і записує в нього дані. |
| **`sd.writeTo(file, data)`** | Дописує дані у відкритий дескриптор файлу. |
| **`sd.copyFile(from, to)`** | Копіює файл. |
| **`sd.remove(path)`** | Видаляє файл або директорію. |
| **`sd.rename(old_path, new_path)`** | Перейменовує або переміщує файл чи папку. |
| **`sd.mkDir(path)`** | Створює нову директорію. |

<br>

##### 2. Модуль `wifi` — Керування Wi-Fi (`require("wifi")`)

| Метод | Опис |
| :--- | :--- |
| **`wifi.enable()`** | Вмикає радіомодуль Wi-Fi. Повертає `true` у разі успіху. |
| **`wifi.disable()`** | Вимикає Wi-Fi модуль для збереження енергії. |
| **`wifi.isEnabled()`** | Повертає `true`, якщо модуль Wi-Fi увімкнено. |
| **`wifi.tryConnectTo(ssid, pwd)`** | Розпочинає асинхронне підключення до точки доступу. |
| **`wifi.isBusy()`** | Повертає `true`, доки триває процес підключення до мережі. |
| **`wifi.isConnected()`** | Повертає `true`, якщо підключення до роутера встановлено. |
| **`wifi.disconnect()`** | Розриває поточне з'єднання з точкою доступу. |
| **`wifi.createAP(ssid, pwd)`** | Запускає власну точку доступу (Access Point) на мікроконтролері. |
| **`wifi.isApEnabled()`** | Перевіряє, чи активний режим точки доступу. |
| **`wifi.setPower(level)`** | Задає потужність передавача (`0` — мінімальна, `1` — середня, `2` — максимальна). |
| **`wifi.getSSID()`** | Повертає назву поточної мережі. |
| **`wifi.getIP()`** | Повертає виділену IP-адресу у вигляді рядка. |

<br>

##### 3. Модуль `web` — Мережеві HTTP/HTTPS запити (`require("web")`)

| Метод | Опис |
| :--- | :--- |
| **`web.get(url)`** | Виконує HTTP/HTTPS `GET`-запит і повертає тіло відповіді сервера у вигляді рядка (або `""` у разі помилки). |
| **`web.post(url, body)`** | Виконує `POST`-запит із довільним текстовим тілом `body`. |
| **`web.jsonPost(url, json_str)`** | Виконує `POST`-запит із заголовком `application/json` та передає JSON-рядок. |
| **`web.formPost(url, form_str)`** | Виконує `POST`-запит із заголовком `application/x-www-form-urlencoded`. |

###### Приклад роботи з Wi-Fi та HTTP-запитами в Lua

```lua
local ssid <const> = "YOUR_SSID"
local pwd <const> = "YOUR_PWD"

local wifi = require("wifi")
local web = require("web")

local BTN_EXIT <const> = 2
local BTN_UP <const> = 5

local server_get = "[https://httpbun.com/get](https://httpbun.com/get)"
local server_post = "[https://httpbun.com/post](https://httpbun.com/post)"

function freeRes()
    unrequire("web")
    web = nil
    unrequire("wifi")
    wifi = nil
    print("Ресурси wifi та web звільнено")
end

function runNetworkTest()
    if not wifi.enable() then
        print("Помилка увімкнення WiFi")
        return
    end

    wifi.tryConnectTo(ssid, pwd)
    while wifi.isBusy() do
        mcu.delay(1)
    end

    if not wifi.isConnected() then
        print("Помилка підключення до " .. ssid)
        return
    end

    print("Підключено! IP: " .. wifi.getIP())

    -- 1. GET-запит
    local resp_get = web.get(server_get .. "?key1=value1")
    print("GET response: " .. resp_get)

    -- 2. POST JSON-запит
    local json_data = '{"device":"ESP32","temp":25.5,"status":"active"}'
    local resp_json = web.jsonPost(server_post, json_data)
    print("JSON POST response: " .. resp_json)

    -- 3. POST Form-запит
    local form_data = "username=esp32user&password=secret123"
    local resp_form = web.formPost(server_post, form_data)
    print("Form POST response: " .. resp_form)

    -- Звільняємо пам'ять модулів після завершення роботи
    freeRes()
end

function update()
    if input.is_pressed(BTN_EXIT) then
        context.exit()
    elseif input.is_released(BTN_UP) then
        wifi.setPower(2) -- Максимальна потужність (0..2)
        runNetworkTest()
    end
end
```

<br>

##### 4. Модулі апаратних інтерфейсів: `i2c`, `uart`, `pwm`

* **`require("i2c")` — Шина I2C:**
  * `i2c.begin([sda], [scl], [freq])`, `i2c.end()` — ініціалізація та зупинка шини.
  * `i2c.hasConnect(addr)` — перевірка наявності пристрою за адресою.
  * `i2c.scanBus()` — сканування шини на наявність підключених пристроїв.
  * `i2c.write(addr, data)`, `i2c.read(addr, len)` — прямий запис і читання.
  * `i2c.beginTr(addr)`, `i2c.send(byte)`, `i2c.receive()`, `i2c.endTr()` — покрокова транзакція.
* **`require("uart")` — Послідовний порт UART:**
  * `uart.begin(baud, [rx_pin], [tx_pin], [uart_num])`, `uart.end()` — запуск та зупинка порту.
  * `uart.updateBaudRate(baud)`, `uart.setRxBuffSize(size)` — зміна швидкості та розміру RX-буфера.
  * `uart.available()`, `uart.peek()`, `uart.read()` — перевірка та вичитування вхідних даних.
  * `uart.print(str)`, `uart.println(str)`, `uart.flush()` — передача даних у порт.
* **`require("pwm")` — Широтно-імпульсна модуляція (ШІМ):**
  * `pwm.attach(pin, [freq], [resolution])`, `pwm.detach(pin)` — прив'язка та відв'язка ШІМ до піна.
  * `pwm.set(pin, duty)`, `pwm.stop(pin)` — встановлення шпаруватості сигналу та зупинка генерації.


<br>

#### Робота з графічним інтерфейсом (GUI) та віджетами в Lua

Класи віджетів у Lua не завантажуються у віртуальну машину за замовчуванням, щоб не витрачати оперативну пам'ять у скриптах, яким не потрібен інтерфейс. 

Перед створенням віджета його тип необхідно зареєструвати викликом **`initType("НазваКласу")`**. Механізм ініціалізації автоматично підтягує всі базові класи та супутні типи-залежності:
* Наприклад, виклик `initType("Menu")` автоматично ініціалізує `IWidgetContainer` та `IWidget`.
* Виклик `initType("ToggleItem")` автоматично ініціалізує `ToggleSwitch`, `MenuItem`, `Label`, `Image` та `IWidget`.

Під час ініціалізації `IWidget` у глобальний простір імен також автоматично додаються константи вирівнювання, орієнтації та видимості:
* **Вирівнювання по горизонталі (`Align`):** `ALIGN_START`, `ALIGN_CENTER`, `ALIGN_END`
* **Вирівнювання по вертикалі (`Gravity`):** `GRAVITY_TOP`, `GRAVITY_CENTER`, `GRAVITY_BOTTOM`
* **Орієнтація (`Orientation`):** `HORIZONTAL`, `VERTICAL`
* **Видимість (`Visibility`):** `VISIBLE`, `INVISIBLE`
* **Глобальні константи кольорів:** `COLOR_TRANSPARENT`, `COLOR_BLACK`, `COLOR_WHITE`, `COLOR_RED`, `COLOR_GREEN`, `COLOR_GREY`, `COLOR_BLUE`, `COLOR_YELLOW`, `COLOR_ORANGE`

<br>

##### Створення та клонування віджетів

Новий екземпляр будь-якого конкретного віджета створюється за допомогою конструктора **`ClassName:new(id)`**, де `id` — унікальний числовий ідентифікатор віджета в межах його батьківського контейнера (`> 0`). Для швидкого копіювання вже налаштованого віджета використовується метод **`widget:clone(new_id)`**.

##### Доступні класи віджетів та їхні методи в Lua

| Клас (`initType`) | Успадковує | Доступні методи в Lua |
| :--- | :--- | :--- |
| **`IWidget`** | — *(базовий)* | `setPos(x, y)`, `getXPos()`, `getYPos()`, `setWidth(w)`, `getWidth()`, `setHeight(h)`, `getHeight()`, `getID()`, `setBackColor(c)`, `setCornerRadius(r)`, `setBorder(bool)`, `setBorderColor(c)`, `setChangingBorder(bool)`, `setChangingBack(bool)`, `setFocusBorderColor(c)`, `setFocusBackColor(c)`, `setFocus()`, `removeFocus()`, `setVisibility(v)`, `setTransparency(bool)`, `drawForced()` |
| **`IWidgetContainer`** | `IWidget` | `addWidget(w)`, `delWidgetByID(id)`, `getWidgetByID(id)`, `getWidgetByIndx(i)`, `findTouchableAt(x, y)`, `delWidgets()`, `getSize()`, `enable()`, `disable()` |
| **`EmptyLayout`** | `IWidgetContainer` | `EmptyLayout:new(id)`, `clone(new_id)` + усі методи `IWidgetContainer` |
| **`Label`** | `IWidget` | `Label:new(id)`, `clone(new_id)`, `setText(str)`, `getText()`, `getLen()`, `setTextSize(s)`, `setTextColor(c)`, `setFont(f)`, `setGravity(g)`, `setAlign(a)`, `setHPadding(p)`, `initWidthToFit([add_w])`, `updateWidthToFit([add_w])`, `setAutoscroll(bool)`, `setAutoscrollInFocus(bool)`, `setAutoscrollDelay(ms)`, `setBackImg(img)`, `setMultiline(bool)` |
| **`Image`** | `IWidget` | `Image:new(id)`, `clone(new_id)`, `setSrc(img_id)` |
| **`Menu`** | `IWidgetContainer` | `Menu:new(id)`, `clone(new_id)`, `addItem(item)`, `focusUp()`, `focusDown()`, `setItemHeight(h)`, `getItemHeight()`, `setItemWidth(w)`, `getItemWidth()`, `setOrientation(o)`, `getCurrItemID()`, `getCurrFocusPos()`, `setCurrFocusPos(pos)`, `getCurrItemText()`, `setItemsSpacing(sp)`, `setLooped(bool)`, `delWidgets()`, `delWidgetByID(id)` |
| **`MenuItem`** | `IWidget` | `MenuItem:new(id)`, `clone(new_id)`, `setImage(img)`, `getImage()`, `setLabel(lbl)`, `getLabel()`, `setText(str)`, `getText()` |
| **`ToggleItem`** | `MenuItem` | `ToggleItem:new(id)`, `clone(new_id)`, `setToggle(sw)`, `getToggle()`, `setOn(bool)`, `toggle()`, `isOn()` |
| **`ToggleSwitch`** | `IWidget` | `ToggleSwitch:new(id)`, `clone(new_id)`, `isOn()`, `setOn(bool)`, `toggle()`, `setLeverColor(c)`, `setOnColor(c)`, `setOffColor(c)`, `setOrientation(o)` |
| **`ProgressBar`** | `IWidget` | `ProgressBar:new(id)`, `clone(new_id)`, `setMax(m)`, `getMax()`, `setProgress(p)`, `getProgress()`, `setProgressColor(c)`, `setOrientation(o)`, `reset()` |

<br>

##### Повний приклад побудови інтерактивного меню в Lua

```lua
-- Приклад створення віджета меню з фіксованою кількістю елементів списку

-- Коди кнопок
BTN_EXIT = 2
BTN_UP = 5
BTN_DOWN = 6
BTN_OK = 1

-- Ідентифікатори віджетів (мають бути > 0 та унікальними в межах одного контейнера)
HEADER_LABEL_ID = 1
MAIN_MENU_ID = 2

-- Ідентифікатори елементів списку меню
SIMPLE_ITEM_ID = 1
TOGGLE_ITEM_ID = 2

-- Додаткові кольори віджетів
COLOR_LIME = 0xA7E0
COLOR_ITEM_FOCUS_BACK = 0x39E7

-- Висота вирізів (заокруглень) екрана
CUTOUT_SIZE = 40

-- Завантажуємо зображення з карти пам'яті в PSRAM (повертається його ID > 0)
img_id_plus = loadImg("/lua/plus.bmp")

-- Підключаємо необхідні класи віджетів (залежні базові класи підключаться автоматично)
initType("ToggleItem") -- Автоматично підключить ToggleSwitch, MenuItem, Label, Image, IWidget
initType("Menu")       -- Автоматично підключить IWidgetContainer

-- Отримуємо вказівник на головний віджет-макет контексту
main_layout = context.getLayout()
main_layout:setBackColor(COLOR_BLACK)

-------------------------------------------------------- Заголовок
header_label = Label:new(HEADER_LABEL_ID)
main_layout:addWidget(header_label)
header_label:setWidth(gl:width() - CUTOUT_SIZE * 2)
header_label:setHeight(CUTOUT_SIZE)
header_label:setText("Приклад меню")
header_label:setAlign(ALIGN_CENTER)
header_label:setGravity(GRAVITY_CENTER)
header_label:setPos(CUTOUT_SIZE, 0)

---------------------------------------------------------- Меню
main_menu = Menu:new(MAIN_MENU_ID)
main_layout:addWidget(main_menu)
main_menu:setWidth(gl:width())
main_menu:setHeight(gl:height() - CUTOUT_SIZE * 2)
main_menu:setPos(0, CUTOUT_SIZE)
main_menu:setItemHeight((main_menu:getHeight() - 2) / 4)
main_menu:setLooped(true)

---------------------------------------------------------- 1. Простий елемент меню
simple_item = MenuItem:new(SIMPLE_ITEM_ID)
main_menu:addWidget(simple_item)
simple_item:setFocusBorderColor(COLOR_LIME)
simple_item:setFocusBackColor(COLOR_ITEM_FOCUS_BACK)
simple_item:setBackColor(COLOR_BLACK)
simple_item:setChangingBorder(true)
simple_item:setChangingBack(true)

simple_item_label = simple_item:getLabel()
simple_item_label:setText("Простий елемент списку")
simple_item_label:setGravity(GRAVITY_CENTER)
simple_item_label:setFocusBackColor(COLOR_ITEM_FOCUS_BACK)
simple_item_label:setAutoscrollInFocus(true)

---------------------------------------------------------- 2. Елемент з перемикачем
toggle_item = ToggleItem:new(TOGGLE_ITEM_ID)
main_menu:addWidget(toggle_item)
toggle_item:setFocusBorderColor(COLOR_LIME)
toggle_item:setFocusBackColor(COLOR_ITEM_FOCUS_BACK)
toggle_item:setChangingBorder(true)
toggle_item:setChangingBack(true)

-- Клонуємо вже налаштовану мітку з попереднього пункту
toggle_item_label = simple_item_label:clone(1)
toggle_item:setLabel(toggle_item_label)
toggle_item_label:setText("Довгий текст елемента списку меню з перемикачем")

-- Налаштовуємо вигляд перемикача
toggle_switch = toggle_item:getToggle()
toggle_switch:setWidth(40)
toggle_switch:setHeight(20)
toggle_switch:setCornerRadius(7)

-- Якщо ресурс іконки успішно завантажено з SD-карти — встановлюємо її для пунктів
if (img_id_plus > 0) then
    toggle_item_img = Image:new(1)
    toggle_item_img:setSrc(img_id_plus)
    simple_item:setImage(toggle_item_img)
    toggle_item:setImage(toggle_item_img:clone(1))
end

-------------------------------------------------------- Обробка натискання кнопки OK
function ok_handler()
    local item_id = main_menu:getCurrItemID()
    if item_id == SIMPLE_ITEM_ID then
        showToast("Натиснуто ОК")
    elseif item_id == TOGGLE_ITEM_ID then
        toggle_item:toggle()
    end
end

-------------------------------------------------------- Покадрове оновлення
function update()
    if input.is_pressed(BTN_EXIT) then
        context.exit()
    elseif input.is_released(BTN_UP) then
        main_menu:focusUp()
    elseif input.is_released(BTN_DOWN) then
        main_menu:focusDown()
    elseif input.is_released(BTN_OK) then
        ok_handler()
    end
end
```

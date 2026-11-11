### Ігровий сервер (`GameServer`)

Клас **`GameServer`** (`#include "game/online/GameServer.h"`) відповідає за створення ігрового хоста, керування підключеннями гравців у лобі, перевірку активності клієнтів (`PING`) та гарантовану доставку ігрових пакетів через вбудований трекер підтверджень **`AckTracker`**.

Сервер працює в асинхронному багатопотоковому режимі: при виклику `begin()` він запускає UDP-слухач на порту `GAME_SERVER_PORT` (за замовчуванням `777`) і створює на другому ядрі МК дві фонові FreeRTOS-задачі:
* **`packetHandlerTask`** — вичитує вхідні пакети з черги `_packet_queue` (розмір черги за замовчуванням — `32` пакети) та викликає відповідні обробники.
* **`pingClientsTask`** — щосекунди перевіряє стан сесій, надсилає `TYPE_PING` клієнтам, які мовчать понад 2 секунди, та автоматично видаляє сесії, що не відповідають понад 3 секунди.

<br>

#### 1. Запуск, налаштування та зупинка сервера

Для ініціалізації сервера використовується метод **`begin()`**:

```cpp
bool begin(
    const String& game_ID,      // Унікальний рядковий ідентифікатор гри (перевіряється при рукостисканні)
    const String& server_name,  // Ім'я сервера (також стає назвою SSID точки доступу, якщо is_local == true)
    const String& pwd,          // Пароль точки доступу Wi-Fi
    uint8_t max_clients = 1,    // Максимальна кількість клієнтів (від 1 до 10)
    bool is_local = true,       // true — підняти власну точку доступу AP; false — працювати в мережі роутера
    uint8_t wifi_chan = 6       // Номер каналу Wi-Fi
);
```

* Якщо **`is_local == true`** (режим за замовчуванням), сервер автоматично вмикає Wi-Fi модуль та розгортає власну точку доступу (`192.168.4.1`).
* Якщо **`is_local == false`**, сервер очікує, що пристрій вже підключено до зовнішнього Wi-Fi маршрутизатора.

##### Керування станом лобі (допуском нових гравців)
Після виклику `begin()` сервер за замовчуванням **закритий** для вхідних підключень. Керування допуском виконується методами:

| Метод | Опис |
| :--- | :--- |
| **`open()`** | Відкриває сервер для прийому запитів на підключення від нових клієнтів. |
| **`close()`** | Закриває сервер для нових клієнтів та автоматично очищує всі неавторизовані сесії. |
| **`toggle()`** | Перемикає стан сервера між відкритим та закритим. |
| **`isOpen()`** | Повертає `true`, якщо сервер відкритий для авторизації. |
| **`isFull()`** | Повертає `true`, якщо всі слоти (`max_clients`) вже зайняті авторизованими гравцями. |
| **`stop()`** | Повністю зупиняє сервер, видаляє фонові задачі й черги, очищує сесії та вимикає Wi-Fi. |

<br>

#### 2. Авторизація гравців у лобі

Процес приєднання гравця до відкритого сервера відбувається у два етапи:
1. **Автоматичний Handshake та перевірка імені:** Коли клієнт надсилає `SUBTYPE_HANDSHAKE`, сервер звіряє його `game_ID` зі своїм. Якщо вони збігаються, клієнт надсилає `SUBTYPE_LOGIN` зі своїм нікнеймом. Сервер автоматично відхиляє ім'я (`SUBTYPE_INCORRECT_NAME`), якщо воно порожнє, довше за `40` символів (`MAX_CLIENT_NAME_LEN`), збігається з іменем сервера або вже зайняте іншим гравцем.
2. **Підтвердження хостом (`onAccept` ➔ `resolveJoin`):** Якщо ім'я коректне, сервер тимчасово піднімає прапор `_is_busy = true` (щоб інші клієнти не перебивали діалог) і викликає встановлений розробником обробник **`onAccept(AcceptHandler handler, void* arg)`**.

Усередині обробника контекст сервера (обов'язково через `post()`) показує хосту діалог «Прийняти / Відхилити гравця» і викликає метод:

```cpp
/**
 * @brief Підтверджує або відхиляє приєднання клієнта до лобі.
 * Знімає прапор зайнятості авторизації (_is_busy = false).
 *
 * @param client_name Ім'я клієнта, яке надійшло в onAccept.
 * @param is_accepted true — впустити в лобі (SUBTYPE_ACCESS_GRANTED); false — відмовити (SUBTYPE_ACCESS_DENIED).
 */
void resolveJoin(const String& client_name, bool is_accepted);
```

Для примусового вигнання (кіку) вже підключеного гравця з лобі або гри використовуються методи **`removeSession(client_name)`** або **`removeSession(remote_ip)`**.

<br>

#### 3. Надійна доставка (`AckTracker`) та відправка даних

Оскільки UDP не гарантує доставку пакетів, у `GameServer` вбудовано механізм підтвердження доставки критичних даних за допомогою класу **`AckTracker`**.

##### Як працює відстеження підтверджень (ACK)
1. Коли сервер викликає **`broadcast(...)`** (усім авторизованим клієнтам) або **`send(session, ...)`** (конкретному клієнту), метод спочатку перевіряє **`isAllAcked()`** — чи всі клієнти підтвердили попередній пакет. Якщо попередній пакет ще не підтверджено, відправка нового блокується і метод повертає `false`.
2. Якщо всі попередні пакети підтверджені, сервер зберігає копію нового пакета у `_last_packet`, присвоює йому наступний порядковий номер (`++_last_packet_id`), скидає прапори в `AckTracker` і відправляє пакет по мережі.
3. Поки `isAllAcked()` повертає `false`, сервер ігнорує нові вхідні ігрові пакети від клієнтів, очікуючи від них лише сервісні пакети `SUBTYPE_ACK` із поточним `_last_packet_id`.
4. У покадровому циклі сцени сервер викликає метод **`requestAcks()`**: якщо хтось із клієнтів ще не надіслав `ACK`, сервер повторно пересилає йому `_last_packet`. Якщо клієнт не відповідає після **`MAX_RESEND_COUNT = 20`** повторних запитів, сервер автоматично видаляє його сесію і викликає `onDisconnect`.

| Метод відправки / ACK | Опис |
| :--- | :--- |
| **`broadcastStartGame()`** | Закриває лобі (`close()`) і розсилає всім клієнтам команду `SUBTYPE_START_GAME`. |
| **`broadcastStopGame()`** | Розсилає всім клієнтам команду завершення гри `SUBTYPE_STOP_GAME`. |
| **`broadcast(type, subtype, size, data)`** | Формує пакет із новим `ID`, скидає `ACK`-прапори для всіх клієнтів і розсилає його. |
| **`send(session_or_ip, type, subtype, size, data)`** | Формує пакет із новим `ID`, скидає `ACK`-прапор для вказаного клієнта та надсилає йому пакет. |
| **`isAllAcked()`** | Повертає `true`, якщо всі цільові клієнти підтвердили отримання останнього надісланого пакета. |
| **`requestAcks()`** | Повторно надсилає останній пакет тим клієнтам, які ще не прислали `ACK` (викликається періодично в сцені). |

<br>

#### 4. Передача об'єкта `GameServer` із контексту лобі в ігрову сцену

Типовий життєвий цикл серверної гри складається з двох фаз: **Лобі (у класі контексту)** та **Сама гра (у класі сцени)**. 

Оскільки об'єкт `GameServer _server` зберігається в класі контексту, під час старту гри контекст **відписує свої обробники лобі** (`unsubscribeServerHandlers()`), очищує віджети інтерфейсу і передає посилання на `_server` у конструктор ігрової сцени. Сцена підписує на `_server` вже власні ігрові обробники (`onGameData`, `onDisconnect`). Коли гра завершується і сцена звільняється, контекст знову підписує свої обробники і повертає гравця в лобі.

##### Приклад реалізації серверного контексту (`ServerChessContext`)

```cpp
void ServerChessContext::setupServer()
{
  // Підписуємо контекст на події авторизації та відключення в лобі
  subscribeServerHandlers();

  // Запускаємо локальний сервер (точку доступу) максимум на 3 клієнтів і відкриваємо лобі
  _server.begin(STR_CHESS_GAME_ID, server_name, server_pwd, MAX_CLIENT_NUM);
  _server.open();
}

void ServerChessContext::subscribeServerHandlers()
{
  _server.onAccept(onAcceptHandler, this);
  _server.onDisconnect(onDisconnectHandler, this);
}

void ServerChessContext::unsubscribeServerHandlers()
{
  _server.onAccept(nullptr, nullptr);
  _server.onDisconnect(nullptr, nullptr);
}

// Зворотні виклики від фонових задач сервера безпечно передають роботу в UI через post():
void ServerChessContext::onAcceptHandler(const String client_name, void* arg)
{
  ServerChessContext* self = static_cast<ServerChessContext*>(arg);
  self->post([self, client_name]()
             { self->showClientAcceptTmpl(client_name); }, 500);
}

void ServerChessContext::onDisconnectHandler(const IPAddress& client_ip, void* arg)
{
  ServerChessContext* self = static_cast<ServerChessContext*>(arg);
  self->post([self]()
             { self->showLobbyTmpl(); }, 500);
}

// Перехід із лобі в ігрову сцену:
void ServerChessContext::startGame(const String& main_client_name)
{
  _server.close();
  unsubscribeServerHandlers(); // Відключаємо колбеки контексту, щоб їх перехопила сцена

  _state_handler = &ServerChessContext::handleGame;
  IPAddress main_ip = _server.getClientIP(main_client_name);

  getLayout()->delWidgets();
  getLayout()->disable();

  // Передаємо посилання на _server всередину ігрової сцени
  _scene = new ServerChessScene(_stored_objs, _server, main_ip);
}

// Покадрове оновлення гри та повернення назад у лобі після завершення партії:
void ServerChessContext::handleGame()
{
  if (!_scene->isReleased())
  {
    _scene->update();
  }
  else
  {
    delete _scene;
    _scene = nullptr;
    subscribeServerHandlers(); // Повертаємо обробники контексту лобі
    getLayout()->enable();
    showLobbyTmpl();
    _server.open();
  }
}
```

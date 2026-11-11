### Ігровий клієнт (`GameClient`)

Клас **`GameClient`** (`#include "game/online/GameClient.h"`) відповідає за встановлення зв'язку з ігровим сервером (`GameServer`), проходження процедури рукостискання й авторизації, автоматичне підтвердження отриманих пакетів (`ACK`), контроль активності з'єднання (`PING`) та передачу намірів гравця на сервер.

При виклику `connect()` клієнт відкриває UDP-слухач на порту `GAME_CLIENT_PORT` (за замовчуванням `333`) і запускає на другому ядрі МК дві фонові FreeRTOS-задачі:
* **`packetHandlerTask`** — обробляє вхідні пакети з черги `_packet_queue` (розмір черги за замовчуванням — `16` пакетів).
* **`checkConnectTask`** — кожні 2 секунди перевіряє час останньої активності сервера. Якщо від сервера немає жодних пакетів (включно з `TYPE_PING`) понад **`MAX_PING_TIME = 4000` мс (4 секунди)**, клієнт автоматично закриває з'єднання та викликає обробник `onDisconnect`.

<br>

#### 1. Підключення до сервера та відправка даних

Перед викликом `connect()` пристрій вже повинен бути підключений до мережі Wi-Fi сервера (наприклад, до точки доступу хоста за допомогою глобального менеджера `_wifi`).

```cpp
bool connect(
    const String& client_name,             // Нікнейм гравця (до 40 символів)
    const String& game_ID,                 // Унікальний ідентифікатор гри (має збігатися з сервером)
    const String& host_IP = "192.168.4.1"  // IP-адреса сервера (за замовчуванням — IP точки доступу ESP32)
);
```

##### Основні методи `GameClient`

| Метод | Опис |
| :--- | :--- |
| **`connect(name, game_ID, [ip])`** | Ініціалізує чергу та задачі клієнта, переводить статус у `STATUS_IDLE` і автоматично надсилає серверу пакет рукостискання (`SUBTYPE_HANDSHAKE`). |
| **`disconnect()`** | Закриває UDP-з'єднання, зупиняє фонові задачі та очищує чергу пакетів (сам модуль Wi-Fi залишається увімкненим). |
| **`send(type, subtype, size, data)`** | Формує `UdpPacket` і потокобезпечно відправляє його на порт сервера (`GAME_SERVER_PORT = 777`). |
| **`getStatus()`** | Повертає поточний стан клієнта (`STATUS_IDLE`, `STATUS_CONNECTED`, `STATUS_DISCONNECTED`). |

<br>

#### 2. Автоматичне підтвердження пакетів (ACK) та фільтрація дублікатів

На стороні клієнта робота з механізмом надійної доставки (`ACK`) повністю автоматизована і не потребує втручання розробника:
1. Коли від сервера надходить пакет із прапором `hasID() == true`, переривання `onPacket` перевіряє його `packet_id`.
2. Якщо `packet_id` **збігається** з `_last_packet_id` (це означає, що сервер не отримав наш попередній `ACK` і повторно надіслав той самий пакет через `requestAcks()`), клієнт **повторно відправляє `SUBTYPE_ACK`** і одразу відкидає дублікат, не пропускаючи його в ігрову сцену.
3. Якщо `packet_id` **новий**, клієнт зберігає його у `_last_packet_id`, кладе пакет у чергу обробки та автоматично надсилає серверу підтвердження `SUBTYPE_ACK`.
4. На пакети `TYPE_PING` від сервера клієнт так само автоматично відповідає зустрічним пакетом `TYPE_PING`.

<br>

#### 3. Події та обробники помилок (`Callbacks`)

Для реагування на мережеві події в лобі та в ігровій сцені `GameClient` надає 6 зворотних викликів:

| Метод підписки | Коли викликається |
| :--- | :--- |
| **`onConnect(handler, arg)`** | Хост на сервері схвалив заявку (`SUBTYPE_ACCESS_GRANTED`), клієнт перейшов у стан `STATUS_CONNECTED` (увійшов у лобі). |
| **`OnError(handler, arg)`** | Сервер відхилив підключення. В обробник передається код помилки `GameClient::Error`. |
| **`onDisconnect(handler, arg)`** | Зв'язок із сервером втрачено за таймаутом (4 с) або сервер примусово вигнав клієнта. |
| **`onGameStart(handler, arg)`** | Від сервера надійшла команда `SUBTYPE_START_GAME` — час створити ігрову сцену і передати їй `_client`. |
| **`onGameStop(handler, arg)`** | Від сервера надійшла команда `SUBTYPE_STOP_GAME` — час завершити гру і повернутися в лобі. |
| **`onGameData(handler, arg)`** | Від сервера надійшов пакет `TYPE_GAME_DATA` із підтвердженою дією або оновленням стану світу. |

##### Коди помилок авторизації (`GameClient::Error`)
* **`ERR_INCORRECT_SERVER`** — на вибраній точці доступу працює сервер іншої гри (`game_ID` не збігається).
* **`ERR_INCORRECT_NAME`** — нікнейм клієнта порожній, довший за 40 символів або вже зайнятий іншим гравцем у лобі.
* **`ERR_ACCESS_DENIED`** — власник сервера вручну відхилив запит на вхід у лобі.
* **`ERR_SERVER_BUSY`** — лобі сервера закрите, заповнене або сервер у цей момент розглядає заявку іншого гравця.

<br>

#### 4. Повний цикл роботи клієнта: від сканування Wi-Fi до запуску сцени

Аналогічно до серверного контексту, об'єкт `GameClient _client` оголошується як поле клієнтського контексту. Спочатку контекст за допомогою `_wifi` сканує точки доступу, підключається до обраної мережі, запускає `_client.connect()`, а після отримання сигналу `onGameStart` відписує свої обробники лобі та передає посилання на `_client` у конструктор клієнтської ігрової сцени.

##### Приклад реалізації клієнтського контексту (`ClientChessContext`)

```cpp
// 1. Після успішного підключення до Wi-Fi точки доступу запускаємо GameClient:
void ClientChessContext::connectToServer()
{
  _state_handler = &ClientChessContext::handleClientConnectInput;

  subscribeClientHandlers();

  if (!_client.connect(client_name, STR_CHESS_GAME_ID))
  {
    showStateLabelTmpl(STR_SERVER_CONNECT_ERR);
    return;
  }

  showStateLabelTmpl(STR_SERVER_CONNECT);
}

void ClientChessContext::subscribeClientHandlers()
{
  _client.onConnect(onClientConnectHandler, this);
  _client.onDisconnect(onClientDisconnectHandler, this);
  _client.onGameStart(onGameStartHandler, this);
  _client.OnError(onClientErrorHandler, this);
}

void ClientChessContext::unsubscribeClientHandlers()
{
  _client.onConnect(nullptr, nullptr);
  _client.onDisconnect(nullptr, nullptr);
  _client.onGameStart(nullptr, nullptr);
  _client.OnError(nullptr, nullptr);
}

// 2. Обробники подій безпечно оновлюють UI контексту через post():
void ClientChessContext::onClientConnectHandler(void* arg)
{
  ClientChessContext* self = static_cast<ClientChessContext*>(arg);
  self->post([self]()
             { self->showLobbyTmpl(); }); // Показуємо екран "Очікування початку гри..."
}

void ClientChessContext::onGameStartHandler(void* arg)
{
  ClientChessContext* self = static_cast<ClientChessContext*>(arg);
  self->post([self]()
             { self->startGame(); }); // Сервер запустив гру!
}

// 3. Перехід із лобі в клієнтську ігрову сцену:
void ClientChessContext::startGame()
{
  getLayout()->delWidgets();
  getLayout()->disable();

  // Відключаємо колбеки лобі, щоб сцена підписала власні обробники onGameData / onGameStop
  unsubscribeClientHandlers();

  // Передаємо посилання на _client у конструктор сцени
  _scene = new ClientChessScene(_stored_objs, _client);
  _state_handler = &ClientChessContext::handleGame;
}

// 4. Покадрове оновлення сцени та повернення в лобі після гри:
void ClientChessContext::handleGame()
{
  if (!_scene->isReleased())
  {
    _scene->update();
  }
  else
  {
    delete _scene;
    _scene = nullptr;

    // Якщо зв'язок із сервером досі активний — повертаємося в клієнтське лобі
    if (_client.getStatus() == GameClient::STATUS_CONNECTED)
    {
      getLayout()->enable();
      subscribeClientHandlers();
      showLobbyTmpl();
    }
    else
    {
      openContext(new ChessContext());
    }
  }
}
```

### Ігровий рушій

Pixeler містить вбудований 2D ігровий рушій, спроєктований для створення двовимірних ігор різної складності (від покрокових настільних ігор і головоломок до динамічних RPG та мережевих ігор по Wi-Fi).

Архітектура рушія чітко відокремлює **керування екранами прошивки (`IContext`)** від **внутрішньої логіки ігрового світу (`IGameScene2D`)**.

<br>

#### Базова архітектура

Кожна 2D-гра в Pixeler будується навколо чотирьох ключових компонентів:

1. **Керуючий контекст (`IContext`):** Володіє вказівником на поточну ігрову сцену (`IGameScene2D*`) та буфером обміну даними між сценами (`DataStream`). Контекст відповідає за запуск гри, перемикання між рівнями (сценами) та вихід назад у меню прошивки.
2. **Ігрова сцена (`IGameScene2D`):** Виступає «режисером» конкретного рівня або ігрової локації. Сцена містить менеджер карти (`Terrain2D`), звуковий плеєр (`SfxPlayer`), менеджер ресурсів (`ResManager`), шаблони спрайтів, шари ігрового інтерфейсу (`IGameUI`, `IGameMenu`) та контейнер активних ігрових об'єктів.
3. **Терейн (`Terrain2D`):** Найнижчий графічний шар сцени, що відповідає за побудову тайлової мапи рівня та перевірку прохідності плиток (підлога, стіни, перешкоди).
4. **Ігрові об'єкти (`IGameObject2D`):** Персонажі, предмети, тригери, камера та інтерактивні елементи світу, які мають координати, спрайти, тверді тіла (хітбокси) та здатні взаємодіяти один з одним.

```{mermaid}
:zoom:

flowchart TD
%% Nodes
Context(["Керуючий контекст (IContext)"])
DS["DataStream _stored_objs (Спільний буфер між сценами)"]
Scene["Ігрова сцена (IGameScene2D* _scene)"]

Terrain["Terrain2D _terrain (Тайлова мапа рівня)"]
Objects["Контейнер об'єктів (IGameObject2D* та _main_obj)"]
UI["IGameUI / IGameMenu (Поверхневий інтерфейс)"]
AudioRes["SfxPlayer + ResManager (Звук та ресурси)"]

%% Edge connections between nodes
Context -- "Володіє буфером" --> DS
Context -- "Створює та оновлює update()" --> Scene
DS -. "Передається за посиланням" .-> Scene

Scene -- "Нижній шар (мапа)" --> Terrain
Scene -- "Логіка та колізії" --> Objects
Scene -- "HUD та меню паузи" --> UI
Scene -- "Аудіо та кеш" --> AudioRes

%% Individual node styling
style Context color:#FFFFFF, fill:#AA00FF, stroke:#AA00FF
style Scene color:#FFFFFF, fill:#2962FF, stroke:#2962FF
style DS color:#FFFFFF, fill:#00C853, stroke:#00C853
```

<br>

#### Інтеграція гри в контекст

Сама гра ніколи не запускається у відриві від системи контекстів Pixeler. Щоб інтегрувати гру в прошивку, необхідно:
1. У класі вашого контексту (успадкованому від `IContext`) оголосити об'єкт **`DataStream`** (спільний буфер для перенесення стану об'єктів при переходах між сценами) та вказівник **`IGameScene2D* _scene{nullptr}`**.
2. Створити екземпляр конкретної сцени через `new`, передавши у її конструктор посилання на `DataStream`.
3. У методі `update()` контексту щокадру перевіряти прапори стану сцени (**`isFinished()`** та **`isReleased()`**) і викликати **`_scene->update()`**.

##### Прапори життєвого циклу сцени

Керуючий контекст спілкується зі сценою за допомогою трьох методів:
* **`_scene->isReleased()`** — повертає `true`, якщо поточна сцена просить замінити її на іншу сцену (наприклад, гравець перейшов у наступну кімнату або на наступний рівень через `openSceneByID()`). Номер наступної сцени зчитується методом **`_scene->getNextSceneID()`**.
* **`_scene->isFinished()`** — повертає `true`, якщо гру повністю завершено (або гравець вийшов із рівня) і потрібно знищити сцену та повернутися до меню контексту.

Залежно від жанру гри інтеграція в контекст може бути простою (одна сцена) або комбінованою (меню вибору рівня + зміна сцен).

<br>

##### Варіант 1: Проста інтеграція (без проміжного меню та зміни рівнів)

Якщо гра складається з однієї самодостатньої сцени (наприклад, партія в Шахи), контекст одразу створює сцену в конструкторі та крутить її в `update()`, доки сцена не підніме прапор `isReleased()`:

```cpp
// У заголовковому файлі контексту:
class LocalChessContext : public pixeler::IContext
{
protected:
  pixeler::DataStream _stored_objs{10}; // Буфер обміну між сценами (розмір у байтах)
  pixeler::IGameScene2D* _scene{nullptr};
  // ...
};

// У файлі реалізації (.cpp):
LocalChessContext::LocalChessContext(uint8_t player_num)
{
  // Одразу ініціалізуємо ігрову сцену
  _scene = new SingleChessScene(_stored_objs);
}

void LocalChessContext::update()
{
  if (!_scene->isReleased())
  {
    // Поки сцена активна — делегуємо їй покадрове оновлення та відрисовку
    _scene->update();
  }
  else
  {
    // Сцена завершила роботу — звільняємо пам'ять і виходимо в головне меню гри
    delete _scene;
    openContext(new ChessContext());
  }
}
```

<br>

##### Варіант 2: Інтеграція з меню вибору рівнів та перемиканням сцен

Якщо в межах одного контексту потрібно показувати звичайне GUI-меню (наприклад, список із 70 рівнів у грі «Комірник» / *Sokoban*), а також підтримувати автоматичний перехід із рівня на рівень, контекст використовує прапор `_is_game_started`:

* **Перед запуском сцени** контекст очищує свій GUI-макет викликом **`getLayout()->delWidgets()`**, щоб віджети меню не займали пам'ять під час гри.
* **У процесі гри** контекст перевіряє і `isFinished()` (вихід назад у список рівнів), і `isReleased()` (перезапуск сцени з новим `next_lvl = _scene->getNextSceneID()`).

```cpp
void SokobanContext::update()
{
  if (_is_game_started)
  {
    // 1. Перевіряємо, чи гравець не завершив гру (вихід у меню рівнів)
    if (!_scene->isFinished())
    {
      // 2. Перевіряємо, чи не потрібно змінити поточний ігровий рівень
      if (!_scene->isReleased())
      {
        _scene->update(); // Звичайне покадрове оновлення сцени
      }
      else
      {
        // Сцена запросила перехід на наступний рівень
        uint8_t next_lvl = _scene->getNextSceneID();
        delete _scene;
        _scene = new SokobanScene(_stored_objs, false, next_lvl);
        _is_game_started = true;
      }
    }
    else
    {
      // Гру завершено — видаляємо сцену та відновлюємо GUI-меню вибору рівнів
      delete _scene;
      _scene = nullptr;
      _is_game_started = false;
      showLvlMenu();
    }
  }
  else
  {
    // Режим звичайного GUI-меню вибору рівня (коли сцена ще не запущена)
    if (_input.isReleased(BtnID::BTN_BACK))
    {
      openContext(new GameListContext());
    }
    else if (_input.isHolded(BtnID::BTN_UP))
    {
      _lvl_menu->focusUp();
      _scrollbar->scrollUp();
    }
    else if (_input.isHolded(BtnID::BTN_DOWN))
    {
      _lvl_menu->focusDown();
      _scrollbar->scrollDown();
    }
    else if (_input.isReleased(BtnID::BTN_OK))
    {
      // Користувач обрав рівень — видаляємо віджети меню і запускаємо сцену
      _prev_lvl = _lvl_menu->getCurrItemID();
      getLayout()->delWidgets();
      _scene = new SokobanScene(_stored_objs, false, _prev_lvl);
      _is_game_started = true;
    }
  }
}
```

<br>

::: {tip}
Під час створення ігрового контексту рекомендується підвищувати частоту процесора до максимальної (`setCpuFrequency(FREQ_MAX)`), а при поверненні у звичайне меню прошивки — знижувати її (`setCpuFrequency(FREQ_MIN)`) для економії заряду акумулятора.
:::

```{toctree}
:maxdepth: 1
:hidden:

engine/scene.md
engine/terrain.md
engine/object.md
engine/sfx.md
engine/multiplayer.md
```

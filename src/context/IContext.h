/**
 * @file IContext.h
 * @brief Головний абстрактий клас, від якого повинні бути успадковані всі класи контексту
 * @details Містить базовий функціонал та поля, що є спільними для всіх класів контексту.
 * Викликає віртуальні методи loop та update у його нащадків.
 * Керує відображенням Notification та Toast.
 * Викликає малювання віджетів на Canvas кожен кадр.
 * Викликає оновлення стану користувацького вводу кожен кадр.
 */

#pragma once
#pragma GCC optimize("O3")

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <functional>

#include "../defines.h"
#include "../driver/graphics/DisplayWrapper.h"
#include "../driver/input/Input.h"
#include "../widget/IWidgetContainer.h"
#include "../widget/notification/Notification.h"
#include "../widget/text/Label.h"
#include "cpu_config.h"

namespace pixeler
{
#define TOAST_LENGTH_LONG 3500
#define TOAST_LENGTH_SHORT 1500

  class IContext
  {
  public:
    IContext();
    virtual ~IContext() = 0;
    IContext(const IContext& rhs) = delete;
    IContext& operator=(const IContext& rhs) = delete;

#ifdef GRAPHICS_ENABLED
    /**
     * @brief Додає задачу до черги виконання, яка буде викликана
     * в потоці контексту під час наступного tick().
     * Може викликатись з будь-якої FreeRTOS-задачі.
     *
     * @param task Функція без аргументів і повернення результату,
     * яка повинна бути виконана в потоці UI
     * @param timeout_ms Максимальний час очікування(мілісекунд) вільного місця в черзі,
     * за замовчуванням - неблокуючий виклик
     * @return true - якщо задачу успішно додано в чергу
     * @return false - якщо черга переповнена і час очікування вичерпано
     */
    bool post(std::function<void()> task, unsigned long timeout_ms = 0);

#endif  // #ifdef GRAPHICS_ENABLED

    /**
     * @brief Метод, що викликається для контексту кожен доступний тік.
     * Виклик методу мусить виконувати об'єкт, що керує цим контекстом.
     */
    void tick();

    /**
     * @brief Віддає вказівник на об'єкт контексту, який повинен викликатися наступним.
     *
     * @return IContext*
     */
    IContext* takeNextContext();

    /**
     * @brief Повертає значення прапора, який вказує на те, чи повинен бути звільнений цей контекст.
     *
     * @return true - якщо контекст повинен бути звільнений. false - якщо контекст повинен бути активним
     */
    bool isReleased() const;

  protected:
    enum CpuFrequency : uint16_t
    {
      FREQ_MIN = BASE_CPU_FREQ_MHZ,
      FREQ_BALANCED = BALANCED_CPU_FREQ_MHZ,
      FREQ_MAX = MAX_CPU_FREQ_MHZ
    };

  protected:
    /**
     * @brief Встановлює робочу частоту ядер МК.
     * Рекомендується встановлювати мінімально можливу робочу частоту в кожному контексті,
     * для економії енергії акумулятора.
     *
     * @param frequency Може мати значення FREQ_MIN, FREQ_BALANCED, FREQ_MAX
     */
    void setCpuFrequency(CpuFrequency frequency);

    /**
     * @brief Прапор, який дозволяє повністю вимкнути відрисовку GUI.
     * true - віджети будуть відмальовуватися. false - перерисовка віджетів буде пропущена
     *
     */
    bool _gui_enabled{true};

    /**
     * @brief Викликається кожен кадр після оновлення стану кнопок, та перед формуванням буферу зображення.
     * Повинен бути обов'язково реалізований в кожному контексті.
     *
     */
    virtual void update() = 0;

    /**
     * @brief Викликається з максимальною частотою, яка доступна для поточного контексту, без прив'язки до GUI.
     * Повинен бути обов'язково реалізований в кожному контексті.
     *
     * @return true - Якщо контекст контролює ввід та малювання. false - Інакше
     */
    virtual bool loop() = 0;

    /**
     * @brief Встановлює стан поточного контексту в такий, що повинен бути звільнений наступного тіку.
     * Також встановлює об'єкт контексту, в який повинен виконатися перехід.
     *
     * @param context Адреса об'єкта контексту, який створено оператором new, або nullptr.
     * Переданий nullptr з субконтексту означатиме вихід з цього режима.
     * Переданий nullptr в контексті першого рівня призведе до перезавантаження прошивки
     *
     */
    void openContext(IContext* context);

    /**
     * @brief Встановлює стан поточного контексту в такий, що повинен бути звільнений наступного тіку.
     * Виклик цього методу в контексті першого рівня призведе до перезавантаження прошивки.
     *
     */
    void releaseContext();

#ifdef GRAPHICS_ENABLED

    /**
     * @brief Встановлює віджет, який буде слугувати макетом GUI для поточного контексту. Віджет буде автоматично видалений разом з контекстом.
     *
     * @param layout Вказівник на віджет макету
     */
    void setLayout(IWidgetContainer* layout);

    /**
     * @brief Повертає вказівник на поточний віджет макету контексту.
     *
     * @return IWidgetContainer*
     */
    IWidgetContainer* getLayout() const;

    /**
     * @brief Виводить коротке повідомлення-підказку в межах поточного контексту.
     * Повідомлення буде автоматично видалене, після спливання вказаного часу або в разі припиннення існування контексту, в якому воно було створене.
     *
     * @param msg_txt Текст повідомлення
     * @param duration Тривалість відображення повідомлення
     */
    void showToast(const char* msg_txt, unsigned long duration = TOAST_LENGTH_SHORT);

    /**
     * @brief Повертає х-координату, на якій віджет буде встановлено по центру відносно екрану.
     *
     * @param widget Вказівник на віджет
     * @return uint16_t
     */
    uint16_t getCenterX(const IWidget* widget) const;

    /**
     * @brief Повертає y-координату, на якій віджет буде встановлено по центру відносно екрану.
     *
     * @param widget Вказівник на віджет
     * @return uint16_t
     */
    uint16_t getCenterY(const IWidget* widget) const;

    /**
     * @brief Відображає віджет Notification для поточного макету.
     *
     * @param notification Вказівник на віджет
     */
    void showNotification(Notification* notification);

    /**
     * @brief Прибирає віджет Notification з відображення у макеті.
     * Пам'ять, яку займає об'єкт віджета не буде звільнено автоматично.
     *
     */
    void hideNotification();

    /**
     * @brief Віддає м'ютекс шаблону тій задачі, яка викликає цей метод.
     *
     * @return true - Якщо мютекс отримано. false - Інакше
     */
    bool takeLayoutMutex() const;

    /**
     * @brief Отримує назад м'ютекс шаблону з тієї задачі, яка викликає цей метод.
     *
     */
    void giveLayoutMutex() const;

  private:
    void removeToast();
    void processPostedTasks();

#endif  // #ifdef GRAPHICS_ENABLED

  private:
    IContext* _next_context{nullptr};

#ifdef GRAPHICS_ENABLED
    TaskHandle_t _owner_task_handle{nullptr};
    QueueHandle_t _task_queue{nullptr};
    mutable SemaphoreHandle_t _layout_mutex{nullptr};
    IWidgetContainer* _layout{nullptr};
    Label* _toast_label{nullptr};
    Notification* _notification{nullptr};
    //
    unsigned long _toast_lifetime{0};
    unsigned long _toast_birthtime{0};
    static constexpr size_t UI_TASK_QUEUE_DEPTH = 10;
#endif  // #ifdef GRAPHICS_ENABLED
    unsigned long _upd_time{0};

    bool _is_released{false};
    bool _is_alive{true};
  };

}  // namespace pixeler

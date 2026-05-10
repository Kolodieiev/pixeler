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

#include "../defines.h"
#include "../driver/graphics/DisplayWrapper.h"
#include "../driver/input/Input.h"
#include "../widget/IWidgetContainer.h"
#include "../widget/notification/Notification.h"
#include "../widget/text/Label.h"
#include "context_id_config.h"
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

    /**
     * @brief Метод, що викликається для контексту кожен доступний тік.
     * Виклик методу мусить виконувати об'єкт, що керує цим контекстом.
     */
    void tick();

    /**
     * @brief Повертає ідентифікатор контексту, який було встановлено методом openContextByID.
     *
     * @return ContextID - унікальний ідентифікатор дисплея.
     */
    ContextID getNextContextID() const;

    /**
     * @brief Повертає значення прапору, який вказує на те, чи повинен бути звільнений цей контекст.
     *
     * @return true - якщо контекст повинен бути звільнений.
     * @return false - якщо контекст повинен бути активним.
     */
    bool isReleased() const;

  protected:
    /**
     * @brief Прапор, який дозволяє повністю вимкнути відрисовку GUI.
     * true - віджети будуть відмальовуватися. false - перерисовка віджетів буде пропущена.
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
     * @return true - Якщо контекст контролює ввід та малювання.
     * @return false - Інакше.
     */
    virtual bool loop() = 0;

    /**
     * @brief Встановлює стан поточного контексту в такий, що повинен бути звільнений.
     * Також встановлює ідентифікатор контексту, в який повинен виконатися перехід.
     *
     * @param context_ID
     */
    void openContextByID(ContextID context_ID);

    /**
     * @brief Встановлює стан поточного контексту в такий, що повинен бути звільнений.
     *
     */
    void release();

#ifdef GRAPHICS_ENABLED

    /**
     * @brief Встановлює віджет, який буде слугувати макетом GUI для поточного контексту. Віджет буде автоматично видалений разом з контекстом.
     *
     * @param layout Вказівник на віджет макету.
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
     * @param msg_txt Текст повідомлення.
     * @param duration Тривалість відображення повідомлення.
     */
    void showToast(const char* msg_txt, unsigned long duration = TOAST_LENGTH_SHORT);

    /**
     * @brief Повертає х-координату, на якій віджет буде встановлено по центру відносно екрану.
     *
     * @param widget Вказівник на віджет.
     * @return uint16_t
     */
    uint16_t getCenterX(const IWidget* widget) const;

    /**
     * @brief Повертає y-координату, на якій віджет буде встановлено по центру відносно екрану.
     *
     * @param widget Вказівник на віджет.
     * @return uint16_t
     */
    uint16_t getCenterY(const IWidget* widget) const;

    /**
     * @brief Відображає віджет Notification для поточного макету.
     *
     * @param notification Вказівник на віджет.
     */
    void showNotification(Notification* notification);

    /**
     * @brief Прибирає віджет Notification з відображення у макеті.
     * Пам'ять, яку займає об'єкт віджета не буде звільнено.
     *
     */
    void hideNotification();

    /**
     * @brief Віддає м'ютекс шаблону тій задачі, яка викликає цей метод.
     *
     * @return true - Якщо мютекс отримано.
     * @return false - Інакше.
     */
    bool takeLayoutMutex();

    /**
     * @brief Отримує назад м'ютекс шаблону з тієї задачі, яка викликає цей метод.
     *
     */
    void giveLayoutMutex();

  private:
    void removeToast();

#endif  // #ifdef GRAPHICS_ENABLED

  private:
#ifdef GRAPHICS_ENABLED
    SemaphoreHandle_t _layout_mutex{nullptr};

    IWidgetContainer* _layout{nullptr};
    Label* _toast_label{nullptr};
    Notification* _notification{nullptr};
    //
    unsigned long _toast_lifetime{0};
    unsigned long _toast_birthtime{0};
#endif  // #ifdef GRAPHICS_ENABLED
    unsigned long _upd_time{0};

    ContextID _next_context_ID{0};

    bool _is_released{false};
  };

}  // namespace pixeler
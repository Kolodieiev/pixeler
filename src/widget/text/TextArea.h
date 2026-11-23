/**
 * @file TextArea.h
 * @brief Багаторядковий віджет для відображення та редагування тексту з курсором, виділенням, undo та нумерацією рядків
 * @details Зберігає текст у сирому буфері (Gap Buffer) з підтримкою алокації в PSRAM.
 * Підтримує кодування UTF-8, перенесення рядків, переміщення курсора кнопками та дотиком,
 * посимвольне виділення в обидва напрямки, внутрішній буфер обміну, дублювання виділення/рядка,
 * багатоступеневий дельта-undo, нумерацію рядків та hint-підказку.
 */
#pragma once
#pragma GCC optimize("O3")

#include "../IWidget.h"

namespace pixeler
{
  class TextArea : public IWidget
  {
  public:
    explicit TextArea(uint16_t widget_ID, TypeID type_ID = TYPE_TEXTAREA);
    virtual ~TextArea() override;

    TextArea(const TextArea&) = delete;
    TextArea& operator=(const TextArea&) = delete;

    /**
     * @brief Викликає процедуру малювання віджета на дисплей.
     * Якщо віджет не було змінено, виконує лише локальне блимання або переміщення курсора.
     */
    virtual void onDraw() override;

    /**
     * @brief Повертає вказівник на глибоку копію віджета.
     *
     * @param id Ідентифікатор, який буде присвоєно новому віджету.
     * @return TextArea*
     */
    virtual TextArea* clone(uint16_t id) const override;

    /**
     * @brief Повертає ідентифікатор типу.
     * Використовується в системі приведення типу.
     *
     * @return constexpr TypeID
     */
    static constexpr TypeID getTypeID()
    {
      return TypeID::TYPE_TEXTAREA;
    }

    /**
     * @brief Вставляє текст із внутрішнього буфера обміну зліва від курсора
     * (замінюючи виділення, якщо воно активне).
     *
     * @return true - Якщо текст успішно вставлено.
     * @return false - Якщо внутрішній буфер обміну порожній або не вистачило пам'яті.
     */
    bool paste();

    /**
     * @brief Вставляє C-рядок зліва від курсора (замінюючи виділення, якщо воно активне).
     *
     * @param text Нуль-термінований рядок UTF-8.
     * @return true - Якщо текст успішно вставлено.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool paste(const char* text);

    /**
     * @brief Вставляє послідовність байтів вказаної довжини зліва від курсора.
     *
     * @param text Вказівник на буфер UTF-8.
     * @param byte_len Кількість байтів для вставки.
     * @return true - Якщо текст успішно вставлено.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool paste(const char* text, size_t byte_len);

    /**
     * @brief Вставляє рядок String зліва від курсора (замінюючи виділення, якщо воно активне).
     *
     * @param text Рядок UTF-8.
     * @return true - Якщо текст успішно вставлено.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool paste(const String& text);

    /**
     * @brief Вставляє один ASCII-символ зліва від курсора.
     *
     * @param ch Символ для вставки.
     * @return true - Якщо символ успішно додано.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool addChar(char ch);

    /**
     * @brief Вставляє UTF-8 символ або коротку послідовність символів зліва від курсора.
     *
     * @param utf8_ch Нуль-термінований UTF-8 символ.
     * @return true - Якщо символ успішно додано.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool addChar(const char* utf8_ch);

    /**
     * @brief Видаляє виділений текст або один UTF-8 символ зліва від курсора.
     *
     * @return true - Якщо текст було змінено.
     * @return false - Якщо видаляти нічого.
     */
    bool rmChar();

    /**
     * @brief Видаляє виділений текст або один UTF-8 символ справа від курсора.
     *
     * @return true - Якщо текст було змінено.
     * @return false - Якщо видаляти нічого.
     */
    bool delChar();

    /**
     * @brief Повністю очищує текст у віджеті та скидає позицію курсора.
     */
    void clearText();

    /**
     * @brief Скасовує останню зміну тексту та відновлює позицію курсора і стан виділення.
     *
     * @return true - Якщо дію успішно скасовано.
     * @return false - Якщо історія змін порожня або не вистачило пам'яті для відновлення.
     */
    bool undo();

    /**
     * @brief Перевіряє, чи є в історії доступні кроки для скасування (undo).
     *
     * @return true - Якщо можна виконати undo.
     * @return false - Інакше.
     */
    bool canUndo() const;

    /**
     * @brief Очищує історію undo та звільняє зайняту нею пам'ять.
     */
    void clearUndoHistory();

    /**
     * @brief Встановлює максимальну кількість кроків в історії undo.
     * Очищує поточну історію змін. Значення 0 вимикає збереження історії undo.
     *
     * @param steps Максимальна кількість кроків.
     */
    void setMaxUndoSteps(uint8_t steps);

    /**
     * @brief Повертає максимальну кількість кроків історії undo.
     *
     * @return uint8_t
     */
    uint8_t getMaxUndoSteps() const;

    /**
     * @brief Встановлює ліміт пам'яті (в байтах) для збереження видалених фрагментів в історії undo.
     *
     * @param max_bytes Максимальний розмір буферів історії undo в байтах.
     */
    void setUndoMemLimit(size_t max_bytes);

    /**
     * @brief Повертає ліміт пам'яті для історії undo в байтах.
     *
     * @return size_t
     */
    size_t getUndoMemLimit() const;

    /**
     * @brief Повертає нуль-термінований вказівник на весь текст у буфері.
     * Вказівник дійсний до наступної модифікації тексту віджета.
     *
     * @return const char*
     */
    const char* getText() const;

    /**
     * @brief Повертає довжину тексту в байтах (без урахування нуль-термінатора).
     *
     * @return size_t
     */
    size_t getByteLen() const;

    /**
     * @brief Повертає кількість UTF-8 символів у тексті.
     *
     * @return uint32_t
     */
    uint32_t getCharLen() const;

    /**
     * @brief Повертає загальну кількість логічних рядків у тексті (розділених '\\n').
     *
     * @return uint32_t
     */
    uint32_t getTotalLines() const;

    /**
     * @brief Перевіряє, чи порожній текст у віджеті.
     *
     * @return true - Якщо текст відсутній.
     * @return false - Інакше.
     */
    bool isEmpty() const;

    /**
     * @brief Переміщує курсор на один UTF-8 символ вліво.
     */
    void moveCursorLeft();

    /**
     * @brief Переміщує курсор на один UTF-8 символ вправо.
     */
    void moveCursorRight();

    /**
     * @brief Переміщує курсор на один візуальний рядок вгору або на початок тексту, якщо рядка вище не існує.
     */
    void moveCursorUp();

    /**
     * @brief Переміщує курсор на один візуальний рядок вниз або в кінець тексту, якщо рядка нижче не існує.
     */
    void moveCursorDown();

    /**
     * @brief Встановлює позицію курсора за абсолютними координатами на дисплеї.
     * Якщо точка нижче або правіше кінця тексту, встановлює курсор у кінець тексту.
     * У режимі виділення автоматично оновлює виділений діапазон.
     *
     * @param x Абсолютна X-координата точки на дисплеї.
     * @param y Абсолютна Y-координата точки на дисплеї.
     */
    void setCursorByPos(uint16_t x, uint16_t y);

    /**
     * @brief Переміщує курсор на початок тексту.
     */
    void setCursorToStart();

    /**
     * @brief Переміщує курсор у кінець тексту.
     */
    void setCursorToEnd();

    /**
     * @brief Повертає поточну позицію курсора в байтах від початку тексту.
     *
     * @return size_t
     */
    size_t getCursorBytePos() const;

    /**
     * @brief Вмикає або вимикає режим посимвольного виділення тексту.
     * При ввімкненні фіксує поточну позицію курсора як початок виділення.
     *
     * @param state true - увімкнути режим виділення, false - вимкнути та зняти виділення.
     */
    void setSelectionMode(bool state);

    /**
     * @brief Повертає стан режиму виділення тексту.
     *
     * @return true - Якщо режим виділення активний.
     * @return false - Інакше.
     */
    bool isSelectionMode() const;

    /**
     * @brief Перевіряє, чи є зараз непорожній виділений фрагмент тексту.
     *
     * @return true - Якщо виділено хоча б один символ.
     * @return false - Інакше.
     */
    bool hasSelection() const;

    /**
     * @brief Знімає виділення та виходить із режиму виділення.
     */
    void clearSelection();

    /**
     * @brief Копіює виділений фрагмент тексту у внутрішній буфер обміну віджета (PSRAM/RAM).
     * Виділення при цьому залишається активним.
     *
     * @return true - Якщо виділений фрагмент успішно скопійовано.
     * @return false - Якщо виділення порожнє або не вдалося виділити пам'ять.
     */
    bool copySelected();

    /**
     * @brief Копіює виділений фрагмент у внутрішній буфер обміну та видаляє його з тексту.
     * У разі помилки виділення пам'яті текст не видаляється.
     *
     * @return true - Якщо фрагмент успішно вирізано.
     * @return false - Якщо виділення порожнє або не вдалося виділити пам'ять.
     */
    bool cutSelected();

    /**
     * @brief Повертає копію виділеного тексту у вигляді String.
     *
     * @return String - Виділений фрагмент або порожній рядок, якщо виділення відсутнє або не вистачило пам'яті.
     */
    String getSelectedText() const;

    /**
     * @brief Перевіряє, чи містить внутрішній буфер обміну скопійований текст.
     *
     * @return true - Якщо буфер обміну не порожній.
     * @return false - Інакше.
     */
    bool hasClipboard() const;

    /**
     * @brief Очищує внутрішній буфер обміну та звільняє зайняту ним пам'ять.
     */
    void clearClipboard();

    /**
     * @brief Видаляє виділений фрагмент тексту та знімає виділення.
     *
     * @return true - Якщо виділений текст було видалено.
     * @return false - Якщо виділення було порожнім.
     */
    bool deleteSelected();

    /**
     * @brief Дублює виділений фрагмент тексту одразу після виділення.
     *
     * @return true - Якщо фрагмент успішно продубльовано.
     * @return false - Якщо виділення порожнє або виникла помилка виділення пам'яті.
     */
    bool duplicateSelected();

    /**
     * @brief Дублює логічний рядок (від '\\n' до '\\n'), у якому знаходиться курсор.
     *
     * @return true - Якщо рядок успішно продубльовано.
     * @return false - Якщо виникла помилка виділення пам'яті.
     */
    bool duplicateLine();

    /**
     * @brief Копіює поточний логічний рядок (від '\\n' до '\\n') у внутрішній буфер обміну та видаляє його з тексту.
     * У разі помилки виділення пам'яті текст не видаляється.
     *
     * @return true - Якщо рядок успішно вирізано.
     * @return false - Якщо текст порожній або не вдалося виділити пам'ять.
     */
    bool cutLine();

    /**
     * @brief Вмикає або вимикає відображення колонки нумерації рядків зліва від тексту.
     *
     * @param state true - показувати номери рядків, false - приховати.
     */
    void setShowLineNumbers(bool state);

    /**
     * @brief Повертає стан відображення колонки нумерації рядків.
     *
     * @return true - Якщо нумерація рядків увімкнена.
     * @return false - Інакше.
     */
    bool hasLineNumbers() const;

    /**
     * @brief Встановлює колір фону колонки нумерації рядків.
     *
     * @param color Колір у форматі RGB565.
     */
    void setLineNumBackColor(uint16_t color);

    /**
     * @brief Повертає колір фону колонки нумерації рядків.
     *
     * @return uint16_t
     */
    uint16_t getLineNumBackColor() const;

    /**
     * @brief Встановлює колір тексту номерів рядків.
     *
     * @param color Колір у форматі RGB565.
     */
    void setLineNumTextColor(uint16_t color);

    /**
     * @brief Повертає колір тексту номерів рядків.
     *
     * @return uint16_t
     */
    uint16_t getLineNumTextColor() const;

    /**
     * @brief Встановлює колір вертикальної лінії-розділювача колонки нумерації рядків.
     *
     * @param color Колір у форматі RGB565.
     */
    void setLineNumBorderColor(uint16_t color);

    /**
     * @brief Повертає колір вертикальної лінії-розділювача колонки нумерації рядків.
     *
     * @return uint16_t
     */
    uint16_t getLineNumBorderColor() const;

    /**
     * @brief Встановлює текст підказки (hint), яка відображається коли поле порожнє.
     *
     * @param hint Нуль-термінований рядок UTF-8.
     */
    void setHint(const char* hint);

    /**
     * @brief Встановлює текст підказки (hint), яка відображається коли поле порожнє.
     *
     * @param hint Рядок UTF-8.
     */
    void setHint(const String& hint);

    /**
     * @brief Повертає поточний текст підказки (hint).
     *
     * @return const String&
     */
    const String& getHint() const;

    /**
     * @brief Встановлює колір тексту підказки (hint).
     *
     * @param color Колір у форматі RGB565.
     */
    void setHintColor(uint16_t color);

    /**
     * @brief Повертає колір тексту підказки (hint).
     *
     * @return uint16_t
     */
    uint16_t getHintColor() const;

    /**
     * @brief Встановлює шрифт для відображення тексту, номерів рядків та підказки.
     *
     * @param font Вказівник на шрифт.
     */
    void setFont(const uint8_t* font);

    /**
     * @brief Повертає вказівник на поточний шрифт.
     *
     * @return const uint8_t*
     */
    const uint8_t* getFont() const;

    /**
     * @brief Встановлює множник розміру тексту.
     *
     * @param size Розмір тексту (від 1 до 7).
     */
    void setTextSize(uint8_t size);

    /**
     * @brief Повертає поточний множник розміру тексту.
     *
     * @return uint8_t
     */
    uint8_t getTextSize() const;

    /**
     * @brief Встановлює колір основного тексту.
     *
     * @param color Колір у форматі RGB565.
     */
    void setTextColor(uint16_t color);

    /**
     * @brief Повертає колір основного тексту.
     *
     * @return uint16_t
     */
    uint16_t getTextColor() const;

    /**
     * @brief Встановлює колір курсора.
     *
     * @param color Колір у форматі RGB565.
     */
    void setCursorColor(uint16_t color);

    /**
     * @brief Повертає колір курсора.
     *
     * @return uint16_t
     */
    uint16_t getCursorColor() const;

    /**
     * @brief Встановлює інтервал блимання курсора в мілісекундах.
     *
     * @param delay_ms Період перемикання видимості курсора в мс.
     */
    void setCursorBlinkDelay(uint16_t delay_ms);

    /**
     * @brief Повертає інтервал блимання курсора в мілісекундах.
     *
     * @return uint16_t
     */
    uint16_t getCursorBlinkDelay() const;

    /**
     * @brief Встановлює колір фону виділеного тексту.
     *
     * @param color Колір у форматі RGB565.
     */
    void setSelectionBackColor(uint16_t color);

    /**
     * @brief Повертає колір фону виділеного тексту.
     *
     * @return uint16_t
     */
    uint16_t getSelectionBackColor() const;

    /**
     * @brief Встановлює колір символів у виділеному тексті.
     *
     * @param color Колір у форматі RGB565.
     */
    void setSelectionTextColor(uint16_t color);

    /**
     * @brief Повертає колір символів у виділеному тексті.
     *
     * @return uint16_t
     */
    uint16_t getSelectionTextColor() const;

    /**
     * @brief Встановлює горизонтальний внутрішній відступ тексту в пікселях.
     *
     * @param padding Відступ зліва та справа в пікселях.
     */
    void setHPadding(uint8_t padding);

    /**
     * @brief Встановлює вертикальний внутрішній відступ тексту в пікселях.
     *
     * @param padding Відступ зверху та знизу в пікселях.
     */
    void setVPadding(uint8_t padding);

    /**
     * @brief Повертає висоту символу поточного шрифту в пікселях.
     *
     * @return uint16_t
     */
    uint16_t getCharHgt() const;

    /**
     * @brief Повертає крок між рядками в пікселях.
     *
     * @return uint16_t
     */
    uint16_t getLineHeight() const;

    /**
     * @brief Повертає кількість візуальних рядків, які одночасно вміщуються у віджеті.
     *
     * @return uint16_t
     */
    uint16_t getVisibleLines() const;

  protected:
    /**
     * @brief Копіює поля до іншого віджета.
     *
     * @param widget Цільовий віджет.
     */
    virtual void copyTo(IWidget* widget) const override;

  private:
    struct VisualLine
    {
      size_t start;
      size_t end;
      bool ends_with_nl;
    };

    struct ContentRect
    {
      uint16_t x;
      uint16_t y;
      uint16_t w;
      uint16_t h;
    };

    struct UndoStep
    {
      char* removed_buf;
      size_t pos;
      size_t removed_len;
      size_t inserted_len;
      size_t prev_cursor;
      size_t prev_sel_anchor;
      uint32_t prev_char_len;
      uint32_t prev_total_lines;
      bool prev_is_selecting;
      bool is_insert_group;
    };

    char byteAt(size_t pos) const;
    void copyRange(size_t from, size_t len, char* dst) const;
    void moveGap(size_t pos) const;
    bool ensureGap(size_t needed);
    uint8_t utf8LenAt(size_t pos) const;
    size_t prevUtf8Pos(size_t pos) const;
    uint32_t decodeUtf8At(size_t pos, uint8_t& out_len) const;
    uint32_t countNewlines(size_t from, size_t to) const;

    bool ensureUndoBuf();
    void freeUndoStep(UndoStep& step);
    bool evictOldestUndo();
    void breakUndoGroup();
    void recordUndoStep(size_t pos, size_t removed_len, size_t inserted_len, size_t prev_cursor, size_t prev_sel_anchor, bool prev_is_selecting, uint32_t prev_char_len, uint32_t prev_total_lines, bool can_coalesce);
    bool pasteInternal(const char* text, size_t byte_len, bool can_coalesce);

    void updateFontMetrics();
    void invalidateGlyphCache();
    void syncLayoutMetrics();
    uint16_t calcGutterWidth() const;
    ContentRect getContentRect() const;
    uint8_t getCharWidth(uint32_t cp, const char* utf8_bytes, uint8_t utf8_len) const;
    uint8_t getCharWidthAt(size_t pos, uint8_t& out_len) const;
    uint16_t measureRangeWidth(size_t from, size_t to) const;

    void calcVisualLine(size_t start_byte, uint16_t max_w, VisualLine& out) const;
    bool findPrevVisualLine(size_t target_line_start, uint16_t max_w, VisualLine& out) const;
    void findVisualLineAtPos(size_t pos, uint16_t max_w, VisualLine& out) const;
    bool lineOwnsCursor(const VisualLine& line, size_t pos) const;
    size_t lineCursorEnd(const VisualLine& line) const;
    size_t closestPosAtX(const VisualLine& line, uint16_t target_x) const;

    void setViewTop(size_t new_top);
    void recalcViewTopLineNum();
    void getSelectionRange(size_t& out_start, size_t& out_end) const;
    bool ensureCursorVisible();
    void syncViewportAfterEdit(bool force_recalc_top_line);
    void updateCursorScreenCoords();
    void finishCursorMove(bool scrolled);

    void drawLineNumber(uint32_t line_num, uint16_t gutter_x, uint16_t gutter_w, uint16_t rect_y, uint16_t rect_h, uint16_t line_top_y) const;
    void drawTextSlice(size_t from, size_t to) const;
    void drawVisualLine(const VisualLine& line, const ContentRect& rect, uint16_t line_top_y, size_t sel_start, size_t sel_end) const;
    void drawCursorAt(int16_t x, int16_t y, size_t pos);
    void eraseDrawnCursor();

  private:
    String _hint;

    char* _buf{nullptr};
    char* _clip_buf{nullptr};
    UndoStep* _undo_buf{nullptr};
    const uint8_t* _font{font_unifont};

    size_t _buf_cap{0u};
    mutable size_t _gap_start{0u};
    mutable size_t _gap_end{0u};
    size_t _clip_len{0u};
    size_t _cursor_pos{0u};
    size_t _sel_anchor{0u};
    size_t _view_top_byte{0u};
    size_t _drawn_cursor_pos{0u};
    size_t _undo_used_bytes{0u};
    size_t _undo_mem_limit{131072u};
    unsigned long _last_blink_ts{0ul};

    uint32_t _char_len{0u};
    uint32_t _total_lines{1u};
    uint32_t _view_top_line_num{1u};
    mutable uint32_t _utf8_cp[64]{};

    uint16_t _text_color{COLOR_BLACK};
    uint16_t _cursor_color{COLOR_BLACK};
    uint16_t _hint_color{0x8410u};
    uint16_t _sel_back_color{0x033Fu};
    uint16_t _sel_text_color{COLOR_WHITE};
    uint16_t _line_num_back_color{0xEF7Du};
    uint16_t _line_num_text_color{0x7BEFu};
    uint16_t _line_num_border_color{0xC618u};
    uint16_t _cursor_blink_delay{500u};
    uint16_t _char_hgt{0u};
    uint16_t _line_step{0u};
    uint16_t _ref_pair_w{0u};
    uint16_t _cached_width{0u};
    uint16_t _cached_height{0u};
    uint16_t _cached_gutter_w{0u};
    uint16_t _visible_lines{1u};

    int16_t _y_char_offset{0};
    int16_t _preferred_x{-1};
    int16_t _cursor_x{0};
    int16_t _cursor_y{0};
    int16_t _drawn_cursor_x{-1};
    int16_t _drawn_cursor_y{-1};

    mutable uint8_t _ascii_w[96]{};
    mutable uint8_t _utf8_w[64]{};
    uint8_t _digit_w{8u};
    uint8_t _text_size{1u};
    uint8_t _h_padding{2u};
    uint8_t _v_padding{2u};
    uint8_t _undo_cap{16u};
    uint8_t _undo_head{0u};
    uint8_t _undo_count{0u};

    bool _show_line_numbers{false};
    bool _is_selecting{false};
    bool _cursor_visible{true};
    bool _is_cursor_drawn{false};
    bool _is_cursor_dirty{false};
  };
}  // namespace pixeler

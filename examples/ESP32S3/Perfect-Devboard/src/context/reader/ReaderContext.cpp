#pragma GCC optimize("O3")
#include "ReaderContext.h"

#include "../WidgetCreator.h"

static const char STR_BOOK_DIR_PREF[] = "book_dir";
static const char STR_BOOK_NAME_PREF[] = "book_name";
static const char STR_READ_BOOK_PAGE[] = "book_page";
static const char STR_BOOK_BRIGHT_PREF[] = "book_bright";

#define BOOK_DIR_ITEMS_NUM 7
#define BOOKS_ITEMS_NUM 7

const char ROOT_PATH[] = "/books";
const char BOOK_EXT[] = ".txt";

bool ReaderContext::loop()
{
  return true;
}

void ReaderContext::savePref()
{
  SettingsManager::set(STR_BOOK_DIR_PREF, _dirname.c_str());
  SettingsManager::set(STR_BOOK_NAME_PREF, _book_name.c_str());
  SettingsManager::set(STR_READ_BOOK_PAGE, String(_cur_book_page).c_str());
  SettingsManager::set(STR_BOOK_BRIGHT_PREF, String(_brightness).c_str());
}

//-------------------------------------------------------------------------------------------

void ReaderContext::showContextMenuTmpl()
{
  IWidgetContainer* layout = getLayout();
  layout->disable();

  _books_list_menu->disable();

  _context_menu = new FixedMenu(ID_BOOK_MENU);
  layout->addWidget(_context_menu);
  _context_menu->setBackColor(COLOR_MENU_ITEM);
  _context_menu->setBorderColor(COLOR_ORANGE);
  _context_menu->setBorder(true);
  _context_menu->setItemHeight(20);
  _context_menu->setWidth(120);
  _context_menu->setHeight(44);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight());

  if (_books_list_menu->getCurrItemID() != 0)
  {
    MenuItem* del_item = WidgetCreator::getMenuItem(ID_ITEM_DEL);
    _context_menu->addItem(del_item);

    Label* upd_lbl = WidgetCreator::getItemLabel(STR_DELETE);
    del_item->setLbl(upd_lbl);
  }

  _mode = MODE_CONTEXT_MENU;
  layout->enable();
}

void ReaderContext::hideContextMenu()
{
  getLayout()->delWidgetByID(ID_BOOK_MENU);
  _books_list_menu->enable();
  _mode = MODE_BOOK_SEL;
}

void ReaderContext::showBookDirsTmpl()
{
  _mode = MODE_BOOK_DIR_SEL;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _book_dirs_menu = new FixedMenu(ID_F_MENU);
  layout->addWidget(_book_dirs_menu);
  _book_dirs_menu->setBackColor(COLOR_MENU_ITEM);
  _book_dirs_menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH);
  _book_dirs_menu->setHeight(UI_HEIGHT);
  _book_dirs_menu->setItemHeight((_book_dirs_menu->getHeight()) / BOOK_DIR_ITEMS_NUM);

  _scrollbar = new ScrollBar(ID_SCROLL);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(UI_HEIGHT);
  _scrollbar->setPos(UI_WIDTH - SCROLLBAR_WIDTH, 0);

  if (!_book_name.isEmpty())
  {
    MenuItem* cont_item = WidgetCreator::getMenuItem(ID_CONT_ITEM);
    _book_dirs_menu->addItem(cont_item);

    Label* cont_lbl = WidgetCreator::getItemLabel(STR_CONTINUE, font_10x20);
    cont_item->setLbl(cont_lbl);
  }
}

void ReaderContext::fillBookDirs()
{
  std::vector<MenuItem*> items;
  makeBookDirsItems(items);

  uint16_t size = items.size();

  for (size_t i = 0; i < size; ++i)
    _book_dirs_menu->addItem(items[i]);

  _scrollbar->setValue(0);
  _scrollbar->setMax(_book_dirs_menu->getSize());
}

void ReaderContext::makeBookDirsItems(std::vector<MenuItem*>& items)
{
  items.clear();

  uint16_t books_num = _dirs.size();
  items.reserve(books_num);

  for (uint16_t i = 0, counter = ID_CONT_ITEM; i < books_num; ++i)
  {
    ++counter;
    MenuItem* item = WidgetCreator::getMenuItem(counter);
    items.push_back(item);

    Label* lbl = new Label(1);
    item->setLbl(lbl);
    lbl->setAutoscrollInFocus(true);
    lbl->setFont(font_10x20);
    lbl->setText(_dirs[i].getName());
  }
}

void ReaderContext::showBooksListTmpl()
{
  _mode = MODE_BOOK_SEL;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _books_list_menu = WidgetCreator::getDynamicMenu(ID_D_MENU);
  layout->addWidget(_books_list_menu);
  _books_list_menu->setWidth(UI_WIDTH - SCROLLBAR_WIDTH);
  _books_list_menu->setHeight(UI_HEIGHT);
  _books_list_menu->setItemHeight((_books_list_menu->getHeight() - 2) / BOOKS_ITEMS_NUM);

  _books_list_menu->setOnNextItemsLoadHandler(onNextItemsLoad, this);
  _books_list_menu->setOnPrevItemsLoadHandler(onPrevItemsLoad, this);

  _scrollbar = new ScrollBar(ID_SCROLL);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(UI_HEIGHT);
  _scrollbar->setPos(UI_WIDTH - SCROLLBAR_WIDTH, 0);
}

void ReaderContext::fillBooks(uint16_t pos)
{
  _books_list_menu->delWidgets();

  uint16_t pl_sz = _books.size();
  if (pl_sz > 0 && pos >= pl_sz)
    --pos;

  std::vector<MenuItem*> items;
  makeBooksItems(items, pos, _books_list_menu->getItemsPerPage());

  uint16_t size = items.size();

  for (size_t i = 0; i < size; ++i)
    _books_list_menu->addItem(items[i]);

  _scrollbar->setMax(pl_sz);
  _scrollbar->setValue(pos);
}

void ReaderContext::makeBooksItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size)
{
  if (file_pos >= _books.size())
    return;

  uint16_t read_to = file_pos + size;

  if (read_to > _books.size())
    read_to = _books.size();

  items.clear();
  items.reserve(read_to - file_pos);

  for (uint16_t i = file_pos; i < read_to; ++i)
  {
    ++file_pos;

    MenuItem* item = WidgetCreator::getMenuItem(file_pos);
    items.push_back(item);

    Label* lbl = new Label(1);
    item->setLbl(lbl);
    lbl->setAutoscrollInFocus(true);
    lbl->setFont(font_10x20);
    lbl->setText(_books[i].getName());
  }
}

void ReaderContext::showReadTmpl()
{
  _mode = MODE_BOOK_READ;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  layout->setBackColor(COLOR_DARKGREY);

  _progress_lbl = new Label(ID_PROGRESS_LBL);
  layout->addWidget(_progress_lbl);
  _progress_lbl->setText("0000/0000");
  _progress_lbl->setTextColor(COLOR_WHITE);
  _progress_lbl->setBackColor(COLOR_DARKGREY);
  _progress_lbl->initWidthToFit(5);
  _progress_lbl->setPos(0, UI_HEIGHT - _progress_lbl->getHeight());

  _page = new Label(ID_PAGE_LBL);
  layout->addWidget(_page);
  _page->setMultiline(true);
  _page->setWidth(UI_WIDTH);
  _page->setHeight(UI_HEIGHT - _progress_lbl->getHeight() - 5);
  _page->setBackColor(COLOR_DARKGREY);
  _page->setTextColor(COLOR_WHITE);

  _display.setBrightness(_brightness);
}

void ReaderContext::showSDErrTmpl()
{
  _mode = MODE_SD_UNCONN;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  layout->addWidget(WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_SD_ERR));
}

void ReaderContext::indexDirs()
{
  String dirs_path = ROOT_PATH;
  _fs.indexDirs(_dirs, dirs_path.c_str());
}

void ReaderContext::indexBooks()
{
  String books_path = ROOT_PATH;
  books_path += "/";
  books_path += _dirname;
  _fs.indexFilesByExt(_books, books_path.c_str(), {BOOK_EXT});
}

void ReaderContext::handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  if (!cur_id)
    return;

  makeBooksItems(items, cur_id, size);
}

void ReaderContext::handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  if (!cur_id)
    return;

  uint16_t item_pos = cur_id - 1;

  if (!item_pos)
    return;

  if (cur_id > size)
    item_pos = cur_id - size - 1;
  else
  {
    item_pos = 0;
    _scrollbar->setValue(cur_id);
  }

  makeBooksItems(items, item_pos, size);
}

void ReaderContext::onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  ReaderContext* self = static_cast<ReaderContext*>(arg);
  self->handleNextItemsLoad(items, size, cur_id);
}

void ReaderContext::onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  ReaderContext* self = static_cast<ReaderContext*>(arg);
  self->handlePrevItemsLoad(items, size, cur_id);
}

void ReaderContext::up()
{
  if (_mode == MODE_BOOK_DIR_SEL)
  {
    _book_dirs_menu->focusUp();
    _scrollbar->scrollUp();
  }
  else if (_mode == MODE_BOOK_SEL)
  {
    _books_list_menu->focusUp();
    _scrollbar->scrollUp();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    _context_menu->focusUp();
  }
  else if (_mode == MODE_BOOK_READ)
  {
    if (_brightness_edit_en)
    {
      if (_brightness + 5 < 255)
      {
        _brightness += 5;
        _display.setBrightness(_brightness);
      }
    }
  }
}

void ReaderContext::down()
{
  if (_mode == MODE_BOOK_DIR_SEL)
  {
    _book_dirs_menu->focusDown();
    _scrollbar->scrollDown();
  }
  else if (_mode == MODE_BOOK_SEL)
  {
    _books_list_menu->focusDown();
    _scrollbar->scrollDown();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    _context_menu->focusDown();
  }
  else if (_mode == MODE_BOOK_READ)
  {
    if (_brightness_edit_en)
    {
      if (_brightness > 10)
      {
        _brightness -= 5;
        _display.setBrightness(_brightness);
      }
    }
  }
}

void ReaderContext::left()
{
  if (_mode == MODE_BOOK_READ)
    loadBookPage(true);
}

void ReaderContext::right()
{
  if (_mode == MODE_BOOK_READ)
    loadBookPage();
}

void ReaderContext::back()
{
  if (_mode == MODE_BOOK_DIR_SEL)
  {
    openContextByID(ID_CONTEXT_MENU);
  }
  else if (_mode == MODE_BOOK_SEL)
  {
    indexDirs();
    showBookDirsTmpl();
    fillBookDirs();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    hideContextMenu();
  }
  else if (_mode == MODE_BOOK_LOAD && _is_book_loading)
  {
    _mode = MODE_TASK_CANCELING;
    _need_load_book = false;
    cancelBookLoad();
  }
}

void ReaderContext::backPressed()
{
  if (_mode == MODE_BOOK_READ)
  {
    if (!_brightness_edit_en)
    {
      _input.enableBtn(BtnID::BTN_UP);
      _input.enableBtn(BtnID::BTN_DOWN);
      _brightness_edit_en = false;
    }

    _display.setBrightness(_old_brightness);

    savePref();
    showBooksListTmpl();
    if (_is_cont_read)
      indexBooks();
    fillBooks(_book_pos);
  }
}

void ReaderContext::showLoadBookTmpl()
{
  _mode = MODE_BOOK_LOAD;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);
  layout->setBackColor(COLOR_BLACK);

  Label* load_msg = new Label(1);
  layout->addWidget(load_msg);
  load_msg->setText(STR_LOADING);
  load_msg->setWidth(UI_WIDTH);
  load_msg->setFont(font_10x20);
  load_msg->setTextSize(2);
  load_msg->setAlign(IWidget::ALIGN_CENTER);
  load_msg->setGravity(IWidget::GRAVITY_CENTER);
  load_msg->setPos(0, getCenterY(load_msg));
}

void ReaderContext::openBook(bool contn)
{
  if (contn)
  {
    if ((_dirname.isEmpty() || _book_name.isEmpty()))
      return;
  }
  else
  {
    _book_name = _books_list_menu->getCurrItemText();
    _book_pos = _books_list_menu->getCurrItemID() - 1;
    _cur_book_page = 0;
  }

  _is_cont_read = contn;

  _need_load_book = true;
  _is_book_loading = true;

  xTaskCreatePinnedToCore(createBookPagesTask, "cbpt", 5 * 512, this, 10, NULL, 0);
  showLoadBookTmpl();
}

String ReaderContext::getBookPath(const char* dirname, const char* book_name)
{
  String book_path = ROOT_PATH;
  book_path += "/";
  book_path += dirname;
  book_path += "/";
  book_path += book_name;
  return book_path;
}

//-------------------------------------------------------------------------------------------

ReaderContext::ReaderContext()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  _old_brightness = _display.getBrightness();

  String book_br = SettingsManager::get(STR_BOOK_BRIGHT_PREF);

  if (book_br.isEmpty())
    _brightness = _old_brightness;
  else
    _brightness = atoi(book_br.c_str());

  _dirname = SettingsManager::get(STR_BOOK_DIR_PREF);
  _book_name = SettingsManager::get(STR_BOOK_NAME_PREF);
  _cur_book_page = atoi(SettingsManager::get(STR_READ_BOOK_PAGE).c_str());

  showBookDirsTmpl();
  indexDirs();
  fillBookDirs();
}

ReaderContext::~ReaderContext()
{
  setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
}

void ReaderContext::update()
{
  if (_mode == MODE_SD_UNCONN)
  {
    if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      openContextByID(ID_CONTEXT_MENU);
    }

    return;
  }

  if (_mode == MODE_BOOK_LOAD)
  {
    if (_input.isReleased(BtnID::BTN_BACK))
    {
      _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
      back();
    }
    else if (!_is_book_loading)
    {
      showReadTmpl();
      loadBookPage(false, true);
    }
    return;
  }

  if (_input.isPressed(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, PRESS_LOCK);
    if (_mode == MODE_BOOK_SEL)
      showContextMenuTmpl();
  }
  else if (_input.isPressed(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, PRESS_LOCK);
    backPressed();
  }
  else if (_input.isHolded(BtnID::BTN_UP))
  {
    _input.lock(BtnID::BTN_UP, HOLD_LOCK);
    up();
  }
  else if (_input.isHolded(BtnID::BTN_DOWN))
  {
    _input.lock(BtnID::BTN_DOWN, HOLD_LOCK);
    down();
  }
  else if (_input.isReleased(BtnID::BTN_RIGHT))
  {
    _input.lock(BtnID::BTN_RIGHT, CLICK_LOCK);
    right();
  }
  else if (_input.isReleased(BtnID::BTN_LEFT))
  {
    _input.lock(BtnID::BTN_LEFT, CLICK_LOCK);
    left();
  }
  else if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, CLICK_LOCK);
    ok();
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, CLICK_LOCK);
    back();
  }
}

void ReaderContext::ok()
{
  if (_mode == MODE_BOOK_DIR_SEL)
  {
    uint16_t item_ID = _book_dirs_menu->getCurrItemID();

    if (item_ID == ID_CONT_ITEM)
    {
      openBook(true);
    }
    else
    {
      _dirname = _book_dirs_menu->getCurrItemText();
      _book_pos = 0;
      indexBooks();
      showBooksListTmpl();
      fillBooks(_book_pos);
    }
  }
  else if (_mode == MODE_BOOK_SEL)
  {
    if (_books_list_menu->getSize() > 0)
    {
      openBook(false);
    }
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    uint16_t id = _context_menu->getCurrItemID();

    if (id == ID_ITEM_DEL)
    {
      _book_name = _books_list_menu->getCurrItemText();
      if (_book_name.isEmpty())
        return;

      String book_path = getBookPath(_dirname.c_str(), _book_name.c_str());

      if (_fs.rmFile(book_path.c_str()))
      {
        if (_books_list_menu->getCurrItemID() - 2 > -1)
          _book_pos = _books_list_menu->getCurrItemID() - 2;
        else
          _book_pos = 0;

        hideContextMenu();
        indexBooks();
        updateBookPos();
        fillBooks(_book_pos);
      }
    }
  }
  else if (_mode == MODE_BOOK_READ)
  {
    _brightness_edit_en = !_brightness_edit_en;

    if (_brightness_edit_en)
    {
      _input.enableBtn(BtnID::BTN_UP);
      _input.enableBtn(BtnID::BTN_DOWN);
      _input.enableBtn(BtnID::BTN_BACK);
      _progress_lbl->setVisibility(IWidget::VISIBLE);
    }
    else
    {
      _input.disableBtn(BtnID::BTN_UP);
      _input.disableBtn(BtnID::BTN_DOWN);
      _input.disableBtn(BtnID::BTN_BACK);
      _progress_lbl->setVisibility(IWidget::INVISIBLE);
    }
  }
}

void ReaderContext::updateBookPos()
{
  if (_book_pos > 0)
  {
    if (_book_pos >= _books.size())
      _book_pos = _books.size() - 1;
  }
}

void ReaderContext::updateReadProgress()
{
  String prog_txt;
  prog_txt += _cur_book_page + 1;
  prog_txt += "/";
  prog_txt += _pages.size();

  _progress_lbl->setText(prog_txt);
  _progress_lbl->updateWidthToFit();
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReaderContext::loadBookPage(bool is_back_read, bool is_first_read)
{
  setCpuFrequencyMhz(MAX_CPU_FREQ_MHZ);
  String book_path = getBookPath(_dirname.c_str(), _book_name.c_str());

  if (!is_back_read)
  {
    if (!is_first_read)
    {
      if (_cur_book_page == _pages.size() - 1)
        return;

      ++_cur_book_page;
    }
  }
  else if (_cur_book_page > 0)
  {
    --_cur_book_page;
  }
  else
  {
    return;
  }

  size_t bytes_read = _fs.readFile(book_path.c_str(), _bytes_buff, _pages[_cur_book_page].bytes_to_read, _pages[_cur_book_page].start_seek_pos);
  _bytes_buff[bytes_read] = '\0';

  _page->setText(_bytes_buff);
  updateReadProgress();
  setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
}

void ReaderContext::createBookPagesTask(void* params)
{
  ReaderContext* self = static_cast<ReaderContext*>(params);

  setCpuFrequencyMhz(MAX_CPU_FREQ_MHZ);

  self->_pages.clear();
  self->_pages.reserve(200);

  String book_path = self->getBookPath(self->_dirname.c_str(), self->_book_name.c_str());
  size_t bytes_read{0};
  size_t seek_pos{0};
  size_t bytes_placed{0};

  unsigned long ts = millis();

  FILE* file = _fs.openFile(book_path.c_str(), "rb");

  while (self->_need_load_book)
  {
    bytes_read = _fs.readFromFile(file, self->_bytes_buff, NUM_BYTES_TO_READ, seek_pos);

    if (bytes_read == 0)
      break;

    self->_bytes_buff[bytes_read] = '\0';
    bytes_placed = self->placeUniMultiline(self->_bytes_buff, ROWS_PER_PAGE, CHARS_PER_LINE);

    self->_pages.emplace_back(seek_pos, bytes_placed);

    seek_pos += bytes_placed;

    if (millis() - ts > WDT_GUARD_TIME)
    {
      delay(1);
      ts = millis();
    }
  }

  _fs.closeFile(file);
  self->_pages.shrink_to_fit();

  setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
  self->_is_book_loading = false;
  vTaskDelete(NULL);
}

size_t ReaderContext::placeUniMultiline(const char* str, uint16_t rows, uint16_t chars_per_line)
{
  size_t placed_bytes = 0;
  uint16_t current_row = 1;
  uint16_t current_col = 0;

  const unsigned char* p = reinterpret_cast<const unsigned char*>(str);
  size_t char_len{0};

  while (*p && current_row <= rows)
  {
    if ((*p & 0xE0) == 0xC0)
      char_len = 2;
    else
      char_len = 1;

    if (*p == '\n')
    {
      if (current_row < rows)
      {
        placed_bytes += 1;
        ++p;
        ++current_row;
        current_col = 0;
        continue;
      }
      else
      {
        break;
      }
    }

    if (current_col >= chars_per_line)
    {
      ++current_row;
      current_col = 0;
      if (current_row > rows)
        break;
    }

    placed_bytes += char_len;
    p += char_len;
    ++current_col;
  }
  return placed_bytes;
}

void ReaderContext::cancelBookLoad()
{
  if (!_is_cont_read)
  {
    showBooksListTmpl();
    fillBooks(_book_pos);
  }
  else
  {
    showBookDirsTmpl();
    fillBookDirs();
  }
}

#pragma GCC optimize("O3")
#include "FilesContext.h"

#include "lib/qr/QR_Gen.h"
#include "manager/SettingsManager.h"
#include "manager/res/BmpLoader.h"
//
#include "../WidgetCreator.h"
#include "./res/folder.h"
#include "./res/lua.h"
#include "widget/menu/item/MenuItem.h"
#include "widget/menu/item/ToggleItem.h"
#include "widget/progress/ProgressBar.h"

#define UPD_TRACK_INF_INTERVAL 1000UL
#define PADDING_BOTT 40
#define MENU_ITEMS_NUM 6

const char STR_SIZE[] = "File size:";
const char STR_LUA_EXT[] = ".lua";
const char STR_BMP_EXT[] = ".bmp";
const char STR_SET_WALLPP[] = "На шпалери";
const char STR_BACK_IMPORT[] = "Отримати у фоні";

bool FilesContext::loop()
{
  if (_mode == MODE_LUA)
  {
    if (_lua_context->isReleased())
    {
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
      _lua_context->tick();
      return false;
    }
  }
  return true;
}

FilesContext::FilesContext()
{
  setCpuFrequencyMhz(MAX_CPU_FREQ_MHZ);
  _dir_img = new Image(1);
  _dir_img->setTransparency(true);
  _dir_img->setWidth(16);
  _dir_img->setHeight(16);
  _dir_img->setSrc(FOLDER_IMG);
  _dir_img->setTransparency(true);

  _lua_img = _dir_img->clone(1);
  _lua_img->setSrc(LUA_IMG);

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  createNotificationObj();

  _fs.setTaskDoneHandler(taskDone, this);

  indexCurDir();
  showFilesTmpl();
  fillFilesTmpl();
}

FilesContext::~FilesContext()
{
  delete _dir_img;
  delete _lua_img;
  delete _lua_context;
  delete _notification;
  setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
}

//-------------------------------------------------------------------------------------------

void FilesContext::showSDErrTmpl()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_SD_ERR);
  layout->addWidget(_msg_lbl);

  _mode = MODE_SD_UNCONN;

  setLayout(layout);
}

void FilesContext::showServerTmpl()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();

  String header_str;

  if (_server.getServerMode() == FileServer::SERVER_MODE_SEND)
    header_str = STR_SHARE;
  else
    header_str = STR_GET;

  header_str += ": ";
  header_str += _server.getAddress().c_str();

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, header_str.c_str());
  layout->addWidget(_msg_lbl);
  _msg_lbl->setHeight(_msg_lbl->getCharHgt() + 4);

  if (_qr_img_buff)
  {
    _qr_img = new Image(ID_QR_IMG);
    layout->addWidget(_qr_img);
    _qr_img->setWidth(_qr_width);
    _qr_img->setHeight(_qr_width);
    _qr_img->setSrc(_qr_img_buff);
    _qr_img->setPos((UI_WIDTH - _qr_width) / 2, (UI_HEIGHT - _qr_width) / 2);
  }

  _mode = MODE_FILE_SERVER;
  setLayout(layout);
}

void FilesContext::showCopyingTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_COPYING, 2);
  layout->addWidget(_msg_lbl);
  _msg_lbl->setHeight(32);
  _msg_lbl->setPos(0, UI_HEIGHT / 2 - _msg_lbl->getHeight() - 2);

  _task_progress = new ProgressBar(ID_PROGRESS);
  layout->addWidget(_task_progress);
  _task_progress->setBackColor(COLOR_BLACK);
  _task_progress->setProgressColor(COLOR_ORANGE);
  _task_progress->setBorderColor(COLOR_WHITE);
  _task_progress->setMax(100);
  _task_progress->setWidth(UI_WIDTH - 5 * 8);
  _task_progress->setHeight(20);
  _task_progress->setProgress(0);
  _task_progress->setPos((UI_WIDTH - _task_progress->getWidth()) / 2, UI_HEIGHT / 2 + 2);

  _mode = MODE_COPYING;

  setLayout(layout);
}

void FilesContext::showRemovingTmpl()
{
  _mode = MODE_REMOVING;

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_REMOVING, 2);
  layout->addWidget(_msg_lbl);
}

void FilesContext::showCancelingTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_CANCELING, 2);
  layout->addWidget(_msg_lbl);

  _mode = MODE_CANCELING;
  setLayout(layout);
}

void FilesContext::showFilesTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  layout->setBackColor(COLOR_BLACK);

  _scrollbar = new ScrollBar(ID_SCROLLBAR);
  layout->addWidget(_scrollbar);
  _scrollbar->setWidth(SCROLLBAR_WIDTH);
  _scrollbar->setHeight(UI_HEIGHT - PADDING_BOTT);
  _scrollbar->setPos(UI_WIDTH - SCROLLBAR_WIDTH, 0);
  _scrollbar->setBackColor(COLOR_MAIN_BACK);

  Label* size_title_lbl = new Label(ID_SIZE_TITLE_LBL);
  layout->addWidget(size_title_lbl);
  size_title_lbl->setText(STR_SIZE);
  size_title_lbl->setTextColor(COLOR_WHITE);
  size_title_lbl->initWidthToFit();
  size_title_lbl->setPos(0, UI_HEIGHT - size_title_lbl->getHeight() * 2 - 2);

  _file_size_lbl = new Label(ID_SIZE_LBL);
  layout->addWidget(_file_size_lbl);
  _file_size_lbl->setText("0");
  _file_size_lbl->setTextColor(COLOR_WHITE);
  _file_size_lbl->initWidthToFit();
  _file_size_lbl->setPos(size_title_lbl->getXPos() + size_title_lbl->getWidth() + 2, size_title_lbl->getYPos());

  _file_pos_lbl = new Label(ID_FILE_POS_LBL);
  layout->addWidget(_file_pos_lbl);
  _file_pos_lbl->setText("[0/0]");
  _file_pos_lbl->setTextColor(COLOR_WHITE);
  _file_pos_lbl->initWidthToFit();
  _file_pos_lbl->setPos(0, UI_HEIGHT - _file_pos_lbl->getHeight() - 2);

  _files_list = WidgetCreator::getDynamicMenu(ID_DYNAMIC_MENU);
  layout->addWidget(_files_list);
  _files_list->setHeight(UI_HEIGHT - (UI_HEIGHT - _file_size_lbl->getYPos()) - 2);
  _files_list->setItemHeight((_files_list->getHeight() - 2) / MENU_ITEMS_NUM);
  _files_list->setWidth(UI_WIDTH - SCROLLBAR_WIDTH);

  _files_list->setOnNextItemsLoadHandler(onNextItemsLoad, this);
  _files_list->setOnPrevItemsLoadHandler(onPrevItemsLoad, this);

  _mode = MODE_NAVIGATION;

  setLayout(layout);
}

//-------------------------------------------------------------------------------------------

String FilesContext::makePathFromBreadcrumbs() const
{
  String out_str;

  for (uint8_t i{0}; i < _breadcrumbs.size(); ++i)
    out_str += _breadcrumbs[i];

  return out_str;
}

void FilesContext::showContextMenu()
{
  _mode = MODE_CONTEXT_MENU;
  _files_list->disable();

  _context_menu = new FixedMenu(ID_CNTXT_MENU);
  getLayout()->addWidget(_context_menu);
  _context_menu->setItemHeight(18);
  _context_menu->setWidth((float)UI_WIDTH / 2.2);
  _context_menu->setBackColor(COLOR_BLACK);
  _context_menu->setBorder(true);
  _context_menu->setBorderColor(COLOR_ORANGE);
  _context_menu->setLoopState(true);

  if (_has_moving_file || _has_copying_file)
  {
    // Якщо є файл для переміщення додати відповідний пункт меню
    MenuItem* paste_item = WidgetCreator::getMenuItem(ID_ITEM_PASTE);
    _context_menu->addItem(paste_item);

    Label* paste_lbl = WidgetCreator::getItemLabel(STR_PASTE, font_unifont);
    paste_item->setLbl(paste_lbl);
    paste_lbl->setHPadding(1);
  }

  // новий каталог
  MenuItem* new_dir_item = WidgetCreator::getMenuItem(ID_ITEM_NEW_DIR);
  _context_menu->addItem(new_dir_item);

  Label* new_dir_lbl = WidgetCreator::getItemLabel(STR_NEW_DIR, font_unifont);
  new_dir_item->setLbl(new_dir_lbl);
  new_dir_lbl->setHPadding(1);

  // контекст для вибраного файлу
  uint16_t id = _files_list->getCurrItemID();

  if (id > 0)
  {
    if (!_files[id - 1].isDir())
    {
      // Виконати Lua-скрипт
      if (_files[id - 1].nameEndsWith(STR_LUA_EXT))
      {
        MenuItem* exec_item = WidgetCreator::getMenuItem(ID_ITEM_EXECUTE);
        _context_menu->addItem(exec_item);

        Label* exec_lbl = WidgetCreator::getItemLabel(STR_EXECUTE, font_unifont);
        exec_item->setLbl(exec_lbl);
        exec_lbl->setHPadding(1);
      }
      else if (_files[id - 1].nameEndsWith(STR_BMP_EXT))
      {
        String path_to_bmp = makePathFromBreadcrumbs();
        path_to_bmp += "/";
        path_to_bmp += _files[id - 1].getName();

        FILE* bmp_file = _fs.openFile(path_to_bmp.c_str(), "rb");
        if (bmp_file)
        {
          BmpLoader::BmpHeader bmp_header;
          if (BmpLoader::checkBmpFile(bmp_file, bmp_header))
          {
            MenuItem* set_wall_item = WidgetCreator::getMenuItem(ID_ITEM_SET_WALLPP);
            _context_menu->addItem(set_wall_item);

            Label* set_wall_lbl = WidgetCreator::getItemLabel(STR_SET_WALLPP, font_unifont);
            set_wall_item->setLbl(set_wall_lbl);
            set_wall_lbl->setHPadding(1);
          }
          _fs.closeFile(bmp_file);
        }
      }

      // копіювати
      MenuItem* copy_item = WidgetCreator::getMenuItem(ID_ITEM_COPY);
      _context_menu->addItem(copy_item);

      Label* copy_lbl = WidgetCreator::getItemLabel(STR_COPY, font_unifont);
      copy_item->setLbl(copy_lbl);
      copy_lbl->setHPadding(1);
    }

    // перейменувати
    MenuItem* rename_item = WidgetCreator::getMenuItem(ID_ITEM_RENAME);
    _context_menu->addItem(rename_item);

    Label* rename_lbl = WidgetCreator::getItemLabel(STR_RENAME, font_unifont);
    rename_item->setLbl(rename_lbl);
    rename_lbl->setHPadding(1);

    // перемістити
    MenuItem* move_item = WidgetCreator::getMenuItem(ID_ITEM_MOVE);
    _context_menu->addItem(move_item);

    Label* move_lbl = WidgetCreator::getItemLabel(STR_MOVE, font_unifont);
    move_item->setLbl(move_lbl);
    move_lbl->setHPadding(1);

    // видалити
    MenuItem* delete_item = WidgetCreator::getMenuItem(ID_ITEM_REMOVE);
    _context_menu->addItem(delete_item);

    Label* delete_lbl = WidgetCreator::getItemLabel(STR_DELETE, font_unifont);
    delete_item->setLbl(delete_lbl);
    delete_lbl->setHPadding(1);
  }

  // імпорт
  MenuItem* import_item = WidgetCreator::getMenuItem(ID_ITEM_IMPORT);
  _context_menu->addItem(import_item);

  Label* import_lbl = WidgetCreator::getItemLabel(STR_GET, font_unifont);
  import_item->setLbl(import_lbl);
  import_lbl->setHPadding(1);

  // експорт
  MenuItem* export_item = WidgetCreator::getMenuItem(ID_ITEM_EXPORT);
  _context_menu->addItem(export_item);

  Label* export_lbl = WidgetCreator::getItemLabel(STR_SHARE, font_unifont);
  export_item->setLbl(export_lbl);
  export_lbl->setHPadding(1);

  // Фоновий сервер
  ToggleItem* import_back_item = new ToggleItem(ID_ITEM_BACK_IMPORT);
  _context_menu->addItem(import_back_item);
  import_back_item->setFocusBorderColor(COLOR_LIME);
  import_back_item->setFocusBackColor(COLOR_FOCUS_BACK);
  import_back_item->setBackColor(COLOR_MENU_ITEM);
  import_back_item->setChangingBorder(true);
  import_back_item->setChangingBack(true);

  Label* import_back_lbl = WidgetCreator::getItemLabel(STR_BACK_IMPORT, font_unifont);
  import_back_item->setLbl(import_back_lbl);
  import_back_lbl->setHPadding(1);
  import_back_lbl->setFullAutoscroll(false);

  ToggleSwitch* import_back_toggle = new ToggleSwitch(1);
  import_back_item->setToggle(import_back_toggle);
  import_back_toggle->setWidth(30);
  import_back_toggle->setHeight(_context_menu->getItemHeight() - 4);
  import_back_toggle->setCornerRadius(4);
  import_back_toggle->setOn(_server.isWorking());

  // оновити
  MenuItem* upd_item = WidgetCreator::getMenuItem(ID_ITEM_UPDATE);
  _context_menu->addItem(upd_item);

  Label* upd_lbl = WidgetCreator::getItemLabel(STR_UPDATE, font_unifont);
  upd_item->setLbl(upd_lbl);
  upd_lbl->setHPadding(1);

  //
  _context_menu->setHeight(_context_menu->getSize() * _context_menu->getItemHeight() + 4);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth() - 1,
                        UI_HEIGHT - PADDING_BOTT - _context_menu->getHeight() - 2);
}

void FilesContext::hideContextMenu()
{
  getLayout()->delWidgetByID(ID_CNTXT_MENU);
  _mode = MODE_NAVIGATION;
  _files_list->enable();
}

//-------------------------------------------------------------------------------------------

void FilesContext::showDialog(Mode mode)
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _dialog_txt = new TextBox(ID_DIALOG_TXT);
  _dialog_txt->setHPadding(5);
  _dialog_txt->setWidth(UI_WIDTH - 10);
  _dialog_txt->setHeight(40);
  _dialog_txt->setBackColor(COLOR_WHITE);
  _dialog_txt->setTextColor(COLOR_BLACK);
  _dialog_txt->setTextSize(2);
  _dialog_txt->setPos(5, 0);
  _dialog_txt->setCornerRadius(3);

  _mode = mode;
  if (_mode == MODE_RENAME_DIALOG)
  {
    _old_name = _files_list->getCurrItemText();
    _dialog_txt->setText(_old_name);
  }

  _keyboard = WidgetCreator::getStandardEnKeyboard(ID_KEYBOARD);

  layout->setBackColor(COLOR_BLACK);
  layout->addWidget(_dialog_txt);
  layout->addWidget(_keyboard);

  setLayout(layout);
}

void FilesContext::hideDialog()
{
  showFilesTmpl();
  if (_dialog_success_res)
    indexCurDir();
  fillFilesTmpl();
}

void FilesContext::saveDialogResult()
{
  if (_mode == MODE_NEW_DIR_DIALOG)
  {
    String dir_path = makePathFromBreadcrumbs();
    dir_path += "/";
    dir_path += _dialog_txt->getText();

    _dialog_success_res = _fs.createDir(dir_path.c_str());
    showResultToast(_dialog_success_res);
  }
  else if (_mode == MODE_RENAME_DIALOG)
  {
    String old_name = makePathFromBreadcrumbs();
    String new_name = old_name;
    old_name += "/";
    old_name += _old_name;
    _old_name = "";

    new_name += "/";
    new_name += _dialog_txt->getText();

    _dialog_success_res = _fs.rename(old_name.c_str(), new_name.c_str());
    showResultToast(_dialog_success_res);
  }

  hideDialog();
}

void FilesContext::keyboardClickHandler()
{
  // uint16_t id = _keyboard->getCurrBtnID();
  // if (id ==)
  // {
  // }

  // Можна оброблювати по ID кнопки, але в даному випадку зручніше оброблювати текст,
  // Тому що ніякі керуючі кнопки не використовуються.

  _dialog_txt->addChars(_keyboard->getCurrBtnTxt().c_str());
}

//-------------------------------------------------------------------------------------------

void FilesContext::prepareFileMoving()
{
  _path_from = makePathFromBreadcrumbs();
  _name_from = _files_list->getCurrItemText();

  _has_moving_file = true;
  _has_copying_file = false;

  hideContextMenu();
}

void FilesContext::prepareFileCopying()
{
  _path_from = makePathFromBreadcrumbs();
  _name_from = _files_list->getCurrItemText();

  _has_moving_file = false;
  _has_copying_file = true;

  hideContextMenu();
}

void FilesContext::pasteFile()
{
  String old_file_path = _path_from;
  old_file_path += "/";
  old_file_path += _name_from;

  String new_file_path = makePathFromBreadcrumbs();
  new_file_path += "/";
  new_file_path += _name_from;

  if (_has_moving_file)
  {
    if (_fs.exists(old_file_path.c_str()) && _fs.rename(old_file_path.c_str(), new_file_path.c_str()))
    {
      indexCurDir();
      fillFilesTmpl();
      hideContextMenu();
      showResultToast(true);
    }
    else
      showResultToast(false);
  }
  else if (_has_copying_file)
  {
    if (!_fs.startCopyingFile(old_file_path.c_str(), new_file_path.c_str()))
    {
      showResultToast(false);
    }
    else
    {
      showCopyingTmpl();
      _task_runnning = true;
    }
  }

  _has_moving_file = false;
  _has_copying_file = false;

  _path_from = "";
  _name_from = "";
}

void FilesContext::removeFile()
{
  String filename = makePathFromBreadcrumbs();
  filename += "/";
  filename += _files_list->getCurrItemText();

  if (!_fs.startRemoving(filename.c_str()))
  {
    showResultToast(false);
  }
  else
  {
    showRemovingTmpl();
    _task_runnning = true;
  }
}

//-------------------------------------------------------------------------------------------

void FilesContext::update()
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

  if (_input.isPressed(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, PRESS_LOCK);
    if (_mode == MODE_NAVIGATION)
      showContextMenu();
    else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
      saveDialogResult();
  }
  else if (_input.isPressed(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, PRESS_LOCK);

    if (_mode == MODE_NAVIGATION)
      openContextByID(ID_CONTEXT_MENU);
    else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
      hideDialog();
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
  else if (_input.isHolded(BtnID::BTN_RIGHT))
  {
    _input.lock(BtnID::BTN_RIGHT, HOLD_LOCK);
    if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
      _keyboard->focusRight();
  }
  else if (_input.isHolded(BtnID::BTN_LEFT))
  {
    _input.lock(BtnID::BTN_LEFT, HOLD_LOCK);
    if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
      _keyboard->focusLeft();
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

  if (_fs.isWorking())
  {
    if ((millis() - _upd_msg_time) > UPD_TRACK_INF_INTERVAL)
    {
      String upd_txt;
      String upd_progress;

      if (_upd_counter > 2)
        _upd_counter = 0;
      else
        ++_upd_counter;

      for (uint8_t i{0}; i < _upd_counter; ++i)
        upd_progress += ".";

      if (_mode == MODE_CANCELING)
      {
        upd_txt = STR_CANCELING;
        upd_txt += upd_progress;
        _msg_lbl->setText(upd_txt);
      }
      else if (_mode == MODE_COPYING)
      {
        _task_progress->setProgress(_fs.getCopyProgress());
        _upd_msg_time = millis();
      }
      else if (_mode == MODE_REMOVING)
      {
        upd_txt = STR_REMOVING;
        upd_txt += upd_progress;
        _msg_lbl->setText(upd_txt);
      }

      _upd_msg_time = millis();
    }
  }
  else if (_task_runnning && _task_done)
  {
    _task_runnning = false;
    _task_done = false;

    showResultToast(_task_done_result);

    indexCurDir();
    showFilesTmpl();
    fillFilesTmpl();
  }
}

void FilesContext::ok()
{
  if (_mode == MODE_NAVIGATION)
  {
    openNextLevel();
  }
  else if (_mode == MODE_CONTEXT_MENU)
  {
    uint16_t id = _context_menu->getCurrItemID();

    if (id == ID_ITEM_UPDATE)
    {
      hideContextMenu();
      indexCurDir();
      fillFilesTmpl();
    }
    else if (id == ID_ITEM_REMOVE)
      removeFile();
    else if (id == ID_ITEM_MOVE)
      prepareFileMoving();
    else if (id == ID_ITEM_COPY)
      prepareFileCopying();
    else if (id == ID_ITEM_PASTE)
      pasteFile();
    else if (id == ID_ITEM_NEW_DIR)
      showDialog(MODE_NEW_DIR_DIALOG);
    else if (id == ID_ITEM_RENAME)
      showDialog(MODE_RENAME_DIALOG);
    else if (id == ID_ITEM_IMPORT)
      startFileServer(FileServer::SERVER_MODE_RECEIVE);
    else if (id == ID_ITEM_EXPORT)
      startFileServer(FileServer::SERVER_MODE_SEND);
    else if (id == ID_ITEM_EXECUTE)
      executeScript();
    else if (id == ID_ITEM_BACK_IMPORT)
      runServerInBg();
    else if (id == ID_ITEM_SET_WALLPP)
      saveWallppSettings();
  }
  else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
  {
    keyboardClickHandler();
  }
  else if (_mode == MODE_NOTIFICATION)
  {
    hideNotification();
    _mode = MODE_NAVIGATION;
    getLayout()->drawForced();
  }
}

void FilesContext::back()
{
  if (_fs.isWorking() && _mode != MODE_CANCELING)
  {
    showCancelingTmpl();
    _fs.cancel();
  }
  else if (_mode == MODE_CONTEXT_MENU)
    hideContextMenu();
  else if (_mode == MODE_NAVIGATION)
    openPrevlevel();
  else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
    _dialog_txt->removeLastChar();
  else if (_mode == MODE_FILE_SERVER)
    stopFileServer();
}

void FilesContext::up()
{
  if (_mode == MODE_NAVIGATION)
  {
    if (_files_list->focusUp())
    {
      updateFileInfo();
      _scrollbar->scrollUp();
    }
  }
  else if (_mode == MODE_CONTEXT_MENU)
    _context_menu->focusUp();
  else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
    _keyboard->focusUp();
}

void FilesContext::down()
{
  if (_mode == MODE_NAVIGATION)
  {
    if (_files_list->focusDown())
    {
      updateFileInfo();
      _scrollbar->scrollDown();
    }
  }
  else if (_mode == MODE_CONTEXT_MENU)
    _context_menu->focusDown();
  else if (_mode == MODE_NEW_DIR_DIALOG || _mode == MODE_RENAME_DIALOG)
    _keyboard->focusDown();
}

void FilesContext::updateFileInfo()
{
  String filename = makePathFromBreadcrumbs();
  filename += "/";
  filename += _files_list->getCurrItemText();

  String str_size;
  double f_size = _fs.getFileSize(filename.c_str());

  if (f_size < 1024)
  {
    str_size = String(f_size);
    str_size += "b";
  }
  else if (f_size < 1024 * 1024)
  {
    str_size = String(f_size / 1024.0);
    str_size += "kb";
  }
  else
  {
    str_size = String(f_size * 9.53674316e-7);  // 1 / (1024 * 1024)
    str_size += "mb";
  }

  _file_size_lbl->setText(str_size.c_str());
  _file_size_lbl->updateWidthToFit();

  String pos = "[";
  pos += String(_files_list->getCurrItemID());
  pos += "/";
  pos += String(_files.size());
  pos += "]";
  _file_pos_lbl->setText(pos.c_str());
  _file_pos_lbl->updateWidthToFit();
}

void FilesContext::openNextLevel()
{
  String next_dir = "/";
  next_dir += _files_list->getCurrItemText();

  String next_dir_path = makePathFromBreadcrumbs();
  next_dir_path += next_dir;

  if (!_fs.dirExist(next_dir_path.c_str(), true))
    return;

  _breadcrumbs.push_back(next_dir);
  indexCurDir();
  fillFilesTmpl();
}

void FilesContext::openPrevlevel()
{
  if (_breadcrumbs.size() > 0)
  {
    _breadcrumbs.pop_back();
    indexCurDir();
    fillFilesTmpl();
  }
  else
  {
    openContextByID(ID_CONTEXT_MENU);
  }
}

void FilesContext::indexCurDir()
{
  String dir_path = makePathFromBreadcrumbs();
  _fs.indexAll(_files, dir_path.c_str());
}

void FilesContext::fillFilesTmpl()
{
  _files_list->delWidgets();

  std::vector<MenuItem*> items;

  makeMenuFilesItems(items, 0, _files_list->getItemsPerPage());

  for (size_t i = 0; i < items.size(); ++i)
    _files_list->addItem(items[i]);

  _scrollbar->setValue(0);
  _scrollbar->setMax(_files.size());

  updateFileInfo();
}

void FilesContext::startFileServer(FileServer::ServerMode mode, bool in_back)
{
  String ssid = SettingsManager::get(STR_PREF_FS_AP_SSID);
  String pwd = SettingsManager::get(STR_PREF_FS_AP_PWD);

  if (ssid.isEmpty())
    ssid = STR_DEF_SSID;
  if (pwd.isEmpty())
    pwd = STR_DEF_PWD;

  _server.setSSID(ssid.c_str());
  _server.setPWD(pwd.c_str());
  //
  String cur_path = makePathFromBreadcrumbs();
  if (_server.begin(cur_path.c_str(), mode))
  {
    if (!in_back)
    {
      _server.setReplaceFile(false);
      QR_Gen gen;
      String addr = _server.getAddress();
      _qr_img_buff = gen.generateQR(addr.c_str(), 3);
      _qr_width = gen.getImageWidth();

      showServerTmpl();
    }
    else
    {
      _server.setReplaceFile(true);
      showToast(STR_SUCCESS);
    }
  }
}

void FilesContext::stopFileServer()
{
  _server.stop();

  showFilesTmpl();
  indexCurDir();
  fillFilesTmpl();

  free(_qr_img_buff);
}

void FilesContext::taskDoneHandler(bool result)
{
  _task_done = true;
  _task_done_result = result;
}

void FilesContext::taskDone(bool result, void* arg)
{
  FilesContext* self = static_cast<FilesContext*>(arg);
  self->taskDoneHandler(result);
}

//-------------------------------------------------------------------------------------------

void FilesContext::makeMenuFilesItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size)
{
  if (file_pos >= _files.size())
    return;

  uint16_t read_to = file_pos + size;

  if (read_to > _files.size())
    read_to = _files.size();

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

    if (_files[i].isDir())
      item->setImg(_dir_img->clone(1));
    else if (_files[i].nameEndsWith(STR_LUA_EXT))
      item->setImg(_lua_img->clone(1));

    lbl->setText(_files[i].getName());
  }
}

//-------------------------------------------------------------------------------------------

void FilesContext::handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  if (!cur_id)
    return;

  makeMenuFilesItems(items, cur_id, size);
}

void FilesContext::onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  FilesContext* self = static_cast<FilesContext*>(arg);
  self->handleNextItemsLoad(items, size, cur_id);
}

//

void FilesContext::handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  // Не отримали ID
  if (!cur_id)
    return;

  uint16_t item_pos = cur_id - 1;

  // Вже перший елемент
  if (!item_pos)
    return;

  // Вирахувати першу позицію звідки потрібно читати список.
  if (cur_id > size)
    item_pos = cur_id - size - 1;
  else
  {
    item_pos = 0;
    // Вирівняти скролбар, якщо меню було завантажене не з першого елемента
    _scrollbar->setValue(cur_id);
  }

  makeMenuFilesItems(items, item_pos, size);
}

void FilesContext::onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  FilesContext* self = static_cast<FilesContext*>(arg);
  self->handlePrevItemsLoad(items, size, cur_id);
}

void FilesContext::showResultToast(bool result)
{
  if (result)
    showToast(STR_SUCCESS, TOAST_LENGTH_SHORT);
  else
    showToast(STR_FAIL, TOAST_LENGTH_SHORT);
}

void FilesContext::createNotificationObj()
{
  const uint8_t V_MARGIN{40};
  const uint8_t H_MARGIN{100};

  _notification = new Notification(1);
  _notification->setWidth(UI_WIDTH - V_MARGIN);
  _notification->setHeight(UI_HEIGHT - H_MARGIN);
  _notification->setPos((UI_WIDTH - _notification->getWidth()) / 2, (UI_HEIGHT - _notification->getHeight()) / 2);
  _notification->setBackColor(COLOR_DARKCYAN);

  _notification->getLeftLbl()->setBackColor(COLOR_DARKCYAN);
  _notification->getRightLbl()->setBackColor(COLOR_DARKCYAN);
  _notification->getTitleLbl()->setBackColor(COLOR_DARKCYAN);
  _notification->getMsgLbl()->setBackColor(COLOR_DARKCYAN);

  _notification->setTitleText(STR_NOTIFICATION);
  _notification->setRightText(STR_OK);
}

void FilesContext::runServerInBg()
{
  hideContextMenu();

  if (_server.isWorking())
    _server.stop();
  else
    startFileServer(FileServer::SERVER_MODE_RECEIVE, true);
}

void FilesContext::executeScript()
{
  String file_name = makePathFromBreadcrumbs();
  file_name += "/";
  file_name += _files_list->getCurrItemText();

  size_t f_size = _fs.getFileSize(file_name.c_str());

  if (f_size == 0)
  {
    log_e("Скрипт порожній");
    return;
  }

  char* text_buf;

  if (psramInit())
    text_buf = static_cast<char*>(ps_malloc(f_size + 1));
  else
    text_buf = static_cast<char*>(malloc(f_size + 1));

  if (!text_buf)
  {
    log_e("Помилка виділення пам'яті");
    return;
  }

  hideContextMenu();

  _fs.readFile(file_name.c_str(), text_buf, f_size);
  text_buf[f_size] = '\0';

  _lua_context = new LuaContext();
  if (!_lua_context->execScript(text_buf))
  {
    String msg = _lua_context->getMsg();
    _notification->setMsgText(msg);
    _mode = MODE_NOTIFICATION;
    showNotification(_notification);
    delete _lua_context;
    _lua_context = nullptr;
  }
  else
  {
    _mode = MODE_LUA;
  }

  free(text_buf);
}

void FilesContext::saveWallppSettings()
{
  hideContextMenu();

  String path_to_bmp = makePathFromBreadcrumbs();
  path_to_bmp += "/";
  path_to_bmp += _files_list->getCurrItemText();

  if (!SettingsManager::set(STR_WALLPP_FILENAME, path_to_bmp.c_str()))
    showToast(STR_FAIL);
  else
    showToast(STR_SUCCESS);
}

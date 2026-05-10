#pragma once
#pragma GCC optimize("O3")

#include "lib/server/FileServer.h"
#include "manager/FileManager.h"
//
#include "lib/lua/context/LuaContext.h"
#include "widget/notification/Notification.h"
//
#include "widget/image/Image.h"
#include "widget/keyboard/Keyboard.h"
#include "widget/menu/DynamicMenu.h"
#include "widget/menu/FixedMenu.h"
#include "widget/progress/ProgressBar.h"
#include "widget/scrollbar/ScrollBar.h"
#include "widget/text/TextBox.h"

using namespace pixeler;

class FilesContext : public IContext
{
public:
  FilesContext();
  virtual ~FilesContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_SCROLLBAR = 1,
    ID_DYNAMIC_MENU,
    ID_CNTXT_MENU,
    ID_KEYBOARD,
    ID_DIALOG_TXT,
    ID_MSG_LBL,
    ID_PROGRESS,
    ID_QR_IMG,
    ID_QR_LBL,
    ID_SIZE_TITLE_LBL,
    ID_SIZE_LBL,
    ID_FILE_POS_LBL,
  };

  enum Item_ID : uint8_t
  {
    ID_ITEM_UPDATE = 1,
    ID_ITEM_PASTE,
    ID_ITEM_MOVE,
    ID_ITEM_COPY,
    ID_ITEM_REMOVE,
    ID_ITEM_NEW_DIR,
    ID_ITEM_EXECUTE,
    ID_ITEM_SET_WALLPP,
    ID_ITEM_RENAME,
    ID_ITEM_IMPORT,
    ID_ITEM_BACK_IMPORT,
    ID_ITEM_EXPORT,
  };

  enum Mode : uint8_t
  {
    MODE_NAVIGATION = 0,
    MODE_LUA,
    MODE_NOTIFICATION,
    MODE_COPYING,
    MODE_REMOVING,
    MODE_MOVING,
    MODE_CONTEXT_MENU,
    MODE_NEW_DIR_DIALOG,
    MODE_RENAME_DIALOG,
    MODE_SD_UNCONN,
    MODE_CANCELING,
    MODE_FILE_SERVER
  };
  //
  String makePathFromBreadcrumbs() const;
  //
  void showFilesTmpl();
  void showCopyingTmpl();
  void showRemovingTmpl();
  void showCancelingTmpl();
  void showSDErrTmpl();
  void showServerTmpl();
  //
  void showContextMenu();
  void hideContextMenu();
  //
  void keyboardClickHandler();
  void showDialog(Mode mode);
  void hideDialog();
  //
  void prepareFileMoving();
  void prepareFileCopying();
  void pasteFile();
  void removeFile();
  //
  void ok();
  void back();
  void up();
  void down();
  void updateFileInfo();
  //
  void openNextLevel();
  void openPrevlevel();
  //
  void indexCurDir();
  void fillFilesTmpl();
  void saveDialogResult();

  void makeMenuFilesItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size);
  //
  void startFileServer(FileServer::ServerMode mode, bool in_back = false);
  void stopFileServer();

  void taskDoneHandler(bool result);
  static void taskDone(bool result, void* arg);
  //
  void handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
  void handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
  void showResultToast(bool result);
  //
  void createNotificationObj();
  //
  void runServerInBg();
  void executeScript();
  void saveWallppSettings();

private:
  FileServer _server;

  String _path_from;
  String _name_from;
  String _copy_to_path;
  String _old_name;
  std::vector<FileInfo> _files;
  std::vector<String> _breadcrumbs;

  LuaContext* _lua_context{nullptr};
  Notification* _notification{nullptr};
  Image* _lua_img{nullptr};
  Label* _msg_lbl{nullptr};
  Image* _qr_img{nullptr};
  uint16_t* _qr_img_buff{nullptr};
  FixedMenu* _context_menu{nullptr};
  ScrollBar* _scrollbar{nullptr};
  ProgressBar* _task_progress{nullptr};
  DynamicMenu* _files_list{nullptr};
  Image* _dir_img{nullptr};
  Keyboard* _keyboard{nullptr};
  TextBox* _dialog_txt{nullptr};
  Label* _file_size_lbl{nullptr};
  Label* _file_pos_lbl{nullptr};

  unsigned long _upd_msg_time{0};
  uint16_t _qr_width{0};

  Mode _mode{MODE_NAVIGATION};
  uint8_t _upd_counter{0};
  bool _is_dir{false};
  bool _has_moving_file{false};
  bool _has_copying_file{false};
  bool _dialog_success_res{false};
  bool _task_runnning{false};
  bool _task_done{false};
  bool _task_done_result{false};
};

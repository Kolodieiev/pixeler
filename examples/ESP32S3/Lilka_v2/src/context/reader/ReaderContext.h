#pragma once

#include "manager/SettingsManager.h"
#include "context/IContext.h"
#include "widget/menu/DynamicMenu.h"
#include "widget/menu/FixedMenu.h"
#include "widget/scrollbar/ScrollBar.h"

using namespace pixeler;

class ReaderContext : public IContext
{
public:
  ReaderContext();
  virtual ~ReaderContext();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Widget_ID : uint8_t
  {
    ID_F_MENU = 1,
    ID_D_MENU,
    ID_SCROLL,
    ID_BOOK_MENU,
    ID_PROGRESS,
    ID_PAGE_LBL,
    ID_PROGRESS_LBL,
    ID_MSG_LBL
  };

  enum Mode : uint8_t
  {
    MODE_BOOK_DIR_SEL = 0,
    MODE_BOOK_SEL,
    MODE_BOOK_READ,
    MODE_BOOK_LOAD,
    MODE_TASK_CANCELING,
    MODE_CONTEXT_MENU,
    MODE_SD_UNCONN
  };

  enum BlMenuItemsID : uint8_t
  {
    ID_ITEM_DEL = 1,
    ID_CONT_ITEM,
  };
  //
  void savePref();
  //
  void ok();
  void up();
  void down();
  void left();
  void right();
  void back();
  void backPressed();
  //
  void showBookDirsTmpl();
  void fillBookDirs();
  void makeBookDirsItems(std::vector<MenuItem*>& items);
  //
  void showBooksListTmpl();
  void fillBooks(uint16_t pos = 0);
  void makeBooksItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size);
  //
  void showContextMenuTmpl();
  void hideContextMenu();
  //
  void showLoadBookTmpl();
  void openBook(bool contn = false);
  //
  void showSDErrTmpl();
  void showReadTmpl();
  //
  void updateReadProgress();
  void updateBookPos();
  //
  void indexDirs();
  void indexBooks();
  //
  void handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
  void handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
  String getBookPath(const char* dirname, const char* book_name);
  void loadBookPage(bool is_back_read = false, bool is_first_read = false);

  static void createBookPagesTask(void* params);
  size_t placeUniMultiline(const char* str, uint16_t rows, uint16_t chars_per_line);

  void cancelBookLoad();

private:
  String _dirname;
  String _book_name;

  // для 320х240
#define ROWS_PER_PAGE 11
#define CHARS_PER_LINE 30
#define NUM_BYTES_TO_READ ROWS_PER_PAGE * CHARS_PER_LINE * 2  //  ROWS_PER_PAGE рядків x CHARS_PER_LINE символів х 2 байти на символ для 320х240

  char _bytes_buff[NUM_BYTES_TO_READ + 1];

  struct BookPage
  {
    uint32_t start_seek_pos;
    uint32_t bytes_to_read;
  };

  std::vector<FileInfo> _dirs;
  std::vector<FileInfo> _books;
  std::vector<BookPage> _pages;

  //
  Label* _progress_lbl;
  Label* _page;
  ScrollBar* _scrollbar;
  FixedMenu* _context_menu;
  FixedMenu* _book_dirs_menu;
  DynamicMenu* _books_list_menu;

  unsigned long _upd_msg_time{0};

  size_t _cur_book_page{0};

  uint16_t _book_pos{0};
  //
  Mode _mode{MODE_BOOK_DIR_SEL};

  bool _need_load_book{false};
  bool _is_book_loading{false};
  bool _is_cont_read{false};
  bool _brightness_edit_en{true};
};
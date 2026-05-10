#pragma once

#include "lib/audio/mp3/Audio.h"
#include "context/IContext.h"
#include "manager/SettingsManager.h"
#include "widget/image/Image.h"
#include "widget/menu/DynamicMenu.h"
#include "widget/menu/FixedMenu.h"
#include "widget/progress/ProgressBar.h"
#include "widget/scrollbar/ScrollBar.h"

using namespace pixeler;

class Mp3Context : public IContext
{
public:
  Mp3Context();
  virtual ~Mp3Context();

protected:
  virtual bool loop() override;
  virtual void update() override;

private:
  enum Mode : uint8_t
  {
    MODE_PLST_SEL = 0,
    MODE_TRACK_SEL,
    MODE_AUDIO_PLAY,
    MODE_PLST_MENU,
    MODE_SD_UNCONN
  };

  enum MenuItemID : uint8_t
  {
    ID_CONT_ITEM = 1,
    ID_ITEM_DEL,
  };

  enum Widget_ID : uint8_t
  {
    ID_F_MENU = 1,
    ID_PL_MENU,
    ID_D_MENU,
    ID_SCROLL,
    ID_TRACK_NAME,
    ID_STREAM_TITLE,
    ID_CUR_TRACK_TIME,
    ID_GEN_TRACK_TIME,
    ID_PLAY_BTN,
    ID_VOLUME_LBL,
    ID_VOLUME_IMG,
    ID_FORWARD_IMG,
    ID_REWIND_IMG,
    ID_PROGRESS,
    ID_MSG_LBL,
  };
  //
  void savePref();
  //
  void showPlaylistsTmpl();
  void fillPlaylists();
  void makeMenuPlaylistsItems(std::vector<MenuItem*>& items);
  //
  void showTracksTmpl();
  void fillTracks(uint16_t track_pos);
  void makeMenuTracksItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size);
  //
  void showPlaying();
  //
  bool playTrack(bool contn = false);
  bool playNext();
  bool playPrev();

  void volumeUp();
  void volumeDown();
  //
  void setStopState();

  bool updateTrackDuration();
  void updateTrackTime();

  void up();
  void down();

  void left();
  void leftPressed();

  void right();
  void rightPressed();

  void ok();
  void okPressed();
  void changeBackLight();

  void back();
  void backPressed();

  void showPlMenu();
  void hidePlMenu();
  //
  void showSDErrTmpl();
  //
  void updateTrackPos();
  //
  void indexPlaylists();
  void indexTracks();

  String getTrackPath(const char* dirname, const char* track_name) const;

  void handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
  void handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id);
  static void onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg);
  //
private:
  Audio _audio;

  String _playlist_name;
  String _track_name;

  std::vector<FileInfo> _playlists;
  std::vector<FileInfo> _tracks;

  Label* _track_name_lbl;
  Label* _stream_title_lbl;
  Image* _play_btn;
  Label* _cur_track_time_lbl;
  Label* _gen_track_time_lbl;
  Label* _volume_lbl;
  ProgressBar* _progress;
  Label* _msg_lbl;
  ScrollBar* _scrollbar;
  FixedMenu* _context_menu;
  DynamicMenu* _tracks_list;
  FixedMenu* _playlists_list;
  //
  unsigned long _upd_msg_time{0};
  uint32_t _track_time{0};
  uint16_t _track_pos{0};

  Mode _mode{MODE_PLST_SEL};

  uint8_t _try_next_counter{0};
  uint8_t _upd_counter{0};
  uint8_t _volume;

  bool _is_new_track{true};
  bool _is_playing{false};
  bool _is_locked{false};
  bool _is_audio_amp_en{false};
  bool _is_radio_mode{false};
};
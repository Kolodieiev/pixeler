#include "Mp3Context.h"

#include "../WidgetCreator.h"
#include "./res/clock.h"
#include "./res/forward.h"
#include "./res/pause.h"
#include "./res/play.h"
#include "./res/rewind.h"
#include "./res/speaker.h"

#define UPD_TRACK_INF_INTERVAL 1000UL
#define UPD_TIME_INTERVAL 10000UL

#define PLAYLIST_ITEMS_NUM 7
#define TRACKS_ITEMS_NUM 7

const char STR_PLAYLIST_PREF[] = "Playlist";
const char STR_TRACK_NAME_PREF[] = "TrackName";
const char STR_VOLUME_PREF[] = "Volume";
const char STR_TRACK_POS_PREF[] = "TrackPos";
const char STR_TRACK_TIME_PREF[] = "TrackTime";

const char ROOT_PATH[] = "/music";
const char AUDIO_EXT[] = ".mp3";

const char STR_ZERO_TRACK_TIME[] = "000:00";

const char STR_STOPPED[] = "Зупинено";

bool Mp3Context::loop()
{
  _audio.loop();
  return true;
}

void Mp3Context::savePref()
{
  SettingsManager::set(STR_VOLUME_PREF, String(_volume).c_str());
  SettingsManager::set(STR_TRACK_POS_PREF, String(_track_pos).c_str());
  SettingsManager::set(STR_TRACK_TIME_PREF, String(_track_time).c_str());
  SettingsManager::set(STR_PLAYLIST_PREF, _playlist_name.c_str());
  SettingsManager::set(STR_TRACK_NAME_PREF, _track_name.c_str());
}

//-------------------------------------------------------------------------------------------

Mp3Context::Mp3Context()
{
  setCpuFrequencyMhz(BALANCED_CPU_FREQ_MHZ);

  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  if (!_fs.isMounted())
  {
    showSDErrTmpl();
    return;
  }

  uint8_t ccpu_cmd_data[2]{CCPU_CMD_PIN_ON, CH_PIN_SPK_PWR};
  _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);

  if (_volume == 0)
    _volume = 5;

  _volume = atoi(SettingsManager::get(STR_VOLUME_PREF).c_str());
  if (_volume == 0)
    _volume = 5;

  _track_pos = atoi(SettingsManager::get(STR_TRACK_POS_PREF).c_str());
  _track_time = atoi(SettingsManager::get(STR_TRACK_TIME_PREF).c_str());
  _playlist_name = SettingsManager::get(STR_PLAYLIST_PREF);
  _track_name = SettingsManager::get(STR_TRACK_NAME_PREF);

  _audio.begin();
  _audio.setVolumeSteps(30);
  _audio.setVolume(_volume);

  String mono_mode = SettingsManager::get(STR_PREF_MONO_AUDIO);
  if (mono_mode.equals("1"))
    _audio.forceMono(true);

  String audio_amp = SettingsManager::get(STR_PREF_AUDIO_AMP);
  if (audio_amp.equals("1"))
  {
    _is_audio_amp_en = true;
    ccpu_cmd_data[1] = CH_PIN_LORA_PWR;
    _ccpu.sendCmd(ccpu_cmd_data, sizeof(ccpu_cmd_data), 2);
  }

  indexPlaylists();
  showPlaylistsTmpl();
  fillPlaylists();
}

Mp3Context::~Mp3Context()
{
  setCpuFrequencyMhz(BASE_CPU_FREQ_MHZ);
}

//-------------------------------------------------------------------------------------------

void Mp3Context::showPlaying()
{
  IWidgetContainer* container = getLayout();

  if (container)
    container->delWidgets();

  uint8_t paddings{10};

  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _track_name_lbl = new Label(ID_TRACK_NAME);
  layout->addWidget(_track_name_lbl);
  _track_name_lbl->setText(_track_name);
  _track_name_lbl->setAlign(IWidget::ALIGN_CENTER);
  _track_name_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _track_name_lbl->setBackColor(COLOR_BLACK);
  _track_name_lbl->setTextColor(COLOR_ORANGE);
  _track_name_lbl->setWidth(UI_WIDTH);
  _track_name_lbl->setHeight(20);
  _track_name_lbl->setAutoscroll(true);
  _track_name_lbl->setPos(getCenterX(_track_name_lbl), 0);

  _progress = new ProgressBar(ID_PROGRESS);
  layout->addWidget(_progress);
  _progress->setTouchable(true);
  _progress->setBackColor(COLOR_BLACK);
  _progress->setProgressColor(COLOR_ORANGE);
  _progress->setBorderColor(COLOR_WHITE);
  _progress->setMax(9999);
  _progress->setWidth(UI_WIDTH);
  _progress->setHeight(20);
  _progress->setProgress(1);
  _progress->setPos(0, UI_HEIGHT - _progress->getHeight());

  _cur_track_time_lbl = _track_name_lbl->clone(ID_CUR_TRACK_TIME);
  layout->addWidget(_cur_track_time_lbl);
  _cur_track_time_lbl->setText(STR_ZERO_TRACK_TIME);
  _cur_track_time_lbl->setAlign(IWidget::ALIGN_END);
  _cur_track_time_lbl->initWidthToFit();
  _cur_track_time_lbl->setBackColor(COLOR_MAIN_BACK);
  _cur_track_time_lbl->setPos(paddings, _progress->getYPos() - 25);
  _cur_track_time_lbl->setAutoscroll(false);

  _gen_track_time_lbl = _cur_track_time_lbl->clone(ID_GEN_TRACK_TIME);
  layout->addWidget(_gen_track_time_lbl);
  _gen_track_time_lbl->setText(STR_ZERO_TRACK_TIME);
  _gen_track_time_lbl->initWidthToFit();
  _gen_track_time_lbl->setPos(UI_WIDTH - _cur_track_time_lbl->getWidth() - paddings, _cur_track_time_lbl->getYPos());

  //

  _play_btn = new Image(ID_PLAY_BTN);
  layout->addWidget(_play_btn);
  _play_btn->setTouchable(true);
  _play_btn->setWidth(32);
  _play_btn->setHeight(32);
  _play_btn->setSrc(PAUSE_IMG);
  _play_btn->setBackColor(COLOR_MAIN_BACK);
  _play_btn->setTransparency(true);
  _play_btn->setPos(getCenterX(_play_btn), getCenterY(_play_btn));

  Image* forward_img = new Image(ID_FORWARD_IMG);
  layout->addWidget(forward_img);
  forward_img->setTouchable(true);
  forward_img->setWidth(24);
  forward_img->setHeight(24);
  forward_img->setSrc(FORWARD_IMG);
  forward_img->setBackColor(COLOR_MAIN_BACK);
  forward_img->setTransparency(true);
  forward_img->setPos(_play_btn->getXPos() + 32 * 2, _play_btn->getYPos() + 4);

  Image* rewind_img = new Image(ID_REWIND_IMG);
  layout->addWidget(rewind_img);
  rewind_img->setTouchable(true);
  rewind_img->setWidth(24);
  rewind_img->setHeight(24);
  rewind_img->setSrc(REWIND_IMG);
  rewind_img->setBackColor(COLOR_MAIN_BACK);
  rewind_img->setTransparency(true);
  rewind_img->setPos(_play_btn->getXPos() - 32 - 24, _play_btn->getYPos() + 4);

  //

  _volume_lbl = new Label(ID_VOLUME_LBL);
  layout->addWidget(_volume_lbl);
  _volume_lbl->setText(String(_volume));
  _volume_lbl->setAlign(IWidget::ALIGN_END);
  _volume_lbl->setGravity(IWidget::GRAVITY_CENTER);
  _volume_lbl->setHeight(15);
  _volume_lbl->setWidth(20);
  _volume_lbl->setBackColor(COLOR_MAIN_BACK);
  _volume_lbl->setTextColor(COLOR_ORANGE);
  _volume_lbl->setPos(getCenterX(_volume_lbl) + _volume_lbl->getWidth() / 2, _progress->getYPos() - _volume_lbl->getHeight() - 5);

  Image* volume_img = new Image(ID_VOLUME_IMG);
  layout->addWidget(volume_img);
  volume_img->setWidth(16);
  volume_img->setHeight(16);
  volume_img->setSrc(SPEAKER_IMG);
  volume_img->setBackColor(COLOR_MAIN_BACK);
  volume_img->setTransparency(true);
  volume_img->setPos(getCenterX(volume_img) - volume_img->getWidth() / 2, _volume_lbl->getYPos());

  _mode = MODE_AUDIO_PLAY;

  setLayout(layout);

  // Необхідно для повторного оновлення довжини поточного треку
  // після увімкнення підсвітки
  _is_new_track = true;

  layout->drawForced();
}

void Mp3Context::showTracksTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _tracks_list = WidgetCreator::getDynamicMenu(ID_D_MENU);
  layout->addWidget(_tracks_list);
  _tracks_list->setWidth(UI_WIDTH - SCROLLBAR_WIDTH);
  _tracks_list->setHeight(UI_HEIGHT);
  _tracks_list->setItemHeight((_tracks_list->getHeight() - 2) / TRACKS_ITEMS_NUM);

  _tracks_list->setOnNextItemsLoadHandler(onNextItemsLoad, this);
  _tracks_list->setOnPrevItemsLoadHandler(onPrevItemsLoad, this);

  _mode = MODE_TRACK_SEL;

  setLayout(layout);
}

void Mp3Context::showPlaylistsTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _playlists_list = new FixedMenu(ID_F_MENU);
  layout->addWidget(_playlists_list);
  _playlists_list->setBackColor(COLOR_MENU_ITEM);
  _playlists_list->setWidth(UI_WIDTH - SCROLLBAR_WIDTH);
  _playlists_list->setHeight(UI_HEIGHT);
  _playlists_list->setItemHeight((UI_HEIGHT - 2) / PLAYLIST_ITEMS_NUM);

  if (!_track_name.isEmpty())
  {
    MenuItem* cont_item = WidgetCreator::getMenuItem(ID_CONT_ITEM);
    _playlists_list->addItem(cont_item);

    Label* cont_lbl = WidgetCreator::getItemLabel(STR_CONTINUE, font_10x20);
    cont_item->setLbl(cont_lbl);
  }

  _mode = MODE_PLST_SEL;

  setLayout(layout);
}

void Mp3Context::fillPlaylists()
{
  std::vector<MenuItem*> items;
  makeMenuPlaylistsItems(items);

  uint16_t size = items.size();

  for (size_t i = 0; i < size; ++i)
    _playlists_list->addItem(items[i]);
}

void Mp3Context::makeMenuPlaylistsItems(std::vector<MenuItem*>& items)
{
  items.clear();

  uint16_t playlist_num = _playlists.size();
  items.reserve(playlist_num);

  for (uint16_t i = 0, counter = ID_CONT_ITEM; i < playlist_num; ++i)
  {
    ++counter;
    MenuItem* item = WidgetCreator::getMenuItem(counter);
    items.push_back(item);

    Label* lbl = new Label(1);
    item->setLbl(lbl);
    lbl->setAutoscrollInFocus(true);
    lbl->setFont(font_10x20);
    lbl->setText(_playlists[i].getName());
  }
}

void Mp3Context::fillTracks(uint16_t track_pos)
{
  _tracks_list->delWidgets();
  size_t pl_sz = _tracks.size();

  if (pl_sz > 0 && track_pos >= pl_sz)
    --track_pos;

  std::vector<MenuItem*> items;
  makeMenuTracksItems(items, track_pos, _tracks_list->getItemsPerPage());

  size_t items_size = items.size();

  for (size_t i = 0; i < items_size; ++i)
    _tracks_list->addItem(items[i]);
}

void Mp3Context::makeMenuTracksItems(std::vector<MenuItem*>& items, uint16_t file_pos, uint8_t size)
{
  if (file_pos >= _tracks.size())
    return;

  uint16_t read_to = file_pos + size;

  if (read_to > _tracks.size())
    read_to = _tracks.size();

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

    lbl->setText(_tracks[i].getName());
  }
}

void Mp3Context::showPlMenu()
{
  if (_track_item_id == 0)
    return;

  _tracks_list->disable();

  _context_menu = new FixedMenu(ID_PL_MENU);
  getLayout()->addWidget(_context_menu);
  _context_menu->setBackColor(COLOR_MENU_ITEM);
  _context_menu->setBorderColor(COLOR_ORANGE);
  _context_menu->setBorder(true);
  _context_menu->setItemHeight(20);
  _context_menu->setWidth(120);
  _context_menu->setHeight(44);
  _context_menu->setPos(UI_WIDTH - _context_menu->getWidth(), UI_HEIGHT - _context_menu->getHeight());

  MenuItem* del_item = WidgetCreator::getMenuItem(ID_ITEM_DEL);
  _context_menu->addItem(del_item);

  Label* upd_lbl = WidgetCreator::getItemLabel(STR_DELETE);
  del_item->setLbl(upd_lbl);

  _mode = MODE_PLST_MENU;
}

void Mp3Context::hidePlMenu()
{
  getLayout()->delWidgetByID(ID_PL_MENU);
  _mode = MODE_TRACK_SEL;
  _tracks_list->enable();
}

void Mp3Context::showSDErrTmpl()
{
  IWidgetContainer* layout = WidgetCreator::getEmptyLayout();

  _msg_lbl = WidgetCreator::getStatusMsgLable(ID_MSG_LBL, STR_SD_ERR);
  layout->addWidget(_msg_lbl);

  _mode = MODE_SD_UNCONN;
  setLayout(layout);
}

void Mp3Context::updateTrackPos()
{
  if (_track_pos > 0)
  {
    if (_track_pos >= _tracks.size())
      _track_pos = _tracks.size() - 1;
  }
}

//-------------------------------------------------------------------------------------------

void Mp3Context::update()
{
  ITouchscreen::Swipe swipe = _input.getSwipe();

  if (_mode == MODE_SD_UNCONN)
  {
    if (swipe == ITouchscreen::SWIPE_L)
    {
      openContextByID(ID_CONTEXT_MENU);
    }

    return;
  }

  if (swipe == ITouchscreen::SWIPE_L)
  {
    _input.lock(CLICK_LOCK);
    back();
  }
  else if (swipe == ITouchscreen::SWIPE_U)
  {
    _input.lock(CLICK_LOCK);
    up();
  }
  else if (swipe == ITouchscreen::SWIPE_D)
  {
    _input.lock(CLICK_LOCK);
    down();
  }
  else if (_input.isPressed())
  {
    _input.lock(PRESS_LOCK);
    okPressed();
  }
  else if (_input.isReleased())
  {
    _input.lock(CLICK_LOCK);
    click();
  }

  if (_mode == MODE_AUDIO_PLAY)
  {
    if (_audio.isRunning())
    {
      if (_is_new_track && updateTrackDuration())
      {
        _track_name_lbl->setText(_track_name);
        _is_new_track = false;
      }

      if (millis() - _upd_msg_time > UPD_TRACK_INF_INTERVAL)
      {
        updateTrackTime();
        _upd_msg_time = millis();
      }
    }
    // Якщо трек скінчився самостійно
    else if (_is_playing)
    {
      // Намагаємося перемкнути
      if (playNext())
      {
        _is_new_track = true;
      }
      // Якщо не вдалося змінити трек з 3х спроб, зупинити плеєр
      else if (_attempt_to_play_next_counter == 3)
      {
        setStopState();
      }
      else
        ++_attempt_to_play_next_counter;
    }
  }
}

//-------------------------------------------------------------------------------------------

bool Mp3Context::playNext()
{
  if (_track_pos >= _tracks.size() - 1)
    return false;

  _track_name = _tracks[_track_pos + 1].getName();

  if (!playTrack(false))
    return false;

  ++_track_pos;
  return true;
}

bool Mp3Context::playPrev()
{
  if (_tracks.empty() || _track_pos == 0)
    return false;

  _track_name = _tracks[_track_pos - 1].getName();

  if (!playTrack(false))
    return false;

  --_track_pos;
  return true;
}

//-------------------------------------------------------------------------------------------

bool Mp3Context::playTrack(bool contn)
{
  if (contn)
  {
    if ((_playlist_name.isEmpty() || _track_name.isEmpty()))
      return false;
  }
  else
  {
    _track_time = 0;
  }

  String track_path = getTrackPath(_playlist_name.c_str(), _track_name.c_str());

  if (!_audio.connecttoFS(track_path.c_str(), _track_time))
  {
    if (_mode == MODE_AUDIO_PLAY)
      setStopState();
    return false;
  }

  _is_new_track = true;
  _is_playing = true;
  showPlaying();
  return true;
}

void Mp3Context::volumeUp()
{
  if (_volume < _audio.maxVolume() - 2)
  {
    if (_volume < 10)
      _volume += 1;
    else
      _volume += 2;

    _audio.setVolume(_volume);
  }

  _volume_lbl->setText(String(_volume));
}

void Mp3Context::volumeDown()
{
  if (_volume > 1)
  {
    if (_volume > 10)
      _volume -= 2;
    else
      _volume -= 1;

    _audio.setVolume(_volume);
  }

  _volume_lbl->setText(String(_volume));
}

void Mp3Context::setStopState()
{
  _audio.stopSong();

  _is_playing = false;
  _track_pos = 0;
  _track_name = "";

  _play_btn->setSrc(PLAY_IMG);
  _track_name_lbl->setText(STR_STOPPED);
  _cur_track_time_lbl->setText(STR_ZERO_TRACK_TIME);
  _gen_track_time_lbl->setText(STR_ZERO_TRACK_TIME);
  _progress->reset();
}

bool Mp3Context::updateTrackDuration()
{
  uint32_t duration = _audio.getAudioFileDuration();

  if (duration == 0)
    return false;

  uint32_t minutes = floor((float)duration / 60);
  String track_time{""};

  if (minutes < 100)
    track_time += "0";

  if (minutes < 10)
    track_time += "0";

  track_time += minutes;

  track_time += ":";

  uint32_t sec = duration - minutes * 60;
  if (sec < 10)
    track_time += "0";

  track_time += sec;

  _gen_track_time_lbl->setText(track_time);

  _progress->setProgress(0);
  _progress->setMax(duration);

  return true;
}

void Mp3Context::updateTrackTime()
{
  _track_time = _audio.getAudioCurrentTime();

  uint32_t minutes = floor((float)_track_time / 60);
  String track_time_str;

  if (minutes < 100)
    track_time_str += "0";

  if (minutes < 10)
    track_time_str += "0";

  track_time_str += String(minutes);

  track_time_str += ":";

  uint32_t sec = _track_time - minutes * 60;

  if (sec < 10)
    track_time_str += "0";

  track_time_str += String(sec);

  _cur_track_time_lbl->setText(track_time_str);

  _progress->setProgress(_track_time);
}

void Mp3Context::setTrackPosAt(uint16_t x, uint16_t y)
{
  uint32_t pos = _progress->getProgressAt(x, y);

  if (pos == 0)
    return;

  _audio.setAudioPlayPosition(pos);
}

void Mp3Context::up()
{
  if (_mode == MODE_PLST_SEL)
  {
    _playlists_list->pageUp();
  }
  else if (_mode == MODE_TRACK_SEL)
  {
    _tracks_list->pageUp();
  }
  else if (_mode == MODE_AUDIO_PLAY)
  {
    volumeUp();
  }
  else if (_mode == MODE_PLST_MENU)
  {
    _context_menu->pageUp();
  }
}

void Mp3Context::down()
{
  if (_mode == MODE_PLST_SEL)
  {
    _playlists_list->pageDown();
  }
  else if (_mode == MODE_TRACK_SEL)
  {
    _tracks_list->pageDown();
  }
  else if (_mode == MODE_AUDIO_PLAY)
  {
    volumeDown();
  }
  else if (_mode == MODE_PLST_MENU)
  {
    _context_menu->pageDown();
  }
}

void Mp3Context::click()
{
  if (_mode == MODE_PLST_SEL)
  {
    uint16_t item_ID = getSelectedItemID(_playlists_list);

    if (item_ID == ID_CONT_ITEM)
    {
      indexTracks();
      playTrack(true);
    }
    else
    {
      _playlist_name = getSelectedItemText(_playlists_list);
      _track_pos = 0;
      indexTracks();
      showTracksTmpl();
      fillTracks(_track_pos);
    }
  }
  else if (_mode == MODE_TRACK_SEL)
  {
    if (_tracks_list->getSize() > 0)
    {
      _track_name = getSelectedItemText(_tracks_list);
      _track_pos = getSelectedItemID(_tracks_list) - 1;
      playTrack(false);
    }
  }
  else if (_mode == MODE_AUDIO_PLAY)
  {
    if (!_track_name.equals(""))
    {
      uint16_t x = _input.getTouchX();
      uint16_t y = _input.getTouchY();

      IWidget* click_widget = getLayout()->findTouchableAt(x, y);
      if (!click_widget)
        return;

      uint16_t widget_id = click_widget->getID();

      switch (widget_id)
      {
        case ID_PLAY_BTN:
          _audio.pauseResume();

          if (_audio.isRunning())
          {
            _play_btn->setSrc(PAUSE_IMG);
            _is_playing = true;
          }
          else
          {
            _play_btn->setSrc(PLAY_IMG);
            _is_playing = false;
          }
          break;
        case ID_FORWARD_IMG:
          playNext();
          break;
        case ID_REWIND_IMG:
          playPrev();
          break;
        case ID_PROGRESS:
          setTrackPosAt(x, y);
          break;
        default:
          break;
      }
    }
  }
  else if (_mode == MODE_PLST_MENU)
  {
    uint16_t id = getSelectedItemID(_context_menu);

    if (id == ID_ITEM_DEL)
    {
      if (_track_name.isEmpty())
        return;

      String path_to_rem = getTrackPath(_playlist_name.c_str(), _track_name.c_str());

      if (_fs.rmFile(path_to_rem.c_str()))
      {
        if (_track_item_id - 2 > -1)
          _track_pos = _track_item_id - 2;
        else
          _track_pos = 0;

        hidePlMenu();
        indexTracks();
        updateTrackPos();
        fillTracks(_track_pos);
      }
    }
  }
}

void Mp3Context::indexPlaylists()
{
  String playlists_path = ROOT_PATH;
  _fs.indexDirs(_playlists, playlists_path.c_str());
}

void Mp3Context::indexTracks()
{
  if (_playlist_name.isEmpty())
    return;

  String playlist_path = ROOT_PATH;
  playlist_path += "/";
  playlist_path += _playlist_name;
  _fs.indexFilesByExt(_tracks, playlist_path.c_str(), {AUDIO_EXT});
}

IWidget* Mp3Context::getSelectedItem(IMenu* menu)
{
  if (!menu)
    return nullptr;

  if (menu->getSize() == 0)
    return nullptr;

  uint16_t x = _input.getTouchX();
  uint16_t y = _input.getTouchY();

  return menu->findTouchableAt(x, y);
}

String Mp3Context::getSelectedItemText(IMenu* menu)
{
  IWidget* raw_item = getSelectedItem(menu);
  if (!raw_item)
    return emptyString;

  MenuItem* item = static_cast<MenuItem*>(raw_item);
  return item->getText();
}

uint16_t Mp3Context::getSelectedItemID(IMenu* menu)
{
  IWidget* raw_item = getSelectedItem(menu);
  if (!raw_item)
    return 0;

  return raw_item->getID();
}

String Mp3Context::getTrackPath(const char* dirname, const char* track_name) const
{
  String track_path = ROOT_PATH;
  track_path += "/";
  track_path += _playlist_name;
  track_path += "/";
  track_path += _track_name;
  return track_path;
}

void Mp3Context::handleNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  if (!cur_id)
    return;

  makeMenuTracksItems(items, cur_id, size);
}

void Mp3Context::onNextItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  Mp3Context* self = static_cast<Mp3Context*>(arg);
  self->handleNextItemsLoad(items, size, cur_id);
}

void Mp3Context::handlePrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id)
{
  if (!cur_id)
    return;

  uint16_t item_pos = cur_id - 1;

  if (!item_pos)
    return;

  if (cur_id > size)
  {
    item_pos = cur_id - size - 1;
  }
  else
  {
    item_pos = 0;
  }

  makeMenuTracksItems(items, item_pos, size);
}

void Mp3Context::onPrevItemsLoad(std::vector<MenuItem*>& items, uint8_t size, uint16_t cur_id, void* arg)
{
  Mp3Context* self = static_cast<Mp3Context*>(arg);
  self->handlePrevItemsLoad(items, size, cur_id);
}

void Mp3Context::okPressed()
{
  if (_mode == MODE_TRACK_SEL)
  {
    _track_name = getSelectedItemText(_tracks_list);
    _track_item_id = getSelectedItemID(_tracks_list);
    showPlMenu();
  }
}

void Mp3Context::back()
{
  if (_mode == MODE_PLST_SEL)
  {
    openContextByID(ID_CONTEXT_MENU);
  }
  else if (_mode == MODE_TRACK_SEL)
  {
    indexPlaylists();
    showPlaylistsTmpl();
    fillPlaylists();
  }
  else if (_mode == MODE_PLST_MENU)
  {
    hidePlMenu();
  }
  else if (_mode == MODE_AUDIO_PLAY)
  {
    _audio.stopSong();
    savePref();
    showTracksTmpl();
    fillTracks(_track_pos);
  }
}

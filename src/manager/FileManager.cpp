#pragma GCC optimize("O3")
#include "FileManager.h"

#include <errno.h>
#include <esp_task_wdt.h>
#include <sd_diskio.h>

#include <cstring>
#include <vector>

#include "bus/SPI_Bus.h"
#include "util/MutexGuard.h"

#ifdef SD_TYPE_MMC
// clang-format off
#include <ff.h>
#include <diskio_sdmmc.h>
// clang-format on
#include <driver/sdmmc_default_configs.h>
#include <driver/sdmmc_defs.h>
#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <pins_arduino.h>
#include <sdmmc_cmd.h>
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include <sd_pwr_ctrl.h>
#include <sd_pwr_ctrl_by_on_chip_ldo.h>
#endif
#endif  // #ifdef SD_TYPE_MMC

#define IDLE_WD_GUARD_TIME 250U
#define OPT_BLOCK_SIZE 16384

namespace pixeler
{
  String FileManager::makeFullPath(const char* path)
  {
    String full_path = SD_MOUNTPOINT;
    full_path += path;
    return full_path;
  }

  String FileManager::makeUniqueFilename(const String& file_path)
  {
    uint16_t counter = 1;
    String temp_path = file_path;
    String unique_filename = file_path;
    while (fileExist(unique_filename.c_str(), true))
    {
      unique_filename = temp_path.substring(0, temp_path.lastIndexOf("."));
      unique_filename += "(";
      unique_filename += counter;
      unique_filename += ")";
      unique_filename += temp_path.substring(temp_path.lastIndexOf("."));
      ++counter;
    }

    return unique_filename;
  }

  FileManager::FileManager()
  {
  }

  uint8_t FileManager::getEntryTypeUnlocked(const char* path, dirent* entry)
  {
    if (entry && entry->d_type != DT_UNKNOWN)
      return entry->d_type;

    struct stat st;
    if (stat(path, &st) == 0)
    {
      if (S_ISREG(st.st_mode))
        return DT_REG;
      else if (S_ISDIR(st.st_mode))
        return DT_DIR;
    }

    return DT_UNKNOWN;
  }

  size_t FileManager::getFileSize(const char* path)
  {
    MutexGuard lock(_sd_mutex);
    return getFileSizeUnlocked(path);
  }

  size_t FileManager::getFileSizeUnlocked(const char* path)
  {
    String full_path = makeFullPath(path);
    struct stat st;
    if (stat(full_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
      return 0;

    return static_cast<size_t>(st.st_size);
  }

  bool FileManager::readStat(struct stat& out_stat, const char* path)
  {
    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);
    if (stat(full_path.c_str(), &out_stat) != 0)
      return false;

    return true;
  }

  bool FileManager::fileExist(const char* path, bool silently)
  {
    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);
    bool result = getEntryTypeUnlocked(full_path.c_str()) == DT_REG;

    if (!result && !silently)
      log_e("File %s not found", full_path.c_str());

    return result;
  }

  bool FileManager::dirExist(const char* path, bool silently)
  {
    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);
    bool result = getEntryTypeUnlocked(full_path.c_str()) == DT_DIR;

    if (!result && !silently)
      log_e("Dir %s not found", full_path.c_str());

    return result;
  }

  bool FileManager::exists(const char* path, bool silently)
  {
    String full_path = makeFullPath(path);
    MutexGuard lock(_sd_mutex);
    uint8_t type = getEntryTypeUnlocked(full_path.c_str());

    if (type == DT_REG || type == DT_DIR)
      return true;

    log_e("[ %s ] not exist", full_path.c_str());
    return false;
  }

  bool FileManager::createDir(const char* path)
  {
    String full_path = makeFullPath(path);

    errno = 0;

    MutexGuard lock(_sd_mutex);
    bool result = !mkdir(full_path.c_str(), 0777);

    if (!result)
    {
      log_e("Помилка створення директорії: %s", full_path.c_str());
      if (errno == EEXIST)
        log_e("Директорія існує");
    }

    return result;
  }

  size_t FileManager::readFile(const char* path, void* out_buffer, size_t len, int32_t seek_pos)
  {
    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);

    int fd = open(full_path.c_str(), O_RDONLY);
    if (fd < 0)
    {
      log_e("Помилка відкриття файлу: %s", full_path.c_str());
      return 0;
    }

    if (seek_pos > 0)
    {
      off_t result = lseek(fd, seek_pos, SEEK_SET);
      if (result == -1)
      {
        log_e("Помилка встановлення позиції(%d) у файлі %s", seek_pos, full_path.c_str());
        close(fd);
        return 0;
      }
    }

    ssize_t bytes_read = read(fd, out_buffer, len);
    if (bytes_read < 0)
    {
      log_e("Помилка читання файлу %s", full_path.c_str());
      close(fd);
      return 0;
    }

    if (bytes_read != static_cast<ssize_t>(len))
      log_e("Прочитано: [ %zd ]  Очікувалося: [ %zu ]", bytes_read, len);

    close(fd);
    return bytes_read;
  }

  size_t FileManager::readFromFile(FILE* file, void* out_buffer, size_t len, size_t seek_pos)
  {
    if (!file)
    {
      log_e("FILE* не повинен бути null");
      return 0;
    }

    if (len == 0)
      return 0;

    int fd = fileno(file);

    MutexGuard lock(_sd_mutex);

    if (seek_pos > 0)
    {
      off_t result = lseek(fd, seek_pos, SEEK_SET);
      if (result == -1)
      {
        log_e("Помилка встановлення позиції(%zu) у файлі", seek_pos);
        return 0;
      }
    }

    ssize_t bytes_read = read(fd, out_buffer, len);
    if (bytes_read < 0)
    {
      log_e("Помилка читання з файлу");
      return 0;
    }

    if (bytes_read != static_cast<ssize_t>(len))
      log_e("Прочитано: [ %zd ]  Очікувалося: [ %zu ]", bytes_read, len);

    return bytes_read;
  }

  String FileManager::readFileToStr(String path)
  {
    if (path.isEmpty())
      return emptyString;

    size_t file_size = getFileSize(path.c_str());

    if (file_size == 0)
      return emptyString;

    char* buffer = static_cast<char*>(malloc(file_size + 1));

    if (!buffer)
    {
      log_e("Bad memory alloc: %zu b", file_size);
      return emptyString;
    }

    size_t bytes_read = readFile(path.c_str(), buffer, file_size);
    buffer[bytes_read] = '\0';

    String ret;
    if (bytes_read > 0)
      ret = buffer;

    free(buffer);
    return ret;
  }

  bool FileManager::readFromFileExact(FILE* file, void* out_buffer, size_t len, size_t seek_pos)
  {
    return readFromFile(file, out_buffer, len, seek_pos) == static_cast<ssize_t>(len);
  }

  size_t FileManager::writeFile(const char* path, const void* buffer, size_t len)
  {
    if (len == 0)
      return 0;

    if (!path || !buffer)
    {
      log_e("Bad arguments");
      return 0;
    }

    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);

    int fd = open(full_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
    {
      log_e("Помилка відкриття файлу: %s", full_path.c_str());
      return 0;
    }

    size_t written = writeOptimalUnlocked(fd, buffer, len);

    fsync(fd);
    close(fd);

    return written;
  }

  size_t FileManager::writeToFile(FILE* file, const void* buffer, size_t len)
  {
    if (len == 0)
      return 0;

    if (!file || !buffer)
    {
      log_e("Bad arguments");
      return 0;
    }

    MutexGuard lock(_sd_mutex);
    int fd = fileno(file);
    return writeOptimalUnlocked(fd, buffer, len);
  }

  size_t FileManager::writeOptimalUnlocked(int file_desc, const void* buffer, size_t len)
  {
    size_t total_written = 0;
    const uint8_t* ptr = static_cast<const uint8_t*>(buffer);

    size_t full_blocks = len / OPT_BLOCK_SIZE;
    size_t remaining_bytes = len % OPT_BLOCK_SIZE;

    unsigned long ts = millis();
    for (size_t i = 0; i < full_blocks; ++i)
    {
      ssize_t res = write(file_desc, ptr + total_written, OPT_BLOCK_SIZE);
      if (res == -1)
        break;

      total_written += res;

      if (millis() - ts > IDLE_WD_GUARD_TIME)
      {
        delay(1);
        ts = millis();
      }
    }

    if (total_written == (full_blocks * OPT_BLOCK_SIZE) && remaining_bytes > 0)
    {
      ssize_t res = write(file_desc, ptr + total_written, remaining_bytes);
      if (res != -1)
        total_written += res;
    }

    if (total_written != len)
      log_e("Записано: [ %zu ]  Очікувалося: [ %zu ]", total_written, len);

    return total_written;
  }

  FILE* FileManager::openFile(const char* path, const char* mode)
  {
    String full_path = makeFullPath(path);

    MutexGuard lock(_sd_mutex);
    FILE* f = fopen(full_path.c_str(), mode);

    if (!f)
      log_e("Помилка взяття дескриптора для %s", full_path.c_str());

    return f;
  }

  void FileManager::closeFile(FILE*& file)
  {
    if (file)
    {
      MutexGuard lock(_sd_mutex);
      closeFileUnlocked(file);
    }
  }

  void FileManager::closeFileUnlocked(FILE*& file)
  {
    int fd = fileno(file);
    fsync(fd);
    fclose(file);
    file = nullptr;
  }

  bool FileManager::seekPos(FILE* file, int32_t pos, uint8_t mode)
  {
    if (!file)
      return false;

    int fd = fileno(file);

    MutexGuard lock(_sd_mutex);

    off_t result = lseek(fd, pos, mode);
    if (result == -1)
    {
      log_e("Помилка встановлення позиції [%d]", pos);
      return false;
    }

    return true;
  }

  size_t FileManager::getPos(FILE* file)
  {
    if (!file)
      return 0;

    int fd = fileno(file);

    MutexGuard lock(_sd_mutex);

    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == -1)
      return 0;

    return pos;
  }

  size_t FileManager::available(FILE* file, size_t file_size)
  {
    MutexGuard lock(_sd_mutex);
    return availableUnlocked(file, file_size);
  }

  uint8_t FileManager::getCopyProgress() const
  {
    return _copy_progress;
  }

  size_t FileManager::availableUnlocked(FILE* file, size_t file_size)
  {
    if (!file)
      return 0;

    int fd = fileno(file);

    off_t current_pos = lseek(fd, 0, SEEK_CUR);
    if (current_pos == -1)
      return 0;

    if (file_size < static_cast<size_t>(current_pos))
      return 0;

    return file_size - current_pos;
  }

  void FileManager::rm()
  {
    bool result = false;

    String full_path = makeFullPath(_rm_path.c_str());

    bool is_dir = dirExist(_rm_path.c_str(), true);

    if (!is_dir)
    {
      MutexGuard lock(_sd_mutex);
      result = rmFileUnlocked(full_path.c_str());
    }
    else
    {
      bool was_mutex_taken = false;
      result = rmDirRecursively(full_path.c_str(), was_mutex_taken);
      if (was_mutex_taken)
        xSemaphoreGive(_sd_mutex);
    }

    if (result)
      log_i("Успішно видалено: %s", full_path.c_str());

    taskDone(result);
  }

  bool FileManager::rmDirRecursively(const char* path, bool& was_mutex_taken, bool make_full)
  {
    bool result = false;

    dirent* dir_entry{nullptr};
    DIR* dir;

    if (!was_mutex_taken)
    {
      xSemaphoreTake(_sd_mutex, portMAX_DELAY);
      was_mutex_taken = true;
    }

    if (make_full)
    {
      String full_path = makeFullPath(path);
      dir = opendir(full_path.c_str());
    }
    else
    {
      dir = opendir(path);
    }

    if (dir)
    {
      errno = 0;
      _ts = millis();
      while (!_is_canceled)
      {
        if (!was_mutex_taken)
        {
          xSemaphoreTake(_sd_mutex, portMAX_DELAY);
          was_mutex_taken = true;
        }

        dir_entry = readdir(dir);
        if (!dir_entry)
        {
          if (!errno)
            result = true;
          break;
        }

        if (std::strcmp(dir_entry->d_name, ".") == 0 || std::strcmp(dir_entry->d_name, "..") == 0)
          continue;

        String full_path = path;
        full_path += "/";
        full_path += dir_entry->d_name;

        uint8_t entr_type = getEntryTypeUnlocked(full_path.c_str(), dir_entry);

        if (entr_type == DT_REG)
        {
          result = rmFileUnlocked(full_path.c_str());
        }
        else if (entr_type == DT_DIR)
        {
          result = rmDirRecursively(full_path.c_str(), was_mutex_taken);
        }
        else
        {
          log_e("Невідомий тип або не вдалося прочитати: %s", full_path.c_str());
          result = false;
        }

        if (!result)
          break;

        if (millis() - _ts > IDLE_WD_GUARD_TIME)
        {
          xSemaphoreGive(_sd_mutex);
          was_mutex_taken = false;
          delay(1);
          _ts = millis();
        }
      }

      closedir(dir);
    }

    if (result)
      result = !remove(path);
    else
      log_e("Помилка під час видалення: %s", path);

    return result;
  }

  bool FileManager::rmDir(const char* path)
  {
    _is_canceled = false;
    bool was_mutex_taken = false;
    bool result = rmDirRecursively(path, was_mutex_taken, true);
    if (was_mutex_taken)
      xSemaphoreGive(_sd_mutex);
    return result;
  }

  bool FileManager::rmFile(const char* path)
  {
    MutexGuard lock(_sd_mutex);
    return rmFileUnlocked(path, true);
  }

  bool FileManager::rmFileUnlocked(const char* path, bool make_full)
  {
    bool result;

    if (make_full)
    {
      String full_path = makeFullPath(path);
      result = !remove(full_path.c_str());

      if (!result)
        log_e("Помилка видалення файлу: %s", full_path.c_str());
    }
    else
    {
      result = !remove(path);

      if (!result)
        log_e("Помилка видалення файлу: %s", path);
    }

    return result;
  }

  void FileManager::rmTask(void* params)
  {
    FileManager* instance = static_cast<FileManager*>(params);
    instance->rm();
  }

  bool FileManager::startRemoving(const char* path)
  {
    if (_is_working)
    {
      log_e("Вже працює інша задача");
      return false;
    }

    if (!exists(path))
      return false;

    _rm_path = path;
    _is_canceled = false;

    BaseType_t result = xTaskCreatePinnedToCore(rmTask, "rmTask", TASK_SIZE, this, 10, nullptr, 0);

    if (result == pdPASS)
    {
      log_i("rmTask is working now");
      _is_working = true;
      return true;
    }
    else
    {
      log_e("rmTask was not running");
      return false;
    }
  }

  bool FileManager::rename(const char* old_name, const char* new_name)
  {
    if (!exists(old_name))
      return false;

    String old_n = makeFullPath(old_name);
    String new_n = makeFullPath(new_name);

    if (new_n.length() >= old_n.length() &&
        (new_n.c_str()[old_n.length()] == '/' || new_n.c_str()[old_n.length()] == '\0') &&
        strncmp(old_n.c_str(), new_n.c_str(), old_n.length()) == 0)
    {
      log_e("Старе і нове ім'я збігаються або спроба переміщення каталогу до самого себе");
      return false;
    }

    MutexGuard lock(_sd_mutex);
    return !::rename(old_n.c_str(), new_n.c_str());
  }

  bool FileManager::createFileCopy(const String& from, const String& to)
  {
    xSemaphoreTake(_sd_mutex, portMAX_DELAY);
    bool was_mutex_taken = true;

    int n_fd = open(to.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);

    if (n_fd < 0)
    {
      log_e("Помилка створення файлу: %s", to.c_str());
      xSemaphoreGive(_sd_mutex);
      return false;
    }

    int o_fd = open(from.c_str(), O_RDONLY);

    if (o_fd < 0)
    {
      log_e("Помилка читання файлу: %s", from.c_str());
      close(n_fd);
      xSemaphoreGive(_sd_mutex);
      return false;
    }

    size_t buf_size = 1024;
    uint8_t* buffer;

    if (psramInit())
    {
      buf_size *= 160;
      buffer = static_cast<uint8_t*>(ps_malloc(buf_size));
    }
    else
    {
      buf_size *= 16;
      buffer = static_cast<uint8_t*>(malloc(buf_size));
    }

    if (!buffer)
    {
      close(n_fd);
      close(o_fd);

      log_e("Помилка виділення пам'яті: %zu b", buf_size);
      esp_restart();
    }

    size_t file_size = getFileSizeUnlocked(_copy_from_path.c_str());
    size_t writed_bytes_counter{0};

    if (file_size > 0)
    {
      log_i("Починаю копіювання");
      log_i("З: %s", from.c_str());
      log_i("До: %s", to.c_str());

      ssize_t bytes_read;

      off_t current_pos = lseek(o_fd, 0, SEEK_CUR);
      size_t byte_aval = file_size;

      _ts = millis();
      while (!_is_canceled && byte_aval > 0)
      {
        if (!was_mutex_taken)
        {
          xSemaphoreTake(_sd_mutex, portMAX_DELAY);
          was_mutex_taken = true;
        }
        size_t to_read = (byte_aval < buf_size) ? byte_aval : buf_size;
        bytes_read = read(o_fd, buffer, to_read);

        if (bytes_read <= 0)
          break;

        writed_bytes_counter += writeOptimalUnlocked(n_fd, buffer, bytes_read);
        _copy_progress = (static_cast<float>(writed_bytes_counter) / file_size) * 100;

        current_pos = lseek(o_fd, 0, SEEK_CUR);
        byte_aval = (current_pos != -1 && file_size > static_cast<size_t>(current_pos)) ? file_size - current_pos : 0;

        if (millis() - _ts > IDLE_WD_GUARD_TIME)
        {
          xSemaphoreGive(_sd_mutex);
          was_mutex_taken = false;
          delay(1);
          _ts = millis();
        }
      }
    }
    else
    {
      _copy_progress = 100;
      delay(50);
    }

    free(buffer);

    fsync(n_fd);
    close(n_fd);
    close(o_fd);

    if (was_mutex_taken)
      xSemaphoreGive(_sd_mutex);

    return writed_bytes_counter == file_size;
  }

  void FileManager::copyFile()
  {
    _copy_to_path = makeUniqueFilename(_copy_to_path);

    String from = makeFullPath(_copy_from_path.c_str());
    String to = makeFullPath(_copy_to_path.c_str());

    bool result = createFileCopy(from, to);

    if (_is_canceled)
    {
      log_i("Копіювання скасовано: %s", from.c_str());

      {
        MutexGuard lock(_sd_mutex);
        rmFileUnlocked(to.c_str());
      }

      taskDone(false);
    }
    else
    {
      if (result)
        log_i("Успішно скопійовано: %s", from.c_str());
      else
        log_i("Невдача копіювання: %s", from.c_str());

      taskDone(result);
    }
  }

  void FileManager::copyFileTask(void* params)
  {
    FileManager* instance = static_cast<FileManager*>(params);
    instance->copyFile();
  }

  bool FileManager::startCopyingFile(const char* from, const char* to)
  {
    if (!from || !to)
    {
      log_e("Bad arguments");
      return false;
    }

    if (_is_working)
    {
      log_e("Вже працює інша задача");
      return false;
    }

    if (!fileExist(from))
      return false;

    _copy_from_path = from;
    _copy_to_path = to;

    _is_canceled = false;
    _copy_progress = 0;

    BaseType_t result = xTaskCreatePinnedToCore(copyFileTask, "copyFileTask", TASK_SIZE, this, 10, nullptr, 0);

    if (result == pdPASS)
    {
      log_i("copyFileTask is working now");
      _is_working = true;
      return true;
    }
    else
    {
      log_e("copyFileTask was not running");
      return false;
    }
  }

  void FileManager::index(std::vector<FileInfo>& out_vec, const char* dir_path, IndexMode mode, const std::vector<String>& file_ext)
  {
    out_vec.clear();
    out_vec.reserve(40);

    if (!dirExist(dir_path))
      return;

    String full_path = makeFullPath(dir_path);

    MutexGuard lock(_sd_mutex);
    DIR* dir = opendir(full_path.c_str());
    if (!dir)
    {
      log_e("Помилка відкриття директорії %s", full_path.c_str());
      return;
    }

    dirent* dir_entry{nullptr};
    String filename;
    bool is_dir;

    unsigned long ts = millis();

    while (1)
    {
      dir_entry = readdir(dir);
      if (!dir_entry)
        break;

      filename = dir_entry->d_name;

      if (filename.equals(".") || filename.equals(".."))
        continue;

      String full_name{full_path};
      full_name += "/";
      full_name += filename;

      uint8_t entr_type = getEntryTypeUnlocked(full_name.c_str(), dir_entry);

      if (entr_type == DT_REG)
        is_dir = false;
      else if (entr_type == DT_DIR)
        is_dir = true;
      else
        continue;

      switch (mode)
      {
        case INDX_MODE_DIR:
          if (is_dir)
            out_vec.emplace_back(filename, true);
          break;
        case INDX_MODE_FILES:
          if (!is_dir)
            out_vec.emplace_back(filename, false);
          break;
        case INDX_MODE_FILES_EXT:
          for (const String& i : file_ext)
          {
            if (!is_dir && filename.endsWith(i))
            {
              out_vec.emplace_back(filename, false);
              break;
            }
          }
          break;
        case INDX_MODE_ALL:
          if (is_dir)
            out_vec.emplace_back(filename, true);
          else
            out_vec.emplace_back(filename, false);
          break;
      }

      if (millis() - ts > IDLE_WD_GUARD_TIME)
      {
        delay(1);
        ts = millis();
      }
    }

    out_vec.shrink_to_fit();
    std::sort(out_vec.begin(), out_vec.end());

    if (dir)
      closedir(dir);
  }

  void FileManager::indexFilesByExt(std::vector<FileInfo>& out_vec, const char* dir_path, const std::vector<String>& file_ext)
  {
    return index(out_vec, dir_path, INDX_MODE_FILES_EXT, file_ext);
  }

  void FileManager::indexFiles(std::vector<FileInfo>& out_vec, const char* dir_path)
  {
    return index(out_vec, dir_path, INDX_MODE_FILES, {});
  }

  void FileManager::indexDirs(std::vector<FileInfo>& out_vec, const char* dir_path)
  {
    return index(out_vec, dir_path, INDX_MODE_DIR, {});
  }

  void FileManager::indexAll(std::vector<FileInfo>& out_vec, const char* dir_path)
  {
    return index(out_vec, dir_path, INDX_MODE_ALL, {});
  }

  void FileManager::taskDone(bool result)
  {
    _is_working = false;

    _last_task_result = result;

    if (_doneHandler)
      _doneHandler(result, _doneArg);

    vTaskDelete(nullptr);
  }

  void FileManager::cancel()
  {
    _is_canceled = true;
  }

  void FileManager::setTaskDoneHandler(TaskDoneHandler handler, void* arg)
  {
    _doneHandler = handler;
    _doneArg = arg;
  }

  bool FileManager::isWorking() const
  {
    return _is_working;
  }

  bool FileManager::lastTaskResult() const
  {
    return _last_task_result;
  }

  //------------------------------------------------------------------------------------------------------------------------

  bool FileManager::isMounted() const
  {
    if (_pdrv == 0xFF)
    {
      log_e("Карту пам'яті не примонтовано");
      return false;
    }

    MutexGuard lock(_sd_mutex);
    return isMountedUnlocked();
  }

  bool FileManager::isMountedUnlocked() const
  {
    String path_to_root = SD_MOUNTPOINT;
    path_to_root += "/";

    struct stat st;
    if (stat(path_to_root.c_str(), &st) != 0)
    {
      log_e("Помилка читання stat під час монтування SD");
      return false;
    }

    return S_ISDIR(st.st_mode);
  }

  void FileManager::enableSdPower()
  {
#ifdef SD_PIN_PWR_ON
    pinMode(SD_PIN_PWR_ON, OUTPUT);
    digitalWrite(SD_PIN_PWR_ON, !SD_PWR_ON_LVL);
    delay(200);

    digitalWrite(SD_PIN_PWR_ON, SD_PWR_ON_LVL);
    delay(200);
#endif  // #ifdef SD_PIN_PWR_ON
  }

  bool FileManager::mount(SemaphoreHandle_t bus_mutex)
  {
    if (_pdrv != 0xFF)
    {
      log_i("Карту пам'яті було примонтовано раніше");
      return true;
    }

    if (_sd_mutex && !_is_ext_lock)
      vSemaphoreDelete(_sd_mutex);

    if (!bus_mutex)
    {
      _sd_mutex = xSemaphoreCreateMutex();
      if (!_sd_mutex)
      {
        log_e("Помилка створення SD-мютекса");
        esp_restart();
      }

      _is_ext_lock = false;
    }
    else
    {
      _sd_mutex = bus_mutex;
      _is_ext_lock = true;
    }

    MutexGuard lock(_sd_mutex);

#ifdef SD_TYPE_SPI
    enableSdPower();

    pinMode(SDSPI_PIN_CS, OUTPUT);
    digitalWrite(SDSPI_PIN_CS, HIGH);

    SPI_Bus::init(SDSPI_BUS, SDSPI_PIN_SCLK, SDSPI_PIN_MISO, SDSPI_PIN_MOSI);
    SPIClass* spi = SPI_Bus::getSpi4Bus(SDSPI_BUS);

    if (!spi || !spi->begin())
    {
      log_e("Некоректна шина SPI або помилка ініціалізації шини");
      return false;
    }

    _pdrv = sdcard_init(SDSPI_PIN_CS, spi, SDSPI_FREQUENCY);
    if (_pdrv == 0xFF)
    {
      log_e("Помилка ініціалізації SD");
      return false;
    }

    if (!sdcard_mount(_pdrv, SD_MOUNTPOINT, SD_MAX_FILES, false))
    {
      sdcard_unmount(_pdrv);
      sdcard_uninit(_pdrv);
      _pdrv = 0xFF;
      log_e("Помилка монтування SD");
      return false;
    }

#else  // #ifdef SD_TYPE_MMC
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

#ifdef SDMMC_MODE_1_BIT
    host.flags = SDMMC_HOST_FLAG_1BIT;
    slot_config.width = 1;
#else
    host.flags = SDMMC_HOST_FLAG_4BIT;
    slot_config.width = 4;
#endif  //  #ifdef SDMMC_MODE_1_BIT

#ifndef SDMMC_SLOT
#error "Не вказано слот SDMMC"
#elif SDMMC_SLOT != SDMMC_HOST_SLOT_0 && defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Чіп підтримує SDMMC тільки на 0-вому слоті"
#elif defined(CONFIG_IDF_TARGET_ESP32S3) || (SDMMC_SLOT != SDMMC_HOST_SLOT_0 && defined(CONFIG_IDF_TARGET_ESP32P4))  // S3 на 0-вому слоті або P4 на 1-му
    host.slot = SDMMC_HOST_SLOT_1;
    slot_config.clk = static_cast<gpio_num_t>(SDMMC_PIN_CLK);
    slot_config.cmd = static_cast<gpio_num_t>(SDMMC_PIN_CMD);
    slot_config.d0 = static_cast<gpio_num_t>(SDMMC_PIN_D0);

#ifndef SDMMC_MODE_1_BIT
    slot_config.d1 = static_cast<gpio_num_t>(SDMMC_PIN_D1);
    slot_config.d2 = static_cast<gpio_num_t>(SDMMC_PIN_D2);
    slot_config.d3 = static_cast<gpio_num_t>(SDMMC_PIN_D3);

#else  // Якщо 1 бітний режим
    slot_config.d1 = GPIO_NUM_0;
    slot_config.d2 = GPIO_NUM_0;
    slot_config.d3 = GPIO_NUM_0;

#endif  //  #ifndef SDMMC_MODE_1_BIT

#else  // P4 на 0-вому або ESP32 на 1-му слоті
    host.slot = SDMMC_HOST_SLOT_0;
    slot_config.clk = GPIO_NUM_0;
    slot_config.cmd = GPIO_NUM_0;
    slot_config.d0 = GPIO_NUM_0;
    slot_config.d1 = GPIO_NUM_0;
    slot_config.d2 = GPIO_NUM_0;
    slot_config.d3 = GPIO_NUM_0;

#endif  // #ifndef SDMMC_SLOT

    slot_config.d4 = GPIO_NUM_0;
    slot_config.d5 = GPIO_NUM_0;
    slot_config.d6 = GPIO_NUM_0;
    slot_config.d7 = GPIO_NUM_0;

    // ---------------------------------------------
#if defined(SDMMC_POWER_CHANNEL) && SDMMC_POWER_CHANNEL != GPIO_NUM_NC
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = SDMMC_POWER_CHANNEL,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = nullptr;

    if (esp_err_t ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle) != ESP_OK)
    {
      log_e("Помилка створення sd_pwr_ctrl драйвера: %s. Підключіть зовнішнє живлення.", ret);
      return false;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;

#endif  // SDMMC_POWER_CHANNEL != GPIO_NUM_NC

    enableSdPower();

    // ---------------------------------------------

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = 0,
        .disk_status_check_enable = false,
        .use_one_fat = false};

    if (esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNTPOINT, &host, &slot_config, &mount_config, &_card) != ESP_OK)
    {
      if (ret == ESP_FAIL)
      {
        log_e("Помилка монтування файлової системи. Відформатуйте карту пам'яті в FAT32.");
      }
      else if (ret == ESP_ERR_INVALID_STATE)
      {
        log_i("SD Вже примонтовано");
        return true;
      }
      else
      {
        log_e("Помилка ініціалізації. Карта пам'яті відсутня або пошкоджена.");
      }
      _card = nullptr;
      return false;
    }

    _pdrv = ff_diskio_get_pdrv_card(_card);

#endif  // #ifdef SD_TYPE_MMC

    // ---------------------------------------------

    delay(10);
    bool result = isMountedUnlocked();

    if (result)
      log_i("Карту пам'яті примонтовано");
    else
      log_e("Помилка перевірки стану монтування SD");

    return result;
  }

  void FileManager::unmount()
  {
    if (_pdrv == 0xFF)
      return;

    {
      MutexGuard lock(_sd_mutex);

#ifdef SD_TYPE_SPI
      sdcard_unmount(_pdrv);
      sdcard_uninit(_pdrv);

#else  // #ifdef SD_TYPE_MMC
      if (_card)
      {
        esp_vfs_fat_sdcard_unmount(SD_MOUNTPOINT, _card);
        _card = nullptr;
      }

#endif  // #ifdef SD_TYPE_MMC

      _pdrv = 0xFF;
      delay(10);

#ifdef SD_PIN_PWR_ON
      digitalWrite(SD_PIN_PWR_ON, !SD_PWR_ON_LVL);
      pinMode(SD_PIN_PWR_ON, INPUT);

#endif  // #ifdef SD_PIN_PWR_ON
    }

    if (!_is_ext_lock)
      vSemaphoreDelete(_sd_mutex);

    _sd_mutex = nullptr;
  }

  FileManager _fs;
}  // namespace pixeler

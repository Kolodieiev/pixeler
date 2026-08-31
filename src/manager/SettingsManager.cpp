#pragma GCC optimize("O3")
#include "SettingsManager.h"

#include <cstring>

#include "FileManager.h"

namespace pixeler
{
  static const char DATA_ROOT[] = "/.data";
  static const char PREF_ROOT[] = "/.data/preferences";

  static const char STR_EMPTY_PREF_VAL[] = "Ім'я налаштування не може бути порожнім";

  bool SettingsManager::set(const String& pref_name, const String& value, const String& subdir)
  {
    if (!_fs.isMounted())
      return false;

    if (pref_name.isEmpty())
    {
      log_e("%s", STR_EMPTY_PREF_VAL);
      return false;
    }

    String path = getSettingsFilePath(pref_name, subdir);

    if (path.isEmpty())
      return false;

    return _fs.writeFile(path.c_str(), value.c_str(), value.length());
  }

  String SettingsManager::get(const String& pref_name, const String& subdir)
  {
    if (!_fs.isMounted())
      return emptyString;

    if (pref_name.isEmpty())
    {
      log_e("%s", STR_EMPTY_PREF_VAL);
      return emptyString;
    }

    String path = getSettingsFilePath(pref_name, subdir);

    return _fs.readFileToStr(path);
  }

  String SettingsManager::getSettingsFilePath(const String& pref_name, const String& subdir)
  {
    if (!_fs.isMounted())
      return emptyString;

    if (pref_name.isEmpty())
    {
      log_e("%s", STR_EMPTY_PREF_VAL);
      return emptyString;
    }

    String path = getSettingsDirPath(subdir);

    if (path.isEmpty())
      return emptyString;

    path += "/";
    path += pref_name;

    return path;
  }

  String SettingsManager::getSettingsDirPath(const String& sub_dirname)
  {
    if (!_fs.isMounted())
      return emptyString;

    if (!_fs.dirExist(DATA_ROOT, true) && !_fs.createDir(DATA_ROOT))
      return emptyString;

    if (!_fs.dirExist(PREF_ROOT, true) && !_fs.createDir(PREF_ROOT))
      return emptyString;

    String path{PREF_ROOT};

    if (!sub_dirname.isEmpty())
    {
      path += "/";
      path += sub_dirname;

      if (!_fs.dirExist(path.c_str()) && !_fs.createDir(path.c_str()))
        return emptyString;
    }

    return path;
  }

  bool SettingsManager::load(void* out_data_struct, size_t data_struct_size, const String& filename, const String& subdir)
  {
    if (!_fs.isMounted())
      return false;

    String sets_path = SettingsManager::getSettingsFilePath(filename, subdir);

    if (sets_path.isEmpty())
      return false;

    if (!_fs.fileExist(sets_path.c_str(), true))
      return false;

    return _fs.readFile(sets_path.c_str(), out_data_struct, data_struct_size) == data_struct_size;
  }

  bool SettingsManager::save(const void* data_struct, size_t data_struct_size, const String& filename, const String& subdir)
  {
    if (!_fs.isMounted())
      return false;

    String sets_path = SettingsManager::getSettingsFilePath(filename, subdir);

    if (sets_path.isEmpty())
      return false;

    return _fs.writeFile(sets_path.c_str(), data_struct, data_struct_size) == data_struct_size;
  }
}  // namespace pixeler

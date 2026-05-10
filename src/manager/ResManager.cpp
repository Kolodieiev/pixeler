#pragma GCC optimize("O3")
#include "ResManager.h"

#include "util/string_util.h"
#include "res/BmpLoader.h"
#include "res/WavLoader.h"

namespace pixeler
{
  uint32_t ResManager::load(const char* path, const char* res_name)
  {
    std::unique_ptr<IResLoader> loader = getLoaderByExt(path);
    if (!loader)
      return 0;

    IResource* resource = loader->load(path, res_name);
    if (!resource)
      return 0;

    _resources.emplace(++_res_id_counter, resource);

    return _res_id_counter;
  }

  const IResource* ResManager::getResByID(uint32_t res_id) const
  {
    const auto it = _resources.find(res_id);

    if (it == _resources.end())
    {
      log_e("Невідомий ідентифікатор ресурса: %u", res_id);
      return nullptr;
    }

    return it->second;
  }

  void ResManager::deleteResByID(uint32_t res_id)
  {
    const auto it = _resources.find(res_id);

    if (it == _resources.end())
    {
      log_e("Невідомий ідентифікатор ресурса: %u", res_id);
      return;
    }

    delete it->second;
    _resources.erase(it);
  }

  const IResource* ResManager::getResByName(const char* res_name, uint32_t& out_id) const
  {
    if (!res_name)
    {
      log_e("res_name не повинен бути null");
      return nullptr;
    }

    for (const auto& it : _resources)
    {
      if (it.second->hasName(res_name))
      {
        out_id = it.first;
        return it.second;
      }
    }

    log_e("Не знайдено ресурс з іменем: %s", res_name);
    return nullptr;
  }

  void ResManager::deleteResByName(const char* res_name)
  {
    if (!res_name)
    {
      log_e("res_name не повинен бути null");
      return;
    }

    for (auto it = _resources.begin(), it_end = _resources.end(); it != it_end; ++it)
    {
      if (it->second->hasName(res_name))
      {
        delete it->second;
        _resources.erase(it);
        return;
      }
    }

    log_e("Не знайдено ресурс з іменем: %s", res_name);
  }

  void ResManager::freeRes()
  {
    for (const auto& it : _resources)
      delete it.second;

    _resources.clear();
    _res_id_counter = 0;
  }

  std::unique_ptr<IResLoader> ResManager::getLoaderByExt(const char* path)
  {
    if (!path)
    {
      log_e("path не повинен бути null");
      return nullptr;
    }

    if (endsWith(path, ".bmp"))
      return std::make_unique<BmpLoader>();
    if (endsWith(path, ".wav"))
      return std::make_unique<WavLoader>();

    log_e("Незареєстрований тип файла: %s", path);
    return nullptr;
  }

  ResManager _res;
}  // namespace pixeler

#pragma GCC optimize("O3")
#include "IResource.h"

#include <cstring>

namespace pixeler
{
  IResource::IResource(const String& name, void* data, ResType type) : _data{data}, _RES_TYPE{type}, _NAME_LEN{name.length()}
  {
    if (psramInit())
      _name = static_cast<char*>(ps_malloc(_NAME_LEN + 1));
    else
      _name = static_cast<char*>(malloc(_NAME_LEN + 1));

    if (!_name)
    {
      log_e("Помилка виділення буферу");
      esp_restart();
    }

    std::memcpy(_name, name.c_str(), _NAME_LEN);
    _name[_NAME_LEN] = '\0';
  }

  IResource::~IResource()
  {
    free(_data);
    free(_name);
  }

  const char* IResource::getName() const
  {
    return _name;
  }

  const void* IResource::getData() const
  {
    return _data;
  }

  IResource::ResType IResource::getType() const
  {
    return _RES_TYPE;
  }

  bool IResource::hasName(const char* name) const
  {
    return strcmp(_name, name) == 0;
  }
}  // namespace pixeler

#pragma once

#include "http_parser.h"

namespace pixeler
{
  typedef enum http_method HTTPMethod;
#define HTTP_ANY (HTTPMethod)(255)
}  // namespace pixeler

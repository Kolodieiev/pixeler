#pragma GCC optimize("O3")

#include "./ReqHandler.h"

#include <MD5Builder.h>
#include <base64.h>

#include "WString.h"
#include "detail/mimetable.h"

using namespace mime;

namespace pixeler
{
  bool ReqHandler::process(FServer& server, HTTPMethod requestMethod, String requestUri)
  {
    return handle(server, requestMethod, requestUri);
  }
}  // namespace pixeler

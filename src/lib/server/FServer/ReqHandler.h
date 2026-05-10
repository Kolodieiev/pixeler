#pragma once
#pragma GCC optimize("O3")

#include <HTTP_Method.h>
#include <Uri.h>
#include <WString.h>

#include <vector>

#include "./HttpTypes.h"

namespace pixeler
{
  class FServer;

  class ReqHandler
  {
  public:
    virtual ~ReqHandler() {}
    /*
      note: old handler API for backward compatibility
    */

    virtual bool canHandle(HTTPMethod method, const String& uri)
    {
      (void)method;
      (void)uri;
      return false;
    }
    virtual bool canUpload(const String& uri)
    {
      (void)uri;
      return false;
    }
    virtual bool canRaw(const String& uri)
    {
      (void)uri;
      return false;
    }

    /*
      note: new handler API with support for filters etc.
    */

    virtual bool canHandle(FServer& server, HTTPMethod method, const String& uri)
    {
      (void)server;
      (void)method;
      (void)uri;
      return false;
    }
    virtual bool canUpload(FServer& server, const String& uri)
    {
      (void)server;
      (void)uri;
      return false;
    }
    virtual bool canRaw(FServer& server, const String& uri)
    {
      (void)server;
      (void)uri;
      return false;
    }
    virtual bool handle(FServer& server, HTTPMethod requestMethod, const String& requestUri)
    {
      (void)server;
      (void)requestMethod;
      (void)requestUri;
      return false;
    }
    virtual void upload(FServer& server, const String& requestUri, HTTPUpload& upload)
    {
      (void)server;
      (void)requestUri;
      (void)upload;
    }
    virtual void raw(FServer& server, const String& requestUri, HTTPRaw& raw)
    {
      (void)server;
      (void)requestUri;
      (void)raw;
    }

    virtual ReqHandler& setFilter(std::function<bool(FServer&)> filter)
    {
      (void)filter;
      return *this;
    }

    ReqHandler* next()
    {
      return _next;
    }
    void next(ReqHandler* r)
    {
      _next = r;
    }

    bool process(FServer& server, HTTPMethod requestMethod, String requestUri);

  private:
    ReqHandler* _next = nullptr;

  protected:
    std::vector<String> pathArgs;

  public:
    const String& pathArg(unsigned int i)
    {
      assert(i < pathArgs.size());
      return pathArgs[i];
    }
  };
}  // namespace pixeler

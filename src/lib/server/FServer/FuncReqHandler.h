#pragma once
#pragma GCC optimize("O3")

#include "./ReqHandler.h"

namespace pixeler
{
  class FuncReqHandler : public ReqHandler
  {
  public:
    FuncReqHandler(FServer::THandlerFunction fn, FServer::THandlerFunction ufn, const Uri& uri, HTTPMethod method)
        : _fn(fn), _ufn(ufn), _uri(uri.clone()), _method(method)
    {
      _uri->initPathArgs(pathArgs);
    }

    ~FuncReqHandler()
    {
      delete _uri;
    }

    bool canHandle(HTTPMethod requestMethod, const String& requestUri) override
    {
      if (_method != HTTP_ANY && _method != requestMethod)
      {
        return false;
      }

      return _uri->canHandle(requestUri, pathArgs);
    }

    bool canUpload(const String& requestUri) override
    {
      if (!_ufn || !canHandle(HTTP_POST, requestUri))
      {
        return false;
      }

      return true;
    }

    bool canRaw(const String& requestUri) override
    {
      (void)requestUri;
      if (!_ufn || _method == HTTP_GET)
      {
        return false;
      }

      return true;
    }

    bool canHandle(FServer& server, HTTPMethod requestMethod, const String& requestUri) override
    {
      if (_method != HTTP_ANY && _method != requestMethod)
      {
        return false;
      }

      return _uri->canHandle(requestUri, pathArgs) && (_filter != NULL ? _filter(server) : true);
    }

    bool canUpload(FServer& server, const String& requestUri) override
    {
      if (!_ufn || !canHandle(server, HTTP_POST, requestUri))
      {
        return false;
      }

      return true;
    }

    bool canRaw(FServer& server, const String& requestUri) override
    {
      (void)requestUri;
      if (!_ufn || _method == HTTP_GET || (_filter != NULL ? _filter(server) == false : false))
      {
        return false;
      }

      return true;
    }

    bool handle(FServer& server, HTTPMethod requestMethod, const String& requestUri) override
    {
      if (!canHandle(server, requestMethod, requestUri))
      {
        return false;
      }

      _fn();
      return true;
    }

    void upload(FServer& server, const String& requestUri, HTTPUpload& upload) override
    {
      (void)upload;
      if (canUpload(server, requestUri))
      {
        _ufn();
      }
    }

    void raw(FServer& server, const String& requestUri, HTTPRaw& raw) override
    {
      (void)raw;
      if (canRaw(server, requestUri))
      {
        _ufn();
      }
    }

    FuncReqHandler& setFilter(FServer::FilterFunction filter)
    {
      _filter = filter;
      return *this;
    }

  protected:
    FServer::THandlerFunction _fn;
    FServer::THandlerFunction _ufn;
    // _filter should return 'true' when the request should be handled
    // and 'false' when the request should be ignored
    FServer::FilterFunction _filter;
    Uri* _uri;
    HTTPMethod _method;
  };
}  // namespace pixeler

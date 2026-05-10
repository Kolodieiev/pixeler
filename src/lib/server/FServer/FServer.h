/*
  FServer.h - Dead simple web-server.
  Supports only one simultaneous client, knows how to handle GET and POST.

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/

#pragma once
#pragma GCC optimize("O3")

#include <functional>
#include <memory>

#include "./HttpTypes.h"
#include "./ReqHandler.h"
#include "./setup.h"
#include "FS.h"
#include "HTTP_Method.h"
#include "Network.h"
#include "Uri.h"

namespace fs
{
  class FS;
}

namespace pixeler
{
  class FServer
  {
  public:
    FServer(IPAddress addr, int port = 80);
    FServer(int port = 80);
    virtual ~FServer();

    virtual void begin();
    virtual void begin(uint16_t port);
    virtual void handleClient();

    virtual void close();
    void stop();

    void chunkResponseBegin(const char* contentType = "text/plain");
    void chunkWrite(const char* data, size_t length);
    void chunkResponseEnd();

    typedef std::function<void(void)> THandlerFunction;
    typedef std::function<bool(FServer& server)> FilterFunction;
    ReqHandler& on(const Uri& uri, THandlerFunction fn);
    ReqHandler& on(const Uri& uri, HTTPMethod method, THandlerFunction fn);
    ReqHandler& on(const Uri& uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);  // ufn handles file uploads
    bool removeRoute(const char* uri);
    bool removeRoute(const char* uri, HTTPMethod method);
    bool removeRoute(const String& uri);
    bool removeRoute(const String& uri, HTTPMethod method);
    void addHandler(ReqHandler* handler);
    bool removeHandler(ReqHandler* handler);
    void onNotFound(THandlerFunction fn);     // called when handler is not assigned
    void onFileUpload(THandlerFunction ufn);  // handle file uploads

    String uri() const
    {
      return _currentUri;
    }

    HTTPMethod method() const
    {
      return _currentMethod;
    }

    virtual NetworkClient& client()
    {
      return _currentClient;
    }

    HTTPUpload& upload()
    {
      return *_currentUpload;
    }

    HTTPRaw& raw()
    {
      return *_currentRaw;
    }

    String pathArg(unsigned int i) const;                                         // get request path argument by number
    String arg(const String& name) const;                                         // get request argument value by name
    String arg(int i) const;                                                      // get request argument value by number
    String argName(int i) const;                                                  // get request argument name by number
    int args() const;                                                             // get arguments count
    bool hasArg(const String& name) const;                                        // check if argument exists
    void collectHeaders(const char* headerKeys[], const size_t headerKeysCount);  // set the request headers to collect
    void collectAllHeaders();                                                     // collect all request headers
    String header(const String& name) const;                                      // get request header value by name
    String header(int i) const;                                                   // get request header value by number
    String headerName(int i) const;                                               // get request header name by number
    int headers() const;                                                          // get header count
    bool hasHeader(const String& name) const;                                     // check if header exists

    int clientContentLength() const;  // return "content-length" of incoming HTTP header from "_currentClient"
    const String version() const;     // get the HTTP version string
    String hostHeader() const;        // get request host header if available or empty String if not

    int responseCode() const;                          // get the HTTP response code set
    int responseHeaders() const;                       // get the HTTP response headers count
    const String& responseHeader(String name) const;   // get the HTTP response header value by name
    const String& responseHeader(int i) const;         // get the HTTP response header value by number
    const String& responseHeaderName(int i) const;     // get the HTTP response header name by number
    bool hasResponseHeader(const String& name) const;  // check if response header exists

    // send response to the client
    // code - HTTP response code, can be 200 or 404
    // content_type - HTTP content type, like "text/plain" or "image/png"
    // content - actual content body
    void send(int code, const char* content_type = NULL, const String& content = String(""));
    void send(int code, char* content_type, const String& content);
    void send(int code, const String& content_type, const String& content);
    void send(int code, const char* content_type, const char* content);

    void send_P(int code, PGM_P content_type, PGM_P content);
    void send_P(int code, PGM_P content_type, PGM_P content, size_t contentLength);

    void enableDelay(boolean value);
    void enableCORS(boolean value = true);
    void enableCrossOrigin(boolean value = true);
    typedef std::function<String(FS& fs, const String& fName)> ETagFunction;
    void enableETag(bool enable, ETagFunction fn = nullptr);

    void setContentLength(const size_t contentLength);
    void sendHeader(const String& name, const String& value, bool first = false);
    void sendContent(const String& content);
    void sendContent(const char* content, size_t contentLength);
    void sendContent_P(PGM_P content);
    void sendContent_P(PGM_P content, size_t size);

    static String urlDecode(const String& text);

    template <typename T>
    size_t streamFile(T& file, const String& contentType, const int code = 200)
    {
      _streamFileCore(file.size(), file.name(), contentType, code);
      return _currentClient.write(file);
    }

    bool _eTagEnabled = false;
    ETagFunction _eTagFunction = nullptr;

    static String responseCodeToString(int code);

  private:
    bool _chunkedResponseActive = false;
    NetworkClient _chunkedClient;  // Store by value, no dangling pointer

  protected:
    virtual size_t _currentClientWrite(const char* b, size_t l)
    {
      return _currentClient.write(b, l);
    }
    virtual size_t _currentClientWrite_P(PGM_P b, size_t l)
    {
      return _currentClient.write_P(b, l);
    }
    void _addReqHandler(ReqHandler* handler);
    bool _removeReqHandler(ReqHandler* handler);
    bool _handleRequest();
    void _finalizeResponse();
    bool _parseRequest(NetworkClient& client);
    void _parseArguments(const String& data);
    bool _parseForm(NetworkClient& client, const String& boundary, uint32_t len);
    bool _parseFormUploadAborted();
    void _uploadWriteByte(uint8_t b);
    int _uploadReadByte(NetworkClient& client);
    void _prepareHeader(String& response, int code, const char* content_type, size_t contentLength);
    bool _collectHeader(const char* headerName, const char* headerValue);

    void _streamFileCore(const size_t fileSize, const String& fileName, const String& contentType, const int code = 200);

    String _getRandomHexString();
    // for extracting Auth parameters
    String _extractParam(String& authReq, const String& param, const char delimit = '"');

    void _clearResponseHeaders();
    void _clearRequestHeaders();

    struct RequestArgument
    {
      String key;
      String value;
      RequestArgument* next;
    };

    boolean _corsEnabled = false;
    NetworkServer _server;

    NetworkClient _currentClient;
    HTTPMethod _currentMethod = HTTP_ANY;
    String _currentUri;
    uint8_t _currentVersion = 0;
    HTTPClientStatus _currentStatus = HC_NONE;
    unsigned long _statusChange = 0;
    boolean _nullDelay = true;

    ReqHandler* _currentHandler = nullptr;
    ReqHandler* _firstHandler = nullptr;
    ReqHandler* _lastHandler = nullptr;
    THandlerFunction _notFoundHandler = nullptr;
    THandlerFunction _fileUploadHandler = nullptr;

    int _currentArgCount = 0;
    RequestArgument* _currentArgs = nullptr;
    int _postArgsLen = 0;
    RequestArgument* _postArgs = nullptr;

    std::unique_ptr<HTTPUpload> _currentUpload;
    std::unique_ptr<HTTPRaw> _currentRaw;

    int _headerKeysCount = 0;
    RequestArgument* _currentHeaders = nullptr;
    size_t _contentLength = 0;
    int _clientContentLength = 0;  // "Content-Length" from header of incoming POST or GET request
    RequestArgument* _responseHeaders = nullptr;

    String _hostHeader;
    bool _chunked = false;

    int _responseHeaderCount = 0;
    int _responseCode = 0;
    bool _collectAllHeaders = false;
  };
}  // namespace pixeler

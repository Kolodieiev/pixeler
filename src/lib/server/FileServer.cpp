#pragma GCC optimize("O3")

#include "FileServer.h"

#include <algorithm>
#include <cstdio>

#include "./fs_html.cpp"
#include "manager/WiFiManager.h"

static constexpr uint16_t SERVER_PORT{80u};
static constexpr size_t ITEMS_PER_PAGE{30u};
static constexpr uint32_t SERVER_TASK_STACK{(1024u / 2u) * 20u};
static constexpr uint32_t WATCHER_TASK_STACK{(1024u / 2u) * 5u};
static constexpr UBaseType_t SERVER_TASK_PRIO{10u};
static constexpr UBaseType_t WATCHER_TASK_PRIO{10u};
static constexpr uint32_t WATCHER_POLL_DELAY_MS{50u};
static constexpr unsigned long UPLOAD_YIELD_INTERVAL_MS{1000ul};
static constexpr size_t HTML_ITEM_RESERVE{180u};
static constexpr size_t HTML_BASE_RESERVE{5000u};
static constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

namespace
{
  bool sanitizeRelativePath(const String& raw_path, String& out_rel_path)
  {
    out_rel_path.clear();
    const size_t len = raw_path.length();
    if (len == 0u)
      return true;

    out_rel_path.reserve(len);
    size_t seg_start{0u};

    for (size_t i{0u}; i <= len; ++i)
    {
      const bool is_end = (i == len);
      const char ch = is_end ? '/' : raw_path[i];

      if (!is_end)
      {
        const uint8_t byte = static_cast<uint8_t>(ch);
        if (byte < 0x20u || ch == '\\')
          return false;
      }

      if (ch == '/')
      {
        const size_t seg_len = i - seg_start;
        if (seg_len > 0u)
        {
          if ((seg_len == 1u && raw_path[seg_start] == '.') ||
              (seg_len == 2u && raw_path[seg_start] == '.' && raw_path[seg_start + 1u] == '.'))
            return false;

          if (!out_rel_path.isEmpty())
            out_rel_path += '/';

          out_rel_path += raw_path.substring(seg_start, i);
        }

        seg_start = i + 1u;
      }
    }

    return true;
  }

  bool extractBaseFilename(const String& raw_name, String& out_name)
  {
    const int last_slash = raw_name.lastIndexOf('/');
    const int last_bslash = raw_name.lastIndexOf('\\');
    const int cut_pos = (last_slash > last_bslash) ? last_slash : last_bslash;

    out_name = (cut_pos >= 0) ? raw_name.substring(static_cast<unsigned int>(cut_pos + 1)) : raw_name;

    if (out_name.isEmpty() || out_name.equals(".") || out_name.equals(".."))
      return false;

    for (size_t i{0u}; i < out_name.length(); ++i)
    {
      if (static_cast<uint8_t>(out_name[i]) < 0x20u)
        return false;
    }

    return true;
  }

  void appendUrlEncoded(String& out, const char* str)
  {
    for (size_t i{0u}; str[i] != '\0'; ++i)
    {
      const uint8_t ch = static_cast<uint8_t>(str[i]);
      const bool is_unreserved =
          (ch >= 'A' && ch <= 'Z') ||
          (ch >= 'a' && ch <= 'z') ||
          (ch >= '0' && ch <= '9') ||
          ch == '-' || ch == '_' || ch == '.' || ch == '~';

      if (is_unreserved)
      {
        out += static_cast<char>(ch);
      }
      else
      {
        out += '%';
        out += HEX_DIGITS[(ch >> 4u) & 0x0Fu];
        out += HEX_DIGITS[ch & 0x0Fu];
      }
    }
  }

  void appendHtmlEscaped(String& out, const char* str)
  {
    for (size_t i{0u}; str[i] != '\0'; ++i)
    {
      switch (str[i])
      {
        case '&':
          out += "&amp;";
          break;
        case '<':
          out += "&lt;";
          break;
        case '>':
          out += "&gt;";
          break;
        case '"':
          out += "&quot;";
          break;
        case '\'':
          out += "&#39;";
          break;
        default:
          out += str[i];
          break;
      }
    }
  }

  String sanitizeDispositionName(const String& filename)
  {
    String safe;
    safe.reserve(filename.length());

    for (size_t i{0u}; i < filename.length(); ++i)
    {
      const uint8_t ch = static_cast<uint8_t>(filename[i]);
      if (ch < 0x20u || ch == '"' || ch == '\\')
        safe += '_';
      else
        safe += static_cast<char>(ch);
    }

    return safe.isEmpty() ? String("file") : safe;
  }

  const char* getFileIcon(const pixeler::FileInfo& info)
  {
    if (info.isDir())
      return "📁";

    const char* name = info.getName();
    const char* dot = nullptr;
    for (const char* p = name; *p != '\0'; ++p)
    {
      if (*p == '.')
        dot = p;
    }

    if (!dot || dot[1] == '\0')
      return "📄";

    char ext[8]{};
    size_t ext_len{0u};
    for (const char* p = dot + 1; *p != '\0' && ext_len < sizeof(ext) - 1u; ++p)
    {
      char c = *p;
      if (c >= 'A' && c <= 'Z')
        c = static_cast<char>(c - 'A' + 'a');
      ext[ext_len++] = c;
    }

    if (strcmp(ext, "mp3") == 0 || strcmp(ext, "wav") == 0 || strcmp(ext, "ogg") == 0 ||
        strcmp(ext, "flac") == 0 || strcmp(ext, "aac") == 0 || strcmp(ext, "m4a") == 0)
      return "🎵";

    if (strcmp(ext, "png") == 0 || strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0 ||
        strcmp(ext, "bmp") == 0 || strcmp(ext, "gif") == 0 || strcmp(ext, "webp") == 0)
      return "🖼️";

    if (strcmp(ext, "mp4") == 0 || strcmp(ext, "avi") == 0 || strcmp(ext, "mkv") == 0 || strcmp(ext, "mov") == 0)
      return "🎬";

    if (strcmp(ext, "zip") == 0 || strcmp(ext, "rar") == 0 || strcmp(ext, "7z") == 0 ||
        strcmp(ext, "tar") == 0 || strcmp(ext, "gz") == 0)
      return "📦";

    if (strcmp(ext, "txt") == 0 || strcmp(ext, "md") == 0 || strcmp(ext, "json") == 0 ||
        strcmp(ext, "csv") == 0 || strcmp(ext, "log") == 0 || strcmp(ext, "pdf") == 0 ||
        strcmp(ext, "xml") == 0 || strcmp(ext, "ini") == 0 || strcmp(ext, "cfg") == 0)
      return "📝";

    if (strcmp(ext, "bin") == 0 || strcmp(ext, "elf") == 0 || strcmp(ext, "hex") == 0)
      return "⚙️";

    return "📄";
  }

  void appendFormattedSize(String& out, const size_t bytes)
  {
    char buf[24];

    if (bytes < 1024u)
    {
      snprintf(buf, sizeof(buf), "%u B", static_cast<unsigned int>(bytes));
    }
    else if (bytes < 1024u * 1024u)
    {
      const uint32_t kb = static_cast<uint32_t>(bytes / 1024u);
      const uint32_t frac = static_cast<uint32_t>(((bytes % 1024u) * 10u) / 1024u);
      snprintf(buf, sizeof(buf), "%u.%u KB", kb, frac);
    }
    else if (bytes < 1024u * 1024u * 1024u)
    {
      const uint32_t mb = static_cast<uint32_t>(bytes / (1024u * 1024u));
      const uint32_t frac = static_cast<uint32_t>(((bytes % (1024u * 1024u)) * 100u) / (1024u * 1024u));
      snprintf(buf, sizeof(buf), "%u.%02u MB", mb, frac);
    }
    else
    {
      const uint32_t gb = static_cast<uint32_t>(bytes / (1024u * 1024u * 1024u));
      const uint32_t frac = static_cast<uint32_t>(((bytes % (1024u * 1024u * 1024u)) * 100u) / (1024u * 1024u * 1024u));
      snprintf(buf, sizeof(buf), "%u.%02u GB", gb, frac);
    }

    out += buf;
  }

  void appendPageLink(String& out, const String& base_url, const size_t page, const char* label)
  {
    out += "<a href=\"";
    out += base_url;
    out += String(static_cast<unsigned int>(page));
    out += "\">";
    out += label;
    out += "</a>";
  }

  void appendPagination(String& out, const String& encoded_dir, const size_t current_page, const size_t total_pages)
  {
    if (total_pages <= 1u)
      return;

    String base_url;
    base_url.reserve(encoded_dir.length() + 16u);
    if (encoded_dir.isEmpty())
    {
      base_url = "/?page=";
    }
    else
    {
      base_url = "/?dir=";
      base_url += encoded_dir;
      base_url += "&amp;page=";
    }

    out += "<nav class=\"pg\">";

    if (current_page > 1u)
    {
      appendPageLink(out, base_url, 1u, "&laquo;");
      appendPageLink(out, base_url, current_page - 1u, "&lsaquo;");
    }
    else
    {
      out += "<span class=\"dis\">&laquo;</span><span class=\"dis\">&lsaquo;</span>";
    }

    const size_t win_start = (current_page > 2u) ? (current_page - 2u) : 1u;
    const size_t win_end = (current_page + 2u < total_pages) ? (current_page + 2u) : total_pages;

    if (win_start > 1u)
    {
      appendPageLink(out, base_url, 1u, "1");
      if (win_start > 2u)
        out += "<span class=\"dots\">&hellip;</span>";
    }

    char num_buf[12];
    for (size_t p{win_start}; p <= win_end; ++p)
    {
      snprintf(num_buf, sizeof(num_buf), "%u", static_cast<unsigned int>(p));
      if (p == current_page)
      {
        out += "<span class=\"cur\">";
        out += num_buf;
        out += "</span>";
      }
      else
      {
        appendPageLink(out, base_url, p, num_buf);
      }
    }

    if (win_end < total_pages)
    {
      if (win_end + 1u < total_pages)
        out += "<span class=\"dots\">&hellip;</span>";

      snprintf(num_buf, sizeof(num_buf), "%u", static_cast<unsigned int>(total_pages));
      appendPageLink(out, base_url, total_pages, num_buf);
    }

    if (current_page < total_pages)
    {
      appendPageLink(out, base_url, current_page + 1u, "&rsaquo;");
      appendPageLink(out, base_url, total_pages, "&raquo;");
    }
    else
    {
      out += "<span class=\"dis\">&rsaquo;</span><span class=\"dis\">&raquo;</span>";
    }

    out += "</nav>";
  }
}  // namespace

namespace pixeler
{
  FileServer::FileServer()
  {
    _stream_mutex = xSemaphoreCreateMutex();
  }

  FileServer::~FileServer()
  {
    stop();

    if (_stream_mutex)
    {
      vSemaphoreDelete(_stream_mutex);
      _stream_mutex = nullptr;
    }
  }

  bool FileServer::begin(const char* server_path, const ServerMode mode)
  {
    if (_is_working.load() || !server_path)
      return false;

    if (!_stream_mutex)
    {
      _stream_mutex = xSemaphoreCreateMutex();
      if (!_stream_mutex)
        return false;
    }

    if (!_fs.isMounted())
      return false;

    _server_path = server_path;
    _server_mode = mode;

    if (_server_mode != SERVER_MODE_SEND_FILE)
    {
      while (_server_path.length() > 1u && _server_path.endsWith("/"))
        _server_path.remove(_server_path.length() - 1u);

      if (_server_path.isEmpty())
        _server_path = "/";

      if (!_fs.dirExist(_server_path.c_str()))
        return false;
    }
    else if (!_fs.fileExist(_server_path.c_str()))
    {
      return false;
    }

    _dir_entries.clear();
    _cached_dir.clear();
    _is_dir_cached = false;

    _server_ip = "http://";

    if (!_wifi.isConnected())
    {
      log_i("%s", STR_ROUTER_NOT_CONNECTED);
      log_i("Створюю власну AP");

      if (_ssid.isEmpty())
      {
        log_e("Не вказано SSID");
        return false;
      }

      if (!_wifi.createAP(_ssid, _pwd, 1))
        return false;
    }

    _server_ip += _wifi.getIP();
    log_i("File server addr: %s", _server_ip.c_str());

    _must_work.store(true);
    _is_working.store(true);

    const BaseType_t result = xTaskCreatePinnedToCore(
        fileServerTask, "fileServerTask", SERVER_TASK_STACK, this, SERVER_TASK_PRIO, nullptr, 0);

    if (result == pdPASS)
    {
      log_i("File server is working now");
      return true;
    }

    log_e("fileServerTask was not running");

    _is_working.store(false);
    _must_work.store(false);

    if (_wifi.isApEnabled())
      _wifi.disable();

    return false;
  }

  void FileServer::stop()
  {
    if (!_is_working.load() && !_must_work.load())
      return;

    _must_work.store(false);
    _need_watch_client.store(false);

    if (_stream_mutex && xSemaphoreTake(_stream_mutex, portMAX_DELAY) == pdTRUE)
    {
      if (_out_file_stream)
        _out_file_stream->close();

      xSemaphoreGive(_stream_mutex);
    }

    while (_is_working.load())
      delay(1);

    while (_is_watcher_working.load())
      delay(1);

    _fs.closeFile(_in_file);

    _dir_entries.clear();
    _dir_entries.shrink_to_fit();
    _cached_dir.clear();
    _is_dir_cached = false;

    if (_wifi.isApEnabled())
      _wifi.disable();
  }

  bool FileServer::isWorking() const
  {
    return _is_working.load();
  }

  void FileServer::setSSID(const char* ssid)
  {
    _ssid = ssid ? ssid : "";
  }

  void FileServer::setPWD(const char* pwd)
  {
    _pwd = pwd ? pwd : "";
  }

  String FileServer::getAddress() const
  {
    if (_is_working.load())
      return _server_ip;
    else
      return emptyString;
  }

  FileServer::ServerMode FileServer::getServerMode() const
  {
    return _server_mode;
  }

  void FileServer::setReplaceFile(const bool state)
  {
    _need_replace_file = state;
  }

  String FileServer::buildFullPath(const String& rel_path) const
  {
    if (rel_path.isEmpty())
      return _server_path;

    String full;
    full.reserve(_server_path.length() + rel_path.length() + 1u);
    full = _server_path;

    if (!_server_path.equals("/"))
      full += '/';

    full += rel_path;
    return full;
  }

  void FileServer::startWebServer()
  {
    _server = new FServer(SERVER_PORT);

    if (_server_mode == SERVER_MODE_RECEIVE)
    {
      _server->on("/", HTTP_GET, [this]()
                  { this->handleReceive(); });
      _server->on("/upload", HTTP_POST, []() {}, [this]()
                  { this->handleFile(); });
    }
    else if (_server_mode == SERVER_MODE_SEND)
    {
      _server->on("/", HTTP_GET, [this]()
                  { this->handleSend(); });
    }
    else
    {
      _server->on("/", HTTP_GET, [this]()
                  { this->handleSendFile(); });
    }

    _server->onNotFound([this]()
                        { this->handle404(); });

    _server->begin();

    while (_must_work.load())
    {
      _server->handleClient();
      delay(1);
    }

    _server->close();
    delete _server;
    _server = nullptr;

    _fs.closeFile(_in_file);

    _is_working.store(false);

    log_i("Задачу FileServer завершено");
    vTaskDelete(nullptr);
  }

  void FileServer::handleReceive()
  {
    String html;
    html.reserve(sizeof(HEAD_HTML) + sizeof(RECEIVE_TITLE_STR) + sizeof(STYLE_HTML) +
                 sizeof(RECEIVE_BODY_HTML) + sizeof(FOOT_HTML));
    html += HEAD_HTML;
    html += RECEIVE_TITLE_STR;
    html += STYLE_HTML;
    html += RECEIVE_BODY_HTML;
    html += FOOT_HTML;

    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "text/html", html);
  }

  void FileServer::handleSend()
  {
    if (_server->hasArg("file") || _server->hasArg("filename"))
    {
      const String raw_file = _server->hasArg("file") ? _server->arg("file") : _server->arg("filename");
      String rel_file;

      if (!sanitizeRelativePath(raw_file, rel_file) || rel_file.isEmpty())
      {
        handle404();
        return;
      }

      const String full_path = buildFullPath(rel_file);
      if (!_fs.fileExistSilently(full_path.c_str()))
      {
        handle404();
        return;
      }

      FILE* file = _fs.openFile(full_path.c_str(), "rb");
      if (!file)
      {
        handle404();
        return;
      }

      const int slash_pos = rel_file.lastIndexOf('/');
      const String display_name = (slash_pos == -1) ? rel_file : rel_file.substring(static_cast<unsigned int>(slash_pos + 1));
      const size_t f_size = _fs.getFileSize(full_path.c_str());

      streamFileToClient(file, display_name, f_size);
      return;
    }

    String rel_dir;
    if (_server->hasArg("dir"))
    {
      if (!sanitizeRelativePath(_server->arg("dir"), rel_dir))
      {
        handle404();
        return;
      }
    }

    const String full_dir = buildFullPath(rel_dir);
    if (!_fs.dirExistSilently(full_dir.c_str()))
    {
      handle404();
      return;
    }

    const bool force_refresh = _server->hasArg("refresh");
    if (force_refresh || !_is_dir_cached || !_cached_dir.equals(rel_dir))
    {
      _dir_entries = _fs.indexAll(full_dir.c_str());
      std::sort(_dir_entries.begin(), _dir_entries.end(), [](const FileInfo& a, const FileInfo& b)
                {
        if (a.isDir() != b.isDir())
          return a.isDir();

        return a < b; });

      _cached_dir = rel_dir;
      _is_dir_cached = true;
    }

    const size_t total_items = _dir_entries.size();
    const size_t total_pages = (total_items == 0u) ? 1u : ((total_items + ITEMS_PER_PAGE - 1u) / ITEMS_PER_PAGE);

    size_t current_page{1u};
    if (_server->hasArg("page"))
    {
      const long parsed = _server->arg("page").toInt();
      if (parsed > 1L)
        current_page = static_cast<size_t>(parsed);
    }

    if (current_page > total_pages)
      current_page = total_pages;

    const size_t start_idx = (current_page - 1u) * ITEMS_PER_PAGE;
    const size_t end_idx = (start_idx + ITEMS_PER_PAGE < total_items) ? (start_idx + ITEMS_PER_PAGE) : total_items;
    const size_t page_count = end_idx - start_idx;

    String encoded_dir;
    if (!rel_dir.isEmpty())
    {
      encoded_dir.reserve(rel_dir.length() * 3u);
      appendUrlEncoded(encoded_dir, rel_dir.c_str());
    }

    String html;
    html.reserve(HTML_BASE_RESERVE + page_count * HTML_ITEM_RESERVE);

    html += HEAD_HTML;
    html += SEND_TITLE_STR;
    html += STYLE_HTML;

    html += "<div class=\"hdr\"><div class=\"crumbs\">📂 <a href=\"/\">/</a>";

    if (!rel_dir.isEmpty())
    {
      String accum_path;
      accum_path.reserve(rel_dir.length());
      size_t seg_start{0u};

      for (size_t i{0u}; i <= rel_dir.length(); ++i)
      {
        if (i == rel_dir.length() || rel_dir[i] == '/')
        {
          const String seg = rel_dir.substring(seg_start, i);
          if (!accum_path.isEmpty())
            accum_path += '/';
          accum_path += seg;

          if (i < rel_dir.length())
          {
            html += "<a href=\"/?dir=";
            appendUrlEncoded(html, accum_path.c_str());
            html += "\">";
            appendHtmlEscaped(html, seg.c_str());
            html += "</a>/";
          }
          else
          {
            appendHtmlEscaped(html, seg.c_str());
            html += '/';
          }

          seg_start = i + 1u;
        }
      }
    }

    html += "</div><div class=\"meta\"><span>";

    char info_buf[64];
    if (total_items > 0u)
    {
      snprintf(info_buf,
               sizeof(info_buf),
               "%u&ndash;%u / %u",
               static_cast<unsigned int>(start_idx + 1u),
               static_cast<unsigned int>(end_idx),
               static_cast<unsigned int>(total_items));
      html += info_buf;
    }
    else
    {
      html += "0";
    }

    html += "</span><a class=\"btn\" href=\"/?";
    if (!encoded_dir.isEmpty())
    {
      html += "dir=";
      html += encoded_dir;
      html += "&amp;";
    }
    html += "refresh=1\">🔄 Оновити</a></div></div>";

    appendPagination(html, encoded_dir, current_page, total_pages);

    html += "<table><thead><tr><th class=\"ic\"></th><th>Назва</th><th class=\"sz\">Розмір</th></tr></thead><tbody>";

    if (!rel_dir.isEmpty())
    {
      html += "<tr><td class=\"ic\">⬆️</td><td class=\"nm\"><a href=\"/";
      const int parent_slash = rel_dir.lastIndexOf('/');
      if (parent_slash > 0)
      {
        const String parent_dir = rel_dir.substring(0u, static_cast<unsigned int>(parent_slash));
        html += "?dir=";
        appendUrlEncoded(html, parent_dir.c_str());
      }
      html += "\">..</a></td><td class=\"sz\">—</td></tr>";
    }

    if (total_items == 0u)
    {
      html += "<tr><td colspan=\"3\" class=\"empty\">📭 Каталог порожній</td></tr>";
    }
    else
    {
      String item_rel;
      String item_full;

      for (size_t i{start_idx}; i < end_idx; ++i)
      {
        const FileInfo& info = _dir_entries[i];
        const char* name = info.getName();

        item_rel.clear();
        if (!rel_dir.isEmpty())
        {
          item_rel += rel_dir;
          item_rel += '/';
        }
        item_rel += name;

        html += "<tr><td class=\"ic\">";
        html += getFileIcon(info);
        html += "</td><td class=\"nm\"><a href=\"/?";
        html += info.isDir() ? "dir=" : "file=";
        appendUrlEncoded(html, item_rel.c_str());
        html += "\">";
        appendHtmlEscaped(html, name);
        if (info.isDir())
          html += '/';
        html += "</a></td><td class=\"sz\">";

        if (info.isDir())
        {
          html += "DIR";
        }
        else
        {
          item_full = buildFullPath(item_rel);
          appendFormattedSize(html, _fs.getFileSize(item_full.c_str()));
        }

        html += "</td></tr>";
      }
    }

    html += "</tbody></table>";

    appendPagination(html, encoded_dir, current_page, total_pages);

    html += FOOT_HTML;

    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "text/html", html);
  }

  void FileServer::handleSendFile()
  {
    FILE* file = _fs.openFile(_server_path.c_str(), "rb");

    if (!file)
    {
      handle404();
    }
    else
    {
      const size_t f_size = _fs.getFileSize(_server_path.c_str());
      const int pos = _server_path.lastIndexOf('/');
      const String filename = (pos == -1) ? _server_path : _server_path.substring(static_cast<unsigned int>(pos + 1));

      streamFileToClient(file, filename, f_size);
    }
  }

  void FileServer::streamFileToClient(FILE* file, const String& filename, const size_t file_size)
  {
    FileStream* stream = new FileStream(file, filename.c_str(), file_size);

    if (xSemaphoreTake(_stream_mutex, portMAX_DELAY) == pdTRUE)
    {
      _out_file_stream = stream;
      xSemaphoreGive(_stream_mutex);
    }

    _need_watch_client.store(true);
    _is_watcher_working.store(true);

    const BaseType_t watcher_res = xTaskCreatePinnedToCore(
        clientWatcherTask, "clientWatcherTask", WATCHER_TASK_STACK, this, WATCHER_TASK_PRIO, nullptr, 1);

    if (watcher_res != pdPASS)
    {
      _is_watcher_working.store(false);
      _need_watch_client.store(false);
      log_e("Не вдалося запустити clientWatcherTask");
    }

    const String safe_name = sanitizeDispositionName(filename);
    String encoded_name;
    encoded_name.reserve(filename.length() * 3u);
    appendUrlEncoded(encoded_name, filename.c_str());

    _server->sendHeader("Content-Type", "application/force-download");
    _server->sendHeader("Content-Disposition",
                        "attachment; filename=\"" + safe_name + "\"; filename*=UTF-8''" + encoded_name);
    _server->sendHeader("Content-Transfer-Encoding", "binary");
    _server->sendHeader("Cache-Control", "no-cache");
    _server->streamFile(*stream, "application/octet-stream");

    _need_watch_client.store(false);

    if (xSemaphoreTake(_stream_mutex, portMAX_DELAY) == pdTRUE)
    {
      _out_file_stream = nullptr;
      xSemaphoreGive(_stream_mutex);
    }

    // Очікуємо завершення clientWatcherTask перед звільненням stream
    while (_is_watcher_working.load())
      delay(1);

    delete stream;
  }

  void FileServer::handleFile()
  {
    HTTPUpload& uploadfile = _server->upload();
    _server->setContentLength(CONTENT_LENGTH_UNKNOWN);

    if (uploadfile.status == UPLOAD_FILE_START)
    {
      _fs.closeFile(_in_file);

      String base_name;
      if (!extractBaseFilename(uploadfile.filename, base_name))
      {
        log_e("Некоректне ім'я файлу для завантаження");
        _server->send(400, "text/html", "");
        return;
      }

      String filename = buildFullPath(base_name);

      log_i("Запит на створення файлу %s", filename.c_str());

      if (_need_replace_file)
      {
        if (_fs.fileExistSilently(filename.c_str()))
          _fs.rmFile(filename.c_str());
      }
      else
      {
        filename = _fs.makeUniqueFilename(filename);
      }

      _in_file = _fs.openFile(filename.c_str(), "wb");

      if (!_in_file)
      {
        log_e("Не можу відкрити файл %s для запису", filename.c_str());
        _server->send(500, "text/html", "");
        return;
      }

      _last_delay_ts = millis();
    }
    else if (uploadfile.status == UPLOAD_FILE_WRITE)
    {
      if (!_must_work.load())
      {
        _fs.closeFile(_in_file);
        WiFiClient client = _server->client();
        if (client)
          client.stop();

        log_i("Сервер скасував завантаження файлу");
        return;
      }

      if (!_in_file)
        return;

      const size_t written = _fs.writeToFile(_in_file, static_cast<const void*>(uploadfile.buf), uploadfile.currentSize);
      if (written != uploadfile.currentSize)
      {
        log_e("Помилка запису файлу: записано %u з %u байтів",
              static_cast<unsigned int>(written),
              static_cast<unsigned int>(uploadfile.currentSize));
        _fs.closeFile(_in_file);
        return;
      }

      if (millis() - _last_delay_ts > UPLOAD_YIELD_INTERVAL_MS)
      {
        delay(1);
        _last_delay_ts = millis();
      }
    }
    else if (uploadfile.status == UPLOAD_FILE_END || uploadfile.status == UPLOAD_FILE_ABORTED)
    {
      if (_in_file)
      {
        _fs.closeFile(_in_file);

        if (uploadfile.status == UPLOAD_FILE_END)
        {
          if (_server->hasArg("xhr"))
            _server->send(200, "text/plain", "OK");
          else
            handleReceive();

          log_i("Файл отримано");
        }
        else
        {
          log_i("Завантаження скасовано");
        }
      }
      else
      {
        log_e("Необроблений файл");
        _server->send(500, "text/html", "");
      }
    }
  }

  void FileServer::handle404()
  {
    String target = _server->uri();
    if (_server->hasArg("file"))
      target = _server->arg("file");
    else if (_server->hasArg("filename"))
      target = _server->arg("filename");
    else if (_server->hasArg("dir"))
      target = _server->arg("dir");

    String html;
    html.reserve(sizeof(HEAD_HTML) + sizeof(NOT_FOUND_TITLE_STR) + sizeof(STYLE_HTML) +
                 sizeof(NOT_FOUND_BODY_START) + sizeof(NOT_FOUND_BODY_END) + sizeof(FOOT_HTML) +
                 target.length() * 2u);
    html += HEAD_HTML;
    html += NOT_FOUND_TITLE_STR;
    html += STYLE_HTML;
    html += NOT_FOUND_BODY_START;
    appendHtmlEscaped(html, target.c_str());
    html += NOT_FOUND_BODY_END;
    html += FOOT_HTML;

    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(404, "text/html", html);
  }

  void FileServer::fileServerTask(void* params)
  {
    FileServer* instance = static_cast<FileServer*>(params);
    instance->startWebServer();
  }

  void FileServer::clientWatcherTask(void* params)
  {
    FileServer* instance = static_cast<FileServer*>(params);

    while (instance->_need_watch_client.load())
    {
      if (!instance->_server->client())
      {
        if (xSemaphoreTake(instance->_stream_mutex, portMAX_DELAY) == pdTRUE)
        {
          if (instance->_out_file_stream && instance->_out_file_stream->available())
          {
            log_i("Клієнт скасував завантаження");
            instance->_out_file_stream->close();
          }

          xSemaphoreGive(instance->_stream_mutex);
        }

        break;
      }

      delay(WATCHER_POLL_DELAY_MS);
    }

    instance->_is_watcher_working.store(false);
    log_i("Задачу ClientWatcher завершено");
    vTaskDelete(nullptr);
  }
}  // namespace pixeler

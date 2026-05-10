#pragma once

#include <WString.h>

#include "./setup.h"

namespace pixeler
{
  enum HTTPUploadStatus
  {
    UPLOAD_FILE_START,
    UPLOAD_FILE_WRITE,
    UPLOAD_FILE_END,
    UPLOAD_FILE_ABORTED
  };

  enum HTTPRawStatus
  {
    RAW_START,
    RAW_WRITE,
    RAW_END,
    RAW_ABORTED
  };

  enum HTTPClientStatus
  {
    HC_NONE,
    HC_WAIT_READ,
    HC_WAIT_CLOSE
  };

  typedef struct
  {
    HTTPUploadStatus status;
    String filename;
    String name;
    String type;
    size_t totalSize;    // file size
    size_t currentSize;  // size of data currently in buf
    uint8_t buf[HTTP_UPLOAD_BUFLEN];
  } HTTPUpload;

  typedef struct
  {
    HTTPRawStatus status;
    size_t totalSize;    // content size
    size_t currentSize;  // size of data currently in buf
    uint8_t buf[HTTP_RAW_BUFLEN];
    void* data;  // additional data
  } HTTPRaw;
}  // namespace pixeler
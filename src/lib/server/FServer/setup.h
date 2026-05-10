#pragma once

#define HTTP_DOWNLOAD_UNIT_SIZE 4096
#define HTTP_UPLOAD_BUFLEN 4096
#define HTTP_RAW_BUFLEN 4096

#define HTTP_MAX_DATA_WAIT 5000   // ms to wait for the client to send the request
#define HTTP_MAX_POST_WAIT 5000   // ms to wait for POST data to arrive
#define HTTP_MAX_SEND_WAIT 5000   // ms to wait for data chunk to be ACKed
#define HTTP_MAX_CLOSE_WAIT 5000  // ms to wait for the client to close the connection

#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)
#define CONTENT_LENGTH_NOT_SET ((size_t)-2)

#pragma once
#include <stdbool.h>

typedef enum { HTTPS_GET, HTTPS_POST, HTTPS_PUT, HTTPS_PATCH, HTTPS_DELETE } HTTPS_METHODS;

extern char* httpsResult;
extern size_t httpsResultSize;

void httpsInitialize();

/**
 * perform a https request
 * 
 * @param url char * to the url, with https://
 * @param type HTTPS_REQUEST_TYPE
 * @param headers char *, newline separated headers.
 * @param data char * to post data.
 */
bool httpsRequest(char *url, HTTPS_METHODS type, char *headers, char *data);
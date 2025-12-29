#include <curl/curl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "https.h"

// 10240 bytes = 10 KiB
#define MAX_RESPONSE_SIZE 10240L
// +1 for null terminator
#define MAX_HEADER_LENGTH 1024+1
// in s
#define TIMEOUT 10L

// extern in header
char* httpsResult = NULL;
size_t httpsResultSize = 0;

bool httpsInitialized = false;

void httpsInitialize(){
    httpsResult = malloc(MAX_RESPONSE_SIZE+1);
    if(httpsResult == NULL)
    {
        printf("HTTPS FAIL: FAILED TO ALLOCATE RESULT BUFFER\n");
        exit(1);
    }
    httpsInitialized = true;
}

static bool writeOk = true;
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    /*
    From curl docs:
    This callback function gets called by libcurl as soon as there is data received that needs to be saved. For most transfers, this callback gets called many times and each invoke delivers another chunk of data. ptr points to the delivered data, and the size of that data is nmemb; size is always 1.
    The data passed to this function is not null-terminated. 
    This function may be called with zero bytes data if the transferred file is empty.
    Set the userdata argument with the CURLOPT_WRITEDATA option. 
    */

    size_t sizeAfter = httpsResultSize + nmemb;
    if(sizeAfter < MAX_RESPONSE_SIZE)
    {
        memcpy(userdata+httpsResultSize, ptr, nmemb);
        httpsResultSize = sizeAfter;
        return nmemb;
    }
    else
    {
        printf("HTTPS FAIL: RESPONSE EXCEEDS MAX_RESPONSE_SIZE\n");
        writeOk = false;
        httpsResultSize = sizeAfter;
        return 0;
    }
}

bool httpsRequest(char *url, HTTPS_METHODS type, char *headers, char *data){
    if(httpsInitialized == false)
    {
        printf("HTTPS FAIL: HTTPS NOT INITALIZED\n"); 
        return false;
    }

    memset(httpsResult, 0, MAX_RESPONSE_SIZE+1);
    httpsResultSize = 0;
    writeOk = true;

    CURL *curl = curl_easy_init();
    if(!curl)
    {
        printf("HTTPS FAIL: FAILED TO INITIALIZE CURL HANDLER\n");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpsResult);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)MAX_RESPONSE_SIZE);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

    struct curl_slist *headerList = NULL;
    if(headers != NULL)
    {
        char header[MAX_HEADER_LENGTH];

        int headersStringIndex = 0;
        while(true)
        {
            int headerBufferIndex = 0;
            memset(header, '\0', MAX_HEADER_LENGTH);
            while(headers[headersStringIndex] != '\0' && headers[headersStringIndex] != '\n')
            {
                header[headerBufferIndex] = headers[headersStringIndex];
                headerBufferIndex++;
                headersStringIndex++;
            }
            headerList = curl_slist_append(headerList, header);

            if(headers[headersStringIndex] == '\0')
            {
                break;
            }
            else
            {
                headersStringIndex++;
            }
        }
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    switch (type)
    {
        case HTTPS_GET:
        default:
            curl_easy_setopt(curl, CURLOPT_HTTPGET , 1);
            break;
        case HTTPS_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            break;
        case HTTPS_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            break;
        case HTTPS_PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            break;
        case HTTPS_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
    }

    CURLcode status = curl_easy_perform(curl);

    if(status != CURLE_OK)
    {
        long httpsError;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpsError);
        printf("HTTPS FAIL: CURL EXITED WITH ERROR %i, HTTPS %li\n", status, httpsError);
        return false;
    }

    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    return true;
}
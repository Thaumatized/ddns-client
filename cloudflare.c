#include "cloudflare.h"

#include "https.h"
#include "c-jsonc/json.h"
#include "ipUtils.h"
#include "utils.h"
#include "config.h"
#include "ipUtils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

JSON *cloudflareConfigs;


char *createRecord(char* token, char*zoneId, char *recordName, char recordType)
{
    char url[256];
    memset(url, 0, sizeof(url));
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/", zoneId);
    char ip[IPV6STRINGLENGTH];
    memset(ip, 0, sizeof(ip));
    if(recordType & IPV6)
    {
        memcpy(ip, "1234:5678:90AB:CDEF:1234:5678:90AB:CDEF", strlen("1234:5678:90AB:CDEF:1234:5678:90AB:CDEF"));
    }
    else
    {
        memcpy(ip, "123.123.123.123", strlen("123.123.123.123"));
    }

    // 54 = "Content-Type: application/json\nAuthorization: "+ NULL
    // 40 = length of cloudflare token.
    // min lenght  = 94
    char headers[128];
    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Content-Type: application/json\nAuthorization: Bearer %s", token);

    char recordTypeString[5];
    memset(recordTypeString, '\0', sizeof(recordTypeString));
    if(recordType & IPV4)
    {
        strcpy(recordTypeString, "A");
    }
    else
    {
        strcpy(recordTypeString, "AAAA");
    }

    // TODO: use new c-jsonc functions here
    char data[256];
    memset(data, 0, sizeof(data));
    sprintf(data,
        "{"
            "\"name\":\"%s\","
            "\"type\":\"%s\","
            "\"content\":\"%s\","
            "\"comment\":\"ddns-client: %s\""
        "}",
         recordName, recordTypeString, ip, jsonGetString(clientConfig, "clientId"));

    printf("Creating record %s (%s)\n", recordName, recordTypeString);
    bool success = httpsRequest(url, HTTPS_POST, headers, data);
    if(success)
    {
        printf("RESULT: %s\n\n", httpsResult);
    }
    else
    {
        printf("FAILED\n\n");
    }

    sleep(*jsonGetNumber(clientConfig, "throttleInterval"));
    if(success)
    {
        JSON *httpsJson = jsonParse(httpsResult);
        char *id = strdup(jsonGetString(httpsJson, "result.id"));
        jsonFree(httpsJson);
        return id;
    }
    else
    {
        return NULL;
    }
}

void setRecord(char* token, char *zoneId, char *recordId, IpAddresses addresses, char recordType)
{
    char url[256];
    memset(url, 0, sizeof(url));
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s", zoneId, recordId);
    char ip[IPV6STRINGLENGTH];
    memset(ip, 0, sizeof(ip));
    if(recordType & IPV6)
    {
        memcpy(ip, addresses.ipv6, strlen(addresses.ipv6));
    }
    else
    {
        memcpy(ip, addresses.ipv4, strlen(addresses.ipv4));
    }

    // 54 = "Content-Type: application/json\nAuthorization: "+ NULL
    // 40 = length of cloudflare token.
    // min lenght  = 94
    char headers[128];
    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Content-Type: application/json\nAuthorization: Bearer %s", token);

    // TODO: use new c-jsonc functions here
    char data[256];
    memset(data, 0, sizeof(data));
    sprintf(data,
        "{"
            "\"content\":\"%s\""
        "}",
         ip);

    printf("Updating %s to %s\n", recordId, ip);
    bool success = httpsRequest(url, HTTPS_PATCH, headers, data);
    if(success)
    {
        printf("RESULT: %s\n\n", httpsResult);
    }
    else
    {
        printf("FAILED\n\n");
    }

    sleep(*jsonGetNumber(clientConfig, "throttleInterval"));
}

int setCloudflareConfigs(JSON *configs)
{
    printf("setCloudflareConfigs\n");
    
    cloudflareConfigs = configs;
    int protocolsEnabled = 0;

    // TODO: Verification
    for (JSON *configJson = cloudflareConfigs->children; configJson != NULL; configJson = configJson->nextSibling)
    {
        if(configJson->type != JSON_OBJECT)
        {
            printf("a cloudflare config is not an object!\nFailed to get configs\n");
            exit(1);
        }

        char *token = jsonGetString(configJson, "token"); 
        // Since this data is required to figure out zone ids and shared between zones under the same token we fetch it first.
        // 54 = "Content-Type: application/json\nAuthorization: "+ NULL
        // 40 = length of cloudflare token.
        // min lenght  = 94
        char headers[128];
        memset(headers, 0, sizeof(headers));
        sprintf(headers, "Content-Type: application/json\nAuthorization: Bearer %s", token);
        httpsRequest("https://api.cloudflare.com/client/v4/zones", HTTPS_GET, headers, NULL);
        JSON *cloudflareZonesJson = jsonParse(httpsResult);
        bool cloudflareApiSuccess = jsonGetBool(cloudflareZonesJson, "success");
        if(!cloudflareApiSuccess)
        {
            printf("Cloudflare api error: %s", httpsResult);
            exit(1);
        }

        JSON *zoneJson = jsonGetArray(configJson, "zones");
        for (zoneJson = zoneJson->children; zoneJson != NULL; zoneJson = zoneJson->nextSibling)
        {
            char *zoneName = jsonGetString(zoneJson, "name");

            //get zone id
            JSON *cloudflareZoneJson = jsonGetArray(cloudflareZonesJson, "result");
            char *zoneId = NULL;
            for (cloudflareZoneJson = cloudflareZoneJson->children; cloudflareZoneJson != NULL; cloudflareZoneJson = cloudflareZoneJson->nextSibling)
            {
                char *name = jsonGetString(cloudflareZoneJson, "name");
                if(!strcmp(name, zoneName))
                {
                    zoneId = jsonGetString(cloudflareZoneJson, "id");
                    break;
                }
            }
            
            printf("ZONE ID: %s\n", zoneId);
            jsonSetString(zoneJson, zoneId, "id");

            // Since this data is required to figure out record ids and shared between records under the same zone we fetch it first.
            char url[256]; //min length is around 90 I think. Better safe than sorry.
            snprintf(url, sizeof(url), "https://api.cloudflare.com/client/v4/zones/%s/dns_records", zoneId);
            httpsRequest(url, HTTPS_GET, headers, NULL);
            JSON *cloudflareRecordsJson = jsonParse(httpsResult);
            cloudflareApiSuccess = jsonGetBool(cloudflareZonesJson, "success");
            if(!cloudflareApiSuccess)
            {
                printf("Cloudflare api error: %s", httpsResult);
                exit(1);
            }

            //get records
            JSON *recordJson = jsonGetArray(zoneJson, "records");
            for (recordJson = recordJson->children; recordJson != NULL; recordJson = recordJson->nextSibling)
            {
                bool recordHandled = false;

                JSON *cloudflareRecordJson = jsonGetArray(cloudflareRecordsJson, "result");
                for (cloudflareRecordJson = cloudflareRecordJson->children; cloudflareRecordJson != NULL; cloudflareRecordJson = cloudflareRecordJson->nextSibling)
                {
                    if(strcmp(jsonGetString(recordJson, "domain"), jsonGetString(cloudflareRecordJson, "name")))
                        continue;
                    if(strcmp(jsonGetString(recordJson, "type"), jsonGetString(cloudflareRecordJson, "type")))
                        continue;

                    
                    int dataLength = strlen("ddns-client: %s") + strlen(jsonGetString(clientConfig, "clientId"));
                    char data[dataLength];
                    memset(data, 0, sizeof(dataLength));
                    sprintf(data, "ddns-client: %s", jsonGetString(clientConfig, "clientId"));

                    // skip records not belonging to this client
                    if(strcmp(data,jsonGetString(cloudflareRecordJson, "comment")))
                        continue;

                    recordHandled = true;
                    char *recordName = jsonGetString(cloudflareRecordJson, "name");
                    char *recordType = jsonGetString(cloudflareRecordJson, "type");
                    char *recordId = jsonGetString(cloudflareRecordJson, "id");
                    char *recordComment = jsonStringify(jsonGetKey(cloudflareRecordJson, "comment"));
                    
                    printf("RECORD ID: %s, for record %s (%s) (%s)\n", recordId, recordName, recordType, recordComment);
                    jsonSetString(recordJson, recordId, "id");


                    if(!strcmp(recordType, "A"))
                    {
                        protocolsEnabled |= IPV4;
                    }
                    if(!strcmp(recordType, "AAAA"))
                    {
                        protocolsEnabled |= IPV6;
                    }

                    break;
                }

                if(!recordHandled) {
                    char *recordType = jsonGetString(recordJson, "type");
                    int typeInt = 0;
                    if(!strcmp(recordType, "A"))
                    {
                        typeInt = IPV4;
                    }
                    if(!strcmp(recordType, "AAAA"))
                    {
                        typeInt = IPV6;
                    }
                    char *recordId = createRecord(token, zoneId, jsonGetString(recordJson, "domain"), typeInt);
                    jsonSetString(recordJson, recordId, "id");
                    free(recordId);
                }
            }
        }
        jsonFree(cloudflareZonesJson);
    }   

    return protocolsEnabled;
}

void updateCloudflareRecords(IpAddresses addresses, int updated) {
    for (JSON *configJson = cloudflareConfigs->children; configJson != NULL; configJson = configJson->nextSibling)
    {
        if(configJson->type != JSON_OBJECT)
        {
            printf("a cloudflare config is not an object!\nFailed to get configs\n");
            exit(1);
        }

        char *token = jsonGetString(configJson, "token"); 

        JSON *zoneJson = jsonGetArray(configJson, "zones");
        for (zoneJson = zoneJson->children; zoneJson != NULL; zoneJson = zoneJson->nextSibling)
        {
            char *zoneId = jsonGetString(zoneJson, "id");

            JSON *recordJson = jsonGetArray(zoneJson, "records");
            for (recordJson = recordJson->children; recordJson != NULL; recordJson = recordJson->nextSibling)
            {
                char *recordId = jsonGetString(recordJson, "id");
                char *recordTypeString = jsonGetString(recordJson, "type");

                int recordType = 0;
                if(!strcmp(recordTypeString, "A"))
                {
                    recordType = IPV4;
                }
                if(!strcmp(recordTypeString, "AAAA"))
                {
                    recordType = IPV6;
                }

                setRecord(token, zoneId, recordId, addresses, recordType);
            }
        }
    }
}
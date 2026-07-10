#include "cloudflare.h"

#include "https.h"
#include "c-jsonc/json.h"
#include "ipUtils.h"

#include <stdlib.h>
#include <string.h>

JSON *cloudflareConfigs;

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
            zoneName = zoneName;

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
            /*
                TODO: Set ids into config json so that we can use them later without fetching
                int zoneIdLength = strlen(zoneId);
                malloc(sizeof(JSON));
            */

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

            printf("cloudflareRecordsJson: %s\n", jsonStringify(cloudflareRecordsJson));

            //get records
            JSON *recordJson = jsonGetArray(cloudflareRecordsJson, "result");
            for (recordJson = recordJson->children; recordJson != NULL; recordJson = recordJson->nextSibling)
            {
                char *recordName = jsonGetString(recordJson, "name");
                char *recordType = jsonGetString(recordJson, "type");
                char *recordId = jsonGetString(recordJson, "id");
                char *recordComment = jsonStringify(jsonGetKey(recordJson, "comment"));
                
                printf("RECORD ID: %s, for record %s (%s) (%s)\n", recordId, recordName, recordType, recordComment);

                if(!strcmp(recordType, "A"))
                {
                    protocolsEnabled |= IPV4;
                }
                if(!strcmp(recordType, "AAAA"))
                {
                    protocolsEnabled |= IPV6;
                }
            }
        }
    }   

    return protocolsEnabled;
}

void setRecord(char* token, char *zone, char* name, char *record, IpAddresses addresses, char ipv6)
{
    for (JSON *configJson = cloudflareConfigs->children; configJson != NULL; configJson = configJson->nextSibling)
    {
        if(configJson->type != JSON_OBJECT)
        {
            printf("a cloudflare config is not an object!\nFailed to get configs\n");
            exit(1);
        }

        char *provider = jsonGetString(configJson, "provider");
        if(!strcmp(provider, "cloudflare"))
        {
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
                zoneName = zoneName;

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

                // Since this data is required to figure out record ids and shared between records under the same zone we fetch it first.
                char url[256]; //min length is around 90 I think
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
                    char *recordName = jsonGetString(recordJson, "name");
                    char *recordType = jsonGetString(recordJson, "type");

                    //get records id
                    JSON *cloudflareRecordJson = jsonGetArray(cloudflareZonesJson, "result");
                    char *recordId = NULL;
                    for (cloudflareRecordJson = cloudflareRecordJson->children; cloudflareRecordJson != NULL; cloudflareRecordJson = cloudflareRecordJson->nextSibling)
                    {
                        char *name = jsonGetString(cloudflareRecordJson, "name");
                        char *type = jsonGetString(cloudflareRecordJson, "type");
                        if(!strcmp(name, recordName) && !strcmp(type, recordType))
                        {
                            recordId = jsonGetString(cloudflareRecordJson, "id");
                            break;
                        }
                    }
                    printf("RECORD ID: %s\n", recordId);

                    int typeInt = 0;
                    if(!strcmp(recordType, "A"))
                    {
                        typeInt = IPV4;
                    }
                    if(!strcmp(recordType, "AAAA"))
                    {
                        typeInt = IPV6;
                    }

                    // TO DO SEND UPDATE TO CLOUDFLARE
                    printf("SENDING CLOUDFLARE UPDATE\n\tID %s\n\tName %s\n\tType %s\n", recordId, recordName, recordType);
                }
                
            }
        }
        else
        {
            printf("unknown prodived '%s', skipping config\n", provider);
        }
    }   

    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/
    /* OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD*/

    /*
    char url[256];
    memset(url, 0, sizeof(url));
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s", zone, record);
    char ip[IPV6STRINGLENGTH];
    char type[5];
    memset(ip, 0, sizeof(ip));
    memset(type, 0, sizeof(type));
    if(ipv6)
    {
        memcpy(ip, ipAddresses.ipv6, strlen(ipAddresses.ipv6));
        strcpy(type, "AAAA");
    }
    else
    {
        memcpy(ip, ipAddresses.ipv4, strlen(ipAddresses.ipv4));
        strcpy(type, "A");
    }

    // 54 = "Content-Type: application/json\nAuthorization: "+ NULL
    // 40 = length of cloudflare token.
    // min lenght  = 94
    char headers[128];
    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Content-Type: application/json\nAuthorization: Bearer %s", token);

    char data[256];
    memset(data, 0, sizeof(data));
    sprintf(data,
        "{"
            "\"content\":\"%s\","
            "\"name\":\"%s\","
            "\"type\":\"%s\","
            "\"comment\":\"ddns-client: %s\""
        "}",
         ip, name, type, clientId);

    printf("Updating %s (%s) to %s\n", name, type, ip);
    bool success = httpsRequest(url, HTTPS_PUT, headers, data);
    if(success)
    {
        printf("RESULT: %s\n\n", httpsResult);
    }
    else
    {
        printf("FAILED\n\n");
    }

    sleep(throttleInterval);
    */
}

// TODO: Everything
void updateCloudflareRecords(IpAddresses addresses, int updated) {
 
    //read config.ini for cloudflare ids
    FILE *fp;
    fp = fopen("config.ini", "r");
    if (fp == NULL) {
        printf("Failed to open config.ini\n" );
        exit(1);
    }

    printf("\n");

    char line[250];
    char token[50];
    char zone[50];
    char name[255];
    char record[50];
    zone[0] = '\0';

    while(fgets(line, sizeof(line), fp) != NULL)
    {
        char newline = 0;
        if(line[strlen(line) - 1] == '\n')
        {
            newline = 1;
        }

        if(stringBeginsWithString(line, "token = "))
        {
            memset(token, 0, sizeof(token));
            memcpy(token, line + 8, strlen(line) - 8 - newline);
        }
        else if(stringBeginsWithString(line, "zone = "))
        {
            memset(zone, 0, sizeof(zone));
            memcpy(zone, line + 7, strlen(line) - 7 - newline);
        }
        else if(stringBeginsWithString(line, "name = "))
        {
            memset(name, 0, sizeof(name));
            memcpy(name, line + 7, strlen(line) - 7 - newline);
        }
        else if(stringBeginsWithString(line, "ipv4 = "))
        {
            if(updated & IPV4)
            {
                memset(record, 0, sizeof(record));
                memcpy(record, line + 7, strlen(line) - 7 - newline);
                setRecord(token, zone, name, record, addresses, false);
            }
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            if(updated & IPV6)
            {
                memset(record, 0, sizeof(record));
                memcpy(record, line + 7, strlen(line) - 7 - newline);
                setRecord(token, zone, name, record, addresses, true);
            }
        }
    }

    fclose(fp);   
}
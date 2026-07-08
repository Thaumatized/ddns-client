#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "https.h"
#include "c-jsonc/json.h"

#include "utils.h"
#include "ipUtils.h"

#include "cloudflare.h"

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

char clientId[128] = "undefined";
int checkInterval = 60; // in seconds
int throttleInterval = 10; // in seconds

int protocolsEnabled = 0;
IpAddresses ipAddresses = EMPTY_IP_ADDRESSES;

void getConfigurations()
{
    FILE *fp;
    fp = fopen("config.jsonc", "r");
    if (fp == NULL) {
        printf("Failed to open config.jsonc\n" );
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *string = malloc(fsize + 1);
    if (string == NULL) {
        printf("Failed to allocate space for config string\n" );
        exit(1);
    }
    fread(string, fsize, 1, fp);
    fclose(fp);
    string[fsize] = '\0';
    JSON *configRootJson = jsonParse(string);
    free(string);

    // Common
    {
        char *clientIdFromConfig = jsonGetString(configRootJson, "clientId");
        memset(clientId, '\0', sizeof(clientId));
        strncpy(clientId, clientIdFromConfig, sizeof(clientId-1));
        
        checkInterval = *jsonGetNumber(configRootJson, "checkInterval");
        throttleInterval = *jsonGetNumber(configRootJson, "throttleInterval");
    }

    // cant free the root, as cloudflare module keeps using its own config from within.
    //jsonFree(configRootJson);
}

// TODO: This is shit but working. Make it better pls.
int getRecord(char* token, char *zone, char* name, bool ipv6, char* out)
{
    char queryOutput[1024];
    memset(queryOutput, 0, 1024);

        char url[256];
    memset(url, 0, sizeof(url));
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/", zone);

    // 40 = length of cloudflare token.
    char headers[64];
    memset(headers, 0, sizeof(headers));
    sprintf(headers, "Authorization: Bearer %s", token);

    printf("GET RECORD %s \n", name);
    bool success = httpsRequest(url, HTTPS_GET, headers, NULL);

    int nameIndex = 0;
    for(;nameIndex < strlen(httpsResult); nameIndex++)
    {
        if(stringBeginsWithString(httpsResult+nameIndex, name)) break;
    }

    if(!stringBeginsWithString(httpsResult+nameIndex, name)) {
        printf("Failed to get with name %s\n", name);
        return 0;
    }

    // "1234567890abcdef1234567890abcdef\",\"name\":\"" -> 42 + null
    // "1234567890abcdef1234567890abcdef" -> 32
    memcpy(out, httpsResult+nameIndex-42, 32);
    return 1;
}

void update_ips(int ipsUpdated)
{
    updateCloudflareRecords(ipAddresses, ipsUpdated);
}

int main(int argc, char *argv[])
{
    httpsInitialize();
    getConfigurations();

    while(1)
    {
        IpAddresses updatedIps = getIpAddresses(protocolsEnabled);

        int ipsUpdated = 0;
        if(strcmp(ipAddresses.ipv4, updatedIps.ipv4) != 0)
        {
            printf("IPv4 address changed\n");
            ipsUpdated |= IPV4;
        }
        if(strcmp(ipAddresses.ipv6, updatedIps.ipv6) != 0)
        {
            printf("IPv6 address changed\n");
            ipsUpdated |= IPV6;
        }

        ipAddresses = updatedIps;

        if(ipsUpdated)
        {
            update_ips(ipsUpdated);
        }
        sleep(checkInterval);
    }

}
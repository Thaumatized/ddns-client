#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cloudflare.h"

JSON *clientConfig;
int protocolsEnabled;

void readConfigs()
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

    // clientConfig
    {
        clientConfig = jsonGetObject(configRootJson, "clientConfig");
        if(clientConfig == NULL)
        {
            printf("ERROR: configuration is missing clientConfig");
            exit(1);
        }

        char *clientId = jsonGetString(clientConfig, "clientId");
        if(clientId == NULL || strlen(clientId) == 0)
        {
            printf("ERROR: clientConfig.clientId is missing or empty");
            exit(1);
        }
        

        double *checkInterval = jsonGetNumber(clientConfig, "checkInterval");
        if(checkInterval == NULL)
        {
            printf("ERROR: clientConfig.checkInterval is missing");
            exit(1);
        }


        double *throttleInterval = jsonGetNumber(clientConfig, "throttleInterval");
        if(throttleInterval == NULL)
        {
            printf("ERROR: clientConfig.throttleInterval is missing");
            exit(1);
        }
    }

    // modules configs
    {
        JSON *cloudflareConfigRoot = jsonGetArray(configRootJson, "cloudflareConfigs");
        if(cloudflareConfigRoot != NULL)
        {
            protocolsEnabled |= setCloudflareConfigs(cloudflareConfigRoot);
            printf("Cloudflare configs done");
        }
        else
        {
            printf("No cloudflare config found, skpping\n");
        }
    }

    // cant free the root, as cloudflare module keeps using its own config from within.
    //jsonFree(configRootJson);
}
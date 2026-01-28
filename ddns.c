#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "https.h"
#include "json.h"

#define IPV4STRINGLENGTH 16 // 123.123.123.123 + Null
#define IPV6STRINGLENGTH 40 // 1234:5678:90AB:CDEF:1234:5678:90AB:CDEF + Null

#define IPV4 0b00000001
#define IPV6 0b00000010

//These are really defined already but vscode deos not know that
#ifndef NI_MAXHOST
    #define NI_MAXHOST 128
    #define NI_NUMERICHOST 1
#endif

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

char clientId[128] = "undefined";
int checkInterval = 60; // in seconds
int throttleInterval = 10; // in seconds

char protocolsEnabled = 0;
char ipv4Address[IPV4STRINGLENGTH] = "127.0.0.1";
char ipv6Address[IPV6STRINGLENGTH] = "::1";

typedef struct cloudflareConfig {
    int type;
    char *domain;
    char *zoneId;
    char *token;
    
    struct cloudflareConfig *next;
} cloudflareConfig;

cloudflareConfig *rootCloudflareConfig;

char stringBeginsWithString(char* string, char* beginsWith)
{
    return strncmp(string, beginsWith, strlen(beginsWith)) == 0;
}

char valid_ipv4(char *ipv4)
{
    int len = strlen(ipv4);
    if(len < 7 || len >= IPV4STRINGLENGTH)
    {
        return 0;
    }

    int i;
    for(i = 0; i < len; i++)
    {
        if(ipv4[i] == '.')
        {
            continue;
        }
        else if(ipv4[i] < '0' || ipv4[i] > '9')
        {
            return 0;
        }
    }

    return 1;
}

void get_ipv4(char *ipv4, char enabled)
{
    if (!enabled)
    {
        strcpy(ipv4, "127.0.0.1");
        return;
    }

    printf("Fetching ipv4:\n");
    bool success = httpsRequest("https://api.ipify.org", HTTPS_GET, NULL, NULL);

    if(success && valid_ipv4(httpsResult))
    {
        memset(ipv4, 0, IPV4STRINGLENGTH);
        memcpy(ipv4, httpsResult, strlen(httpsResult));
    }
    else
    {
        printf("Failed to get ipv4\n");
    }

    printf("IPv4: %s\n", ipv4);
}

char valid_ipv6(char *ipv6)
{
    int len = 0;
    while(
        ipv6[len] != '/'
        && ipv6[len] != '\0'
        && ipv6[len] != '\n'
        && ipv6[len] != ' '
    ){
        len++;
    }
    if(len < 2 || len >= IPV6STRINGLENGTH)
    {
        return 0;
    }

    int i;
    for(i = 0; i < len; i++)
    {
        if(ipv6[i] != ':'
            && (ipv6[i] < '0' || ipv6[i] > '9')
            && (ipv6[i] < 'a' || ipv6[i] > 'f')
            && (ipv6[i] < 'A' || ipv6[i] > 'F')
            )
        {
            return 0;
        }
    }

    //Exclude local address ::1
    if(ipv6[0] == ':')
    {
        return 0;
    }

    //Exclude link local addresses
    if(ipv6[0] == 'f' && ipv6[1] == 'e' && ipv6[2] == '8' && ipv6[3] == '0')
    {
        return 0;
    }

    //Exclude ULA addresses
    if(ipv6[0] == 'f' && (ipv6[1] == 'c'  || ipv6[1] == 'd'))
    {
        return 0;
    }

    return 1;
}

void get_ipv6(char *ipv6, char enabled)
{
    if (!enabled)
    {
        strcpy(ipv6, "::1");
        return;
    }

    printf("Fetching ipv6:\n");

    struct ifaddrs *result;
    char host[NI_MAXHOST];
    int success;

    success = getifaddrs(&result);
    if(success != 0)
    {
        printf("Failed to get ipv6, error %i\n", success);
    }
    else
    {
        for(struct ifaddrs *addrinfo = result; addrinfo != NULL; addrinfo = addrinfo->ifa_next) {
            if (addrinfo->ifa_addr == NULL)
                   continue;

            if(addrinfo->ifa_addr->sa_family == AF_INET6)
            {
                success = getnameinfo(
                    addrinfo->ifa_addr,
                    sizeof(struct sockaddr_in6),
                    host,
                    NI_MAXHOST,
                    NULL,
                    0,
                    NI_NUMERICHOST
                );
                if (success != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(success));
                    exit(EXIT_FAILURE);
                }
                printf("\tinterface: %s address: %s\n", addrinfo->ifa_name, host);
                if(valid_ipv6(host))
                {
                    strcpy(ipv6, host);
                }
            }
        }   
    }
    freeifaddrs(result);

    printf("IPv6: %s\n", ipv6);
}

char fetch_ips()
{
    printf("Fetching IPs\n");
    char ipv4[IPV4STRINGLENGTH];
    char ipv6[IPV6STRINGLENGTH];

    //These are important to prevent the program from updating the records if one of the IPs is invalid
    memcpy(ipv4, ipv4Address, sizeof(ipv4));
    memcpy(ipv6, ipv6Address, sizeof(ipv6));

    get_ipv4(ipv4, ipv4Enabled);
    get_ipv6(ipv6, ipv6Enabled);

    char returnValue = 0;

    if(strcmp(ipv4, ipv4Address) != 0)
    {
        printf("ipv4 address changed\n");
        memcpy(ipv4Address, ipv4, sizeof(ipv4));
        returnValue |= IPV4;
    }
    if(strcmp(ipv6, ipv6Address) != 0)
    {
        printf("ipv6 address changed\n");
        memcpy(ipv6Address, ipv6, sizeof(ipv6));
        returnValue |= IPV6;
    }

    return returnValue;
}

void getConfig()
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
    char error[JSON_MAX_ERROR];
    JSON *configRootJson = jsonParse(string, error);
    if(configRootJson == NULL)
    {
        printf("JSON PARSE ERROR: %s\n", error);
        exit(1);
    }
    free(string);

    // client id
    {
        char *clientIdFromConfig = jsonGetString(configRootJson, "clientId", error);
        if(*error != '\0')
        {
            printf("JSON ERROR: %s\n", error);
            printf("Failed to get clientId; defaulting to \"%s\"\n", clientId);
            memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
        }
        else
        {
            memset(clientId, '\0', sizeof(clientId));
            strncpy(clientId, clientIdFromConfig, sizeof(clientId-1));
        }
    }

    // checkInterval
    {
        double interval = jsonGetNumber(configRootJson, "checkInterval", error);
        if(*error != '\0')
        {
            printf("JSON ERROR: %s\n", error);
            printf("Failed to get checkInterval; defaulting to \"%g\"\n", checkInterval);
            memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
        }
        else
        {
            checkInterval = interval;
        }
    }

    // throttleInterval
    {
        double interval = jsonGetNumber(configRootJson, "throttleInterval", error);
        if(*error != '\0')
        {
            printf("JSON ERROR: %s\n", error);
            printf("Failed to get throttleInterval; defaulting to \"%g\"\n", throttleInterval);
            memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
        }
        else
        {
            throttleInterval = interval;
        }
    }

    //Configs
    {
    JSON *configJson = jsonGetArray(configRootJson, "configs", error);
        if(*error != '\0')
        {
            printf("JSON ERROR: %s\n", error);
            printf("Failed to get config\n");
            exit(1);
        }
        else
        {
            for (configJson = configJson->children; configJson != NULL; configJson = configJson->nextSibling)
            {
                if(configJson->type != JSON_OBJECT)
                {
                    printf("a config is not an object!\nFailed to get configs\n");
                    exit(1);
                }

                char *provider = jsonGetString(configJson, "provider", error);
                if(*error != '\0')
                {
                    printf("JSON ERROR: %s\n", error);
                    printf("Failed to get provider, skipping config\n");
                    memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
                    continue;
                }

                if(!strcmp(provider, "cloudflare"))
                {
                    char *token = jsonGetString(configJson, "token", error); 
                    if(*error != '\0')
                    {
                        printf("JSON ERROR: %s\n", error);
                        printf("Failed to get token, skipping config\n");
                        memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
                        continue;
                    }
                    token = strdup(token);

                    JSON *zoneJson = jsonGetArray(configJson, "zones", error);
                    if(zoneJson == NULL)
                    {
                        printf("JSON ERROR: %s\n", error);
                        printf("Failed to get zones, skipping config\n");
                        memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
                        continue;
                    }

                    
                    for (zoneJson = zoneJson->children; zoneJson != NULL; zoneJson = zoneJson->nextSibling)
                    {
                        char *zoneName = jsonGetString(zoneJson, "name", error); 
                        if(*error != '\0')
                        {
                            printf("JSON ERROR: %s\n", error);
                            printf("Failed to get zone name, skipping zone\n");
                            memset(error, '\0', JSON_MAX_ERROR); //non fatal error; clear error for future errors
                            continue;
                        }
                        zoneName = strdup(zoneName);

                        // TODO IMPLEMENT SHIT
                    }
                }
                else
                {
                    printf("unknown prodived '%s', skipping config\n", provider);
                }
            }
        }   
    }

    jsonFree(configRootJson);
}

void setRecord(char* token, char *zone, char* name, char *record, char ipv6)
{
    char url[256];
    memset(url, 0, sizeof(url));
    sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s", zone, record);
    char ip[IPV6STRINGLENGTH];
    char type[5];
    memset(ip, 0, sizeof(ip));
    memset(type, 0, sizeof(type));
    if(ipv6)
    {
        memcpy(ip, ipv6Address, strlen(ipv6Address));
        strcpy(type, "AAAA");
    }
    else
    {
        memcpy(ip, ipv4Address, strlen(ipv4Address));
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
}

char *getZone(char *token, char *)


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

void update_ips(char ipsUpdated)
{
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
            if(ipsUpdated & IPV4)
            {
                memset(record, 0, sizeof(record));
                memcpy(record, line + 7, strlen(line) - 7 - newline);
                setRecord(token, zone, name, record, 0);
            }
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            if(ipsUpdated & IPV6)
            {
                memset(record, 0, sizeof(record));
                memcpy(record, line + 7, strlen(line) - 7 - newline);
                setRecord(token, zone, name, record, 1);
            }
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    httpsInitialize();
    getConfig();

    while(1)
    {
        char ipsUpdated = fetch_ips();
        if(ipsUpdated)
        {
            update_ips(ipsUpdated);
        }
        sleep(checkInterval);
    }

}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>

#include "https.h"

#define IPV4STRINGLENGTH 16 // 123.123.123.123 + Null
#define IPV6STRINGLENGTH 40 // 1234:5678:90AB:CDEF:1234:5678:90AB:CDEF + Null

#define IPV4CHANGED 0b00000001
#define IPV6CHANGED 0b00000010

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

int checkInterval = 60; // in seconds
int throttleInterval = 10; // in seconds
char ipv4Enabled = 0;
char ipv6Enabled = 0;
char ipv4Address[IPV4STRINGLENGTH] = "127.0.0.1";
char ipv6Address[IPV6STRINGLENGTH] = "::1";

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

    /*
    FILE *fp;
    char path[150];
    memset(path, 0, sizeof(path));

    fp = popen("/bin/ip address | grep \"inet6\"", "r");
    if (fp == NULL) {
        printf("Failed to run command to get ipv6 \n" );
        exit(1);
    }
    
    char ipv6gotset = 0;
    while(fgets(path, sizeof(path), fp) != NULL)
    {
        printf("evaluating : %s", path);
        if(valid_ipv6(path + 10))
        {
            int ipv6len = 0;
            while(path[ipv6len+10] != '/' && ipv6len < IPV6STRINGLENGTH)
            {
                ipv6len++;
            }
            // + 10 is to jump over the "    inet6 " part of the string

            memset(ipv6, 0, IPV6STRINGLENGTH);
            memcpy(ipv6, path + 10, ipv6len);

            ipv6gotset = 1;
            break;
        }
        printf("rejected.\n");
        memset(path, 0, sizeof(path));
    };

    if(!ipv6gotset)
    {
        printf("Failed to get ipv6\n");
    }
        
    pclose(fp);
    */

    /*
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    //hints.ai_family = AF_INET6;
    hints.ai_family = AF_UNSPEC;

    int success = getaddrinfo("localhost", NULL, &hints, &result);

    if(success != 0)
    {
        printf("Failed to get ipv6, error %i\n", success);
    }
    else
    {
        struct addrinfo *looperAddrinfo = result;
        do {
            //printf("IPV6TEST %i\n", (*looperAddrinfo).ai_addrlen);
            printf("IPV6TEST %i ", (*looperAddrinfo).ai_family);
            for(int i = 0; i < (*looperAddrinfo).ai_addrlen; i++)
            {
                printf("%x-", (uint8_t)(*((*result).ai_addr)).sa_data[i]);
            }
            printf("\n");

            looperAddrinfo = (*looperAddrinfo).ai_next;
        } while (looperAddrinfo != NULL);   
    }
    */

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
                printf("\taddress: %s %i\n", host, addrinfo->ifa_flags);
            }
        }   
    }
    freeifaddrs(result);

    /*
               struct ifaddrs *ifaddr;
           int family, s;
           char host[NI_MAXHOST];

           if (getifaddrs(&ifaddr) == -1) {
               perror("getifaddrs");
               exit(EXIT_FAILURE);
           }

           // Walk through linked list, maintaining head pointer so we can free list later.

           for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
                    ifa = ifa->ifa_next) {
               if (ifa->ifa_addr == NULL)
                   continue;

               family = ifa->ifa_addr->sa_family;

               // Display interface name and family (including symbolic form of the latter for the common families).

               printf("%-8s %s (%d)\n",
                      ifa->ifa_name,
                      (family == AF_PACKET) ? "AF_PACKET" :
                      (family == AF_INET) ? "AF_INET" :
                      (family == AF_INET6) ? "AF_INET6" : "???",
                      family);

               // For an AF_INET6 interface address, display the address.

               if (family == AF_INET6) {
                   s = getnameinfo(
                        ifa->ifa_addr,
                        sizeof(struct sockaddr_in6),
                        host,
                        NI_MAXHOST,
                        NULL,
                        0,
                        NI_NUMERICHOST
                    );
                   if (s != 0) {
                       printf("getnameinfo() failed: %s\n", gai_strerror(s));
                       exit(EXIT_FAILURE);
                   }
                   printf("\t\taddress: <%s>\n", host);
               }
           }

           freeifaddrs(ifaddr);
    */

    //printf("IPv6: %s\n", ipv6);
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
        returnValue |= IPV4CHANGED;
    }
    if(strcmp(ipv6, ipv6Address) != 0)
    {
        printf("ipv6 address changed\n");
        memcpy(ipv6Address, ipv6, sizeof(ipv6));
        returnValue |= IPV6CHANGED;
    }

    return returnValue;
}

void getConfig()
{
    //read config.ini for values
    FILE *fp;
    fp = fopen("config.ini", "r");
    if (fp == NULL) {
        printf("Failed to open config.ini\n" );
        exit(1);
    }

    char line[250];

    //Used for verification
    unsigned int lineNumber = 0;
    unsigned int earliestToken = 0;
    unsigned int earliestZone = 0;
    unsigned int earliestName = 0;
    unsigned int earliestRecordId = 0;

    while(fgets(line, sizeof(line), fp) != NULL)
    {
        lineNumber++;
        if(stringBeginsWithString(line, "token = "))
        {
            if(earliestToken == 0)
            earliestToken = lineNumber;
        }
        else if(stringBeginsWithString(line, "zone = "))
        {
            if(earliestZone == 0)
            earliestZone = lineNumber;
        }
        else if(stringBeginsWithString(line, "name = "))
        {
            if(earliestName == 0)
            earliestName = lineNumber;
        }
        else if(stringBeginsWithString(line, "ipv4 = "))
        {
            ipv4Enabled = 1;
            if(earliestRecordId == 0)
            earliestRecordId = lineNumber;
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            ipv6Enabled = 1;
            if(earliestRecordId == 0)
            earliestRecordId = lineNumber;
        }
        else if(stringBeginsWithString(line, "interval = "))
        {
            checkInterval = atoi(line + 11);
        }
        else if(stringBeginsWithString(line, "throttle = "))
        {
            throttleInterval = atoi(line + 11);
        }
    }

    if(earliestToken == 0)
    {
        printf("No token specified in config.ini\n");
        exit(1);
    }
    if(earliestZone == 0)
    {
        printf("No zone specified in config.ini\n");
        exit(1);
    }
    if(earliestName == 0)
    {
        printf("No Name specified in config.ini\n");
        exit(1);
    }
    if(earliestRecordId == 0)
    {
        printf("No records specified in config.ini\n");
        exit(1);
    }

    if(earliestToken > earliestRecordId)
    {
        printf("A token must be defined before record Id:s config.ini, %u %u \n", earliestToken, earliestRecordId);
        exit(1);
    }
    if(earliestZone > earliestRecordId)
    {
        printf("A zone must be defined before record Id:s config.ini\n");
        exit(1);
    }
    if(earliestName > earliestRecordId)
    {
        printf("A name must be defined before record Id:s config.ini\n");
        exit(1);
    }

    fclose(fp);
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
    char* penis = "";

    char data[256];
    memset(data, 0, sizeof(data));
    sprintf(data,
        "{"
            "\"content\":\"%s\","
            "\"name\":\"%s\","
            "\"type\":\"%s\""
        "}",
         ip, name, type);

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
            if(ipsUpdated & IPV4CHANGED)
            {
                memset(record, 0, sizeof(record));
                memcpy(record, line + 7, strlen(line) - 7 - newline);
                setRecord(token, zone, name, record, 0);
            }
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            if(ipsUpdated & IPV6CHANGED)
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
    /*
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
        /*/

    char ipv6[1000];
    get_ipv6(ipv6, true);
    printf("IPV6: %s\n", ipv6);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "https.h"
#include "config.h"
#include "c-jsonc/json.h"

#include "utils.h"
#include "ipUtils.h"

#include "cloudflare.h"

IpAddresses ipAddresses = EMPTY_IP_ADDRESSES;



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
    printf("Updating IPs\n");
    updateCloudflareRecords(ipAddresses, ipsUpdated);
}

int main(int argc, char *argv[])
{
    printf("Initializing https\n");
    httpsInitialize();
    printf("Initializing configurations\n");
    readConfigs();

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
        sleep(*jsonGetNumber(clientConfig, "checkInterval"));
    }

}
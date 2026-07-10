
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdio.h>

#include <memory.h>

#include "ipUtils.h"
#include "https.h"

#include <netdb.h>
#include <ifaddrs.h>


//These are really defined already but vscode deos not know that
#ifndef NI_MAXHOST
    #define NI_MAXHOST 128
    #define NI_NUMERICHOST 1
#endif

bool valid_ipv4(char *ipv4)
{
    int len = strlen(ipv4);
    if(len < 7 || len >= IPV4STRINGLENGTH)
    {
        return false;
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
            return false;
        }
    }

    return true;
}

bool get_ipv4(char *out)
{
    printf("Fetching ipv4:\n");
    bool success = httpsRequest("https://api.ipify.org", HTTPS_GET, NULL, NULL);

    if(success && valid_ipv4(httpsResult))
    {
        memset(out, 0, IPV4STRINGLENGTH);
        memcpy(out, httpsResult, strlen(httpsResult));
    }
    else
    {
        printf("Failed to get ipv4\n");
    }

    printf("IPv4: %s\n", out);
}

bool valid_ipv6(char *ipv6)
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
        return false;
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
            return false;
        }
    }

    //Exclude local address ::1
    if(ipv6[0] == ':')
    {
        return false;
    }

    //Exclude link local addresses
    if(ipv6[0] == 'f' && ipv6[1] == 'e' && ipv6[2] == '8' && ipv6[3] == '0')
    {
        return false;
    }

    //Exclude ULA addresses
    if(ipv6[0] == 'f' && (ipv6[1] == 'c'  || ipv6[1] == 'd'))
    {
        return false;
    }

    return true;
}

bool get_ipv6(char *out)
{
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
                    strcpy(out, host);
                }
            }
        }   
    }
    freeifaddrs(result);

    printf("IPv6: %s\n", out);
}

IpAddresses getIpAddresses(int enabledFlags)
{
    printf("Fetching IPs, enabled: IPV4(%i), IPV6(%i)\n", enabledFlags & IPV4, enabledFlags & IPV6);
    IpAddresses ipAddresses = EMPTY_IP_ADDRESSES;

    if(enabledFlags & IPV4)
    {
        if(get_ipv4(ipAddresses.ipv4))
        {
            ipAddresses.validIpAddresses |= IPV4;
        }
    }
    if(enabledFlags & IPV6)
    {
        if(get_ipv6(ipAddresses.ipv6))
        {
            ipAddresses.validIpAddresses |= IPV6;
        }
    }

    printf("IP results:\n\tIPV4(%i): %s\n\tIPV6(%i): %s\n", ipAddresses.validIpAddresses & IPV4, ipAddresses.ipv4, ipAddresses.validIpAddresses & IPV6, ipAddresses.ipv6);

    return ipAddresses;
}

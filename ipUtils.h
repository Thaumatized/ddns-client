#pragma once
#include <stdbool.h>

#define IPV4STRINGLENGTH 16 // 123.123.123.123 + Null
#define IPV6STRINGLENGTH 40 // 1234:5678:90AB:CDEF:1234:5678:90AB:CDEF + Null

#define IPV4 0b00000001
#define IPV6 0b00000010

typedef struct IpAddresses
{
    int validIpAddresses;
    char ipv4[IPV4STRINGLENGTH];
    char ipv6[IPV6STRINGLENGTH];
} IpAddresses;

bool valid_ipv4(char *ipv4);

bool get_ipv4(char *ipv4);

bool valid_ipv6(char *ipv6);

bool get_ipv6(char *ipv6);

#define EMPTY_IP_ADDRESSES { \
        .validIpAddresses = 0, \
        .ipv4 = "123.123.123.123", \
        .ipv6 = "1234:5678:90AB:CDEF:1234:5678:90AB:CDEF", \
    };

IpAddresses getIpAddresses(int enabledFlags);
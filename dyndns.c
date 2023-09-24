#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
void get_ipv6(char *ipv6)
{
    FILE *fp;
    char path[150];

    /* Open the command for reading. */
    fp = popen("/bin/ip address | grep \"/64 scope global dynamic mngtmpaddr\"", "r");
    if (fp == NULL) {
        printf("Failed to run command to get ipv6 \n" );
        exit(1);
    }
    
    fgets(path, sizeof(path), fp);
    int ipv6len = 0;
    while(path[ipv6len+10] != '/')
    {
        ipv6len++;
    }
    // + 10 is to jump over the "    inet6 " part of the string
    memcpy(ipv6, path + 10, ipv6len);

    /* close */
    pclose(fp);
}

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output
void get_ipv4(char *ipv4)
{
    FILE *fp;
    char path[15];

    /* Open the command for reading. */
    fp = popen("/bin/curl https://api.ipify.org --silent", "r");
    if (fp == NULL) {
        printf("Failed to run command to get ipv4\n" );
        exit(1);
    }

    fgets(path, sizeof(path), fp);
    memcpy(ipv4, path, sizeof(path));

    /* close */
    pclose(fp);
}

int main(int argc, char *argv[])
{
    char ipv6[39];
    memset(ipv6, 0, sizeof(ipv6));
    get_ipv6(ipv6);

    char ipv4[15];
    memset(ipv4, 0, sizeof(ipv4));
    get_ipv4(ipv4);

    printf("ipv6: %s\n", ipv6);
    printf("ipv4: %s\n", ipv4);

    return 0;
}
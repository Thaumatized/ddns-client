#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

void get_ipv4(char *ipv4, char enabled)
{
    if (!enabled)
    {
        strcpy(ipv4, "127.0.0.1");
        return;
    }

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

void get_ipv6(char *ipv6, char enabled)
{
    if (!enabled)
    {
        strcpy(ipv6, "::1");
        return;
    }

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

int main(int argc, char *argv[])
{
    char token[50];
    memset(token, 0, sizeof(token));
    char v4 = 1;
    char v6 = 1;

    //read config.ini for values
    FILE *fp;
    fp = fopen("config.ini", "r");
    if (fp == NULL) {
        printf("Failed to open config.ini\n" );
        exit(1);
    }

    char line[250];
    while(fgets(line, sizeof(line), fp) != NULL)
    {
        if(strstr(line, "token = ") != NULL)
        {
            memcpy(token, line + 8, strlen(line) - 9); // -9 to get rid of the \n
        }
        else if(strstr(line, "ipv4 = ") != NULL)
        {
            v4 = line[7] - '0';
        }
        else if(strstr(line, "ipv6 = ") != NULL)
        {
            v6 = line[7] - '0';
        }
    }

    char ipv4[15];
    memset(ipv4, 0, sizeof(ipv4));
    get_ipv4(ipv4, v4);

    char ipv6[39];
    memset(ipv6, 0, sizeof(ipv6));
    get_ipv6(ipv6, v6);

    return 0;
}
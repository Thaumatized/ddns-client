#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

struct configuration
{
    char token[50];
    char v4;
    char v6;
    char ipv4[15];
    char ipv6[39];
};


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

    printf("IPv4: %s\n", ipv4);
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
    
    while(fgets(path, sizeof(path), fp) != NULL)
    {
        if(strncmp(path + 10, "fd", 2) == 0)
        {
            continue;
        }

        int ipv6len = 0;
        while(path[ipv6len+10] != '/')
        {
            ipv6len++;
        }
        // + 10 is to jump over the "    inet6 " part of the string
        memcpy(ipv6, path + 10, ipv6len);
    };

    /* close */
    pclose(fp);

    printf("IPv6: %s\n", ipv6);
}

void getConfig(struct configuration *config)
{
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
            memcpy(config->token, line + 8, strlen(line) - 8 - 1);
        }
        else if(strstr(line, "ipv4 = ") != NULL)
        {
            config->v4 = line[7] - '0';
        }
        else if(strstr(line, "ipv6 = ") != NULL)
        {
            config->v6 = line[7] - '0';
        }
    }

    if(config->token[0] == '\0')
    {
        printf("No token specified in config.ini\n");
        exit(1);
    }

    if(config->v4 == 0 && config->v6 == 0)
    {
        printf("Both ipv4 and ipv6 are disabled in config.ini\n");
        exit(1);
    }


    get_ipv4(config->ipv4, config->v4);
    get_ipv6(config->ipv6, config->v6);

    fclose(fp);
}

void setRecord(struct configuration *config, char *zone, char* name, char *record, char ipv6)
{
    if(zone[0] == '\0')
    {
        printf("record id %s specified before any zone", record);
        exit(1);
    }
    else
    {
        char url[200];
        memset(url, 0, sizeof(url));
        sprintf(url, "https://api.cloudflare.com/client/v4/zones/%s/dns_records/%s", zone, record);
        char ip[39];
        char type[5];
        memset(ip, 0, sizeof(ip));
        memset(type, 0, sizeof(type));
        if(ipv6)
        {
            memcpy(ip, config->ipv6, strlen(config->ipv6));
            strcpy(type, "AAAA");
        }
        else
        {
            memcpy(ip, config->ipv4, strlen(config->ipv4));
            strcpy(type, "A");
        }

        char command[1000];
        memset(command, 0, sizeof(command));
        sprintf(command, "/bin/curl \
        --request PUT \
        --url %s \
        --header 'Content-Type: application/json' \
        --header 'Authorization: Bearer %s' \
        --data '{ \
            \"content\": \"%s\", \
            \"name\": \"%s\", \
            \"type\": \"%s\", \
            \"proxied\": true \
        }' \
        ", url, config->token, ip, name, type);

        printf("Updating %s (%s) to %s\n", name, type, ip);
        system(command);
        printf("\n\n");
    }
}

int main(int argc, char *argv[])
{
    struct configuration config = {};
    getConfig(&config);

    //read config.ini for cloudflare ids
    FILE *fp;
    fp = fopen("config.ini", "r");
    if (fp == NULL) {
        printf("Failed to open config.ini\n" );
        exit(1);
    }

    printf("\n");

    char line[250];
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

        if(strstr(line, "zone = ") != NULL)
        {
            memset(zone, 0, sizeof(zone));
            memcpy(zone, line + 7, strlen(line) - 7 - newline);
        }
        else if(strstr(line, "name = ") != NULL)
        {
            memset(name, 0, sizeof(name));
            memcpy(name, line + 7, strlen(line) - 7 - newline);
        }
        else if(strstr(line, "ipv4 = ") != NULL)
        {

            memset(record, 0, sizeof(record));
            memcpy(record, line + 7, strlen(line) - 7 - newline);
            setRecord(&config, zone, name, record, 0);
        }
        else if(strstr(line, "ipv6 = ") != NULL)
        {
            memset(record, 0, sizeof(record));
            memcpy(record, line + 7, strlen(line) - 7 - newline);
            setRecord(&config, zone, name, record, 1);
        }
    }

    fclose(fp);

    return 0;
}
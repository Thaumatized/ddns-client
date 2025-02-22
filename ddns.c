#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define IPV4STRINGLENGTH 16 // 123.123.123.123 + Null
#define IPV6STRINGLENGTH 40 // 1234:5678:90AB:CDEF:1234:5678:90AB:CDEF + Null

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

struct configuration
{
    char token[50];
    char v4;
    char v6;
    int interval;
    char ipv4[IPV4STRINGLENGTH];
    char ipv6[IPV6STRINGLENGTH];
};

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

    FILE *fp;
    char path[IPV4STRINGLENGTH];
    memset(path, 0, sizeof(path));

    /* Open the command for reading. */
    fp = popen("/bin/curl https://api.ipify.org --silent --max-time 5", "r");
    if (fp == NULL) {
        printf("Failed to run command to get ipv4\n" );
        exit(1);
    }

    fgets(path, sizeof(path), fp);
    if(valid_ipv4(path))
    {
        memset(ipv4, 0, IPV4STRINGLENGTH);
        memcpy(ipv4, path, sizeof(path));
    }
    else
    {
        printf("Failed to get ipv4\n");
    }

    /* close */
    pclose(fp);

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

    FILE *fp;
    char path[150];
    memset(path, 0, sizeof(path));

    /* Open the command for reading. */
    fp = popen("/bin/ip address | grep \"/64 scope global dynamic mngtmpaddr\"", "r");
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

    /* close */
    pclose(fp);

    printf("IPv6: %s\n", ipv6);
}

char fetch_ips(struct configuration *config)
{
    printf("Fetching IPs\n");
    char ipv4[IPV4STRINGLENGTH];
    char ipv6[IPV6STRINGLENGTH];

    //These are important to prevent the program from updating the records if one of the IPs is invalid
    memcpy(ipv4, config->ipv4, sizeof(ipv4));
    memcpy(ipv6, config->ipv6, sizeof(ipv6));

    get_ipv4(ipv4, config->v4);
    get_ipv6(ipv6, config->v6);

    if(strcmp(ipv4, config->ipv4) != 0 || strcmp(ipv6, config->ipv6) != 0)
    {
        printf("IPs changed\n");
        memcpy(config->ipv4, ipv4, sizeof(ipv4));
        memcpy(config->ipv6, ipv6, sizeof(ipv6));
        printf("\n");
        return 1;
    }
    else
    {
        printf("\n");
        return 0;
    }
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
        else if(strstr(line, "interval = ") != NULL)
        {
            config->interval = atoi(line + 11);
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
        char ip[IPV6STRINGLENGTH];
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
            \"type\": \"%s\" \
        }' \
        ", url, config->token, ip, name, type);

        printf("Updating %s (%s) to %s\n", name, type, ip);
        system(command);
        printf("\n\n");
    }
}

void update_ips(struct configuration * config)
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
            setRecord(config, zone, name, record, 0);
        }
        else if(strstr(line, "ipv6 = ") != NULL)
        {
            memset(record, 0, sizeof(record));
            memcpy(record, line + 7, strlen(line) - 7 - newline);
            setRecord(config, zone, name, record, 1);
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    struct configuration config = {};
    getConfig(&config);

    while(1)
    {
        if(fetch_ips(&config))
        {
            update_ips(&config);
        }
        sleep(config.interval);
    }
}
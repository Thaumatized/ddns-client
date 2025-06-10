#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define IPV4STRINGLENGTH 16 // 123.123.123.123 + Null
#define IPV6STRINGLENGTH 40 // 1234:5678:90AB:CDEF:1234:5678:90AB:CDEF + Null

// huge thanks to https://stackoverflow.com/questions/646241/c-run-a-system-command-and-get-output

char stringBeginsWithString(char* string, char* beginsWith)
{
    return strncmp(string, beginsWith, strlen(beginsWith)) == 0;
}

struct configuration
{
    int interval;
    int throttle;
    char v4;
    char v6;
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
            config->v4 = 1;
            if(earliestRecordId == 0)
            earliestRecordId = lineNumber;
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            config->v6 = 1;
            if(earliestRecordId == 0)
            earliestRecordId = lineNumber;
        }
        else if(stringBeginsWithString(line, "interval = "))
        {
            config->interval = atoi(line + 11);
        }
        else if(stringBeginsWithString(line, "throttle = "))
        {
            config->throttle = atoi(line + 11);
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

void setRecord(struct configuration *config, char* token, char *zone, char* name, char *record, char ipv6)
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
    ", url, token, ip, name, type);

    printf("Updating %s (%s) to %s\n", name, type, ip);
    system(command);
    printf("\n\n");

    sleep(config->throttle);
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

            memset(record, 0, sizeof(record));
            memcpy(record, line + 7, strlen(line) - 7 - newline);
            setRecord(config, token, zone, name, record, 0);
        }
        else if(stringBeginsWithString(line, "ipv6 = "))
        {
            memset(record, 0, sizeof(record));
            memcpy(record, line + 7, strlen(line) - 7 - newline);
            setRecord(config, token, zone, name, record, 1);
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    struct configuration config = {
        60, // interval
        10, // throttle;
        0, //v4
        0, //v6
        "", // ipv4
        "" // ipv6
    };
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
#include <stdbool.h>
#include <string.h>

bool stringBeginsWithString(char* string, char* beginsWith)
{
    return strncmp(string, beginsWith, strlen(beginsWith)) == 0;
}
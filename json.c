#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

#include <stdio.h>

JSON *jsonParseRecursor(char *data, char* label, char *error);

JSON *jsonParse(char *data, char *error) {

    char *cleanedData = malloc(strlen(data)+1);
    int dataIndex = 0;
    int cleanedIndex = 0;


    while (data[dataIndex] != '\0')
    {
        // whitespace
        if(isspace((unsigned char)data[dataIndex]))
        {
            dataIndex++;
            continue;
        }

        /* multiline comments */
        if(data[dataIndex] == '/' && data[dataIndex+1] == '*')
        {
            dataIndex += 2;
            while (data[dataIndex] != '*' || data[dataIndex+1] != '/')
            {
               dataIndex++;
            }
            dataIndex += 2;
            continue;
        }
        
        // single line comments
        if(data[dataIndex] == '/' && data[dataIndex+1] == '/')
        {
            dataIndex += 2;
            while (data[dataIndex] != '\n')
            {
               dataIndex++;
            }
            dataIndex++;
            continue;
        }

        // strings (copy including whitespace and "comments")
        if(data[dataIndex] == '"')
        {
            int stringLength = 1;
            bool escaped = false;
            while (escaped || data[dataIndex+stringLength] != '"')
            {
                escaped = data[dataIndex+stringLength] == '\\' && !escaped;
                stringLength++;
            }
            stringLength += 1; // closing quote

            memcpy(cleanedData+cleanedIndex, data+dataIndex, stringLength);
            dataIndex += stringLength;
            cleanedIndex += stringLength;
            continue;
        }

        cleanedData[cleanedIndex] = data[dataIndex];
        dataIndex++;
        cleanedIndex++;
    }
    cleanedData[cleanedIndex] = '\0';

    
    JSON *json = jsonParseRecursor(cleanedData, NULL, error);

    free(cleanedData);

    return json;
}

JSON *jsonParseRecursor(char *data, char* label, char *error) {

    JSON * newJson = malloc(sizeof(JSON));
    if(newJson == NULL)
    {
        snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
        return NULL;
    }


    // boolean - true
    if(!strcmp(data, "true"))
    {
        *newJson = (JSON){
            .type = JSON_BOOLEAN,
            .label = label,
            .boolean = true,
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        return newJson;
    }

    // boolean - false
    if(!strcmp(data, "false"))
    {
        *newJson = (JSON){
            .type = JSON_BOOLEAN,
            .label = label,
            .boolean = false,
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        return newJson;
    }

    // null
    if(!strcmp(data, "null"))
    {
        *newJson = (JSON){
            .type = JSON_NULL,
            .label = label,
            .boolean = false,
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        return newJson;
    }
    
    // number
    if(isdigit(data[0]) || data[0] == '-')
    {
        char *end = NULL;
        *newJson = (JSON){
            .type = JSON_NUMBER,
            .label = label,
            .number = strtod(data, &end),
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        if((*end) != '\0')
        {
            free(newJson);
            snprintf(error, JSON_MAX_ERROR, "data '%s' does not appear to be a valid json value", data);
            return NULL;
        }

        return newJson;
    }

    // string
    if(data[0] == '"')
    {
        int stringLength = 1;
        bool escaped = false;
        while (escaped || data[stringLength] != '"')
        {
            escaped = data[stringLength] == '\\' && !escaped;
            stringLength++;
        }
        // We don't need to add null terminator space to stringLength, as we want to strip the quotes, and the first one is taking space already.

        *newJson = (JSON){
            .type = JSON_STRING,
            .label = label,
            .string = malloc(stringLength),
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        if(newJson->string == NULL)
        {
            snprintf(error, JSON_MAX_ERROR, "FAILED TO ALLOCATE MEMORY");                    
            free(newJson);
            return NULL;
        }
        
        strncpy(newJson->string, data+1, stringLength-1);
        newJson->string[stringLength-1] = '\0';

        return newJson;
    }

    // arrays and objects
    if(data[0] == '[' || data[0] == '{')
    {
        bool object = data[0] == '{';
        char *childLabel = NULL;

        JSON *firstChild = NULL;
        JSON *lastChild = NULL;
        JSON *currentChild = NULL;

        int dataIndex = 1;
        int depth = 1;
        while ((data[dataIndex] != ']' && data[dataIndex] != '}') || depth > 1)
        {
            int stringLength = 1;
            bool inAString = data[dataIndex] == '"';
            bool escaped = false;

            // For objects we need to get child label before getting child json string
            if(object)
            {
                if(!inAString)
                {
                    snprintf(error, JSON_MAX_ERROR, "JSON field inside of an object appears to be missing a label in %s", data);    
                    goto errorCleanup;
                }

                dataIndex++; // skip the quotation mark

                while (escaped || data[dataIndex+stringLength] != '"')
                {
                    escaped = data[dataIndex+stringLength] == '\\' && !escaped;
                    stringLength++;
                }
                stringLength += 1; // NULL terminator

                childLabel = malloc(stringLength);
                strncpy(childLabel, data+dataIndex, stringLength-1);
                childLabel[stringLength-1] = '\0';

                // Since data index has space for NULL terminator this also skips the colon
                dataIndex += stringLength;
                // However, we need to make sure it exists; Otherwise the json is malformed which is likely a mistake on the part of the user
                if(data[dataIndex] != ':')
                {
                    snprintf(error, JSON_MAX_ERROR, "JSON field inside of an object appears to be missing a colon between label and data %s", data);
                    goto errorCleanup;
                }
                dataIndex++;

                // Reset the common variables for fetching the actual child object
                stringLength = 1;
                inAString = data[dataIndex] == '"';
                escaped = false;
            }

            //Everything but objects and arrays
            if(data[dataIndex] != '{' && data[dataIndex] != '[')
            {
                while (inAString || (data[dataIndex+stringLength] != ',' && data[dataIndex+stringLength] != ']' && data[dataIndex+stringLength] != '}'))
                {
                    char c = data[dataIndex+stringLength];

                    if(!escaped && c == '"')
                    {
                        inAString = !inAString;
                    }
                    escaped = c == '\\' && !escaped;

                    stringLength++;
                }
                stringLength++; // NULL terminator
            }
            else // Objects and arrays
            {
                depth++;
                while (depth > 1)
                {
                    char c = data[dataIndex+stringLength];
                    
                    if(!inAString)
                    {
                        if(c == '{' || c == '[')
                        {
                            depth++;
                        }
                        if(c == '}' || c == ']')
                        {
                            depth--;
                        }
                    }

                    if(!escaped && c == '"')
                    {
                        inAString = !inAString;
                    }
                    escaped = c == '\\' && !escaped;

                    stringLength++;
                }
                stringLength++; // NULL terminator
            }

            // childString definition would collide with errorCleanup jump without separate block
            {
                char childString[stringLength];
                strncpy(childString, data+dataIndex, stringLength);
                childString[stringLength-1] = '\0';
                currentChild = jsonParseRecursor(childString, childLabel, error);
            }

            // error handled by child
            if(currentChild == NULL)
            {
                errorCleanup:

                if(firstChild != NULL)
                {    
                    currentChild = firstChild;
                    while (currentChild != NULL)
                    {
                        jsonFree(currentChild);
                        currentChild = currentChild->nextSibling;
                    }
                }
                return NULL;
            }

            if(firstChild == NULL)
            {
                firstChild = currentChild;
            }
            if(lastChild != NULL)
            {
                lastChild->nextSibling = currentChild;
                currentChild->previousSibling = lastChild;
            }
            lastChild = currentChild;

            // Since stringLength has space for NULL terminator, it also conveniently skips over the comma
            dataIndex += stringLength;
            // However, if this was the last item and there is no trailing comma, 
            // we would jump over the closing bracket.
            if(data[dataIndex-1] == '}' || data[dataIndex-1] == ']')
            {
                break;
            }
            // And, if this *wasnt* the last item, and there is no "trailing" comma, we have malformed json
            if(data[dataIndex-1] != ',')
            {
                snprintf(error, JSON_MAX_ERROR, "Two JSON fields inside an object/array appear to be missing a comma from between them in %s", data);
                goto errorCleanup;
                break;
            }
        }
        
        *newJson = (JSON){
            .type = object ? JSON_OBJECT : JSON_ARRAY,
            .label = label,
            .children = firstChild,
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        return newJson;
    }


    snprintf(error, JSON_MAX_ERROR, "data '%s' does not appear to be a valid json value", data);

    return NULL;
}

char *jsonStringify(JSON *data, char *error) {
    if(data == NULL)
    {
        char *retstring = strdup("null");
        if(retstring == NULL)
        {
            snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
            return NULL;
        }
        return retstring;
    }
    
    switch (data->type)
    {
    case JSON_OBJECT:
    case JSON_ARRAY:
        {
            bool object = data->type == JSON_OBJECT;

            int childCount = 0;
            JSON * childPointer = data->children;
            while (childPointer != NULL)
            {
                childCount++;
                childPointer = childPointer->nextSibling;
            }

            char *childStrings[childCount];
            int bufferSize = 0;
            childPointer = data->children;
            for(int childIndex = 0; childIndex < childCount; childIndex++)
            {
                childStrings[childIndex] = jsonStringify(childPointer, error);
                if(childStrings == NULL)
                {
                    for(int index = 0; index < childIndex; index++)
                    {
                        free(childStrings[index]);
                    }
                    return NULL;
                }
                bufferSize += object ? strlen(childStrings[childIndex]) + strlen(childPointer->label) : strlen(childStrings[childIndex]);
                childPointer = childPointer->nextSibling;
            }

            if(object)
            {
                // + 3 for brackets and NULL termiantor
                // + childCount*4-1 for quotation marks, colons and commas
                // (labels are wrapped in quotations, colons after each label and comma after each item except the last one)
                bufferSize += 3 + childCount*4-1;
            }
            else
            {
                // + 3 for brackets and NULL termiantor
                // + childCount-1 for commas
                bufferSize += 3 + childCount-1;
            }

            char *retstring = malloc(bufferSize);
            if(retstring == NULL)
            {
                snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");                    
                for(int index = 0; index < childCount; index++)
                {
                    free(childStrings[index]);
                }
                return NULL;
            }

            retstring[0] = object ? '{' : '[';
            int lengthSoFar = 1;
            childPointer = data->children;
            for(int childIndex = 0; childIndex < childCount; childIndex++)
            {
                if(object)
                {
                    retstring[lengthSoFar] = '"';
                    lengthSoFar+=1;

                    strcpy(retstring+lengthSoFar, childPointer->label);
                    lengthSoFar += strlen(childPointer->label);

                    retstring[lengthSoFar] = '\"';
                    retstring[lengthSoFar+1] = ':';
                    lengthSoFar+=2;
                }


                strcpy(retstring+lengthSoFar, childStrings[childIndex]);
                lengthSoFar += strlen(childStrings[childIndex]);

                retstring[lengthSoFar] = ',';
                lengthSoFar+=1;

                free(childStrings[childIndex]);
                childPointer = childPointer->nextSibling;
            }

            if(childCount > 0)
            {
                // -1 to overwrite the last comma
                retstring[lengthSoFar-1] = object ? '}' : ']'; 
                retstring[lengthSoFar] = '\0'; 
            }
            else
            {
                retstring[lengthSoFar] = object ? '}' : ']'; 
                retstring[lengthSoFar+1] = '\0'; 
            }

            return retstring;
        }
        break;
    case JSON_NUMBER:
        {
            int bufferSize = snprintf(NULL, 0, "%G", data->number) +1; // +1 for NULL terminator
            char *retstring = malloc(bufferSize);
            if(retstring == NULL)
            {
                snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
                return NULL;
            }
            snprintf(retstring, bufferSize, "%G", data->number);
            return retstring;
        }
        break;
    case JSON_STRING:
        {
            int bufferSize = strlen(data->string)+3; // +3 for quotes and null
            char *retstring = malloc(bufferSize);
            if(retstring == NULL)
            {
                snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
                return NULL;
            }
            sprintf(retstring, "\"%s\"", data->string);
            return retstring;
        }
        break;
    case JSON_BOOLEAN:
        {
            char *retstring;
            if(data->boolean)
                retstring = strdup("true");
            else
                retstring = strdup("false");
            if(retstring == NULL)
            {
                snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
                return NULL;
            }
            return retstring;
        }
        break;
    case JSON_NULL:
        {
            char *retstring = strdup("null");
            if(retstring == NULL)
            {
                snprintf(error, JSON_MAX_ERROR,  "FAILED TO ALLOCATE MEMORY");
                return NULL;
            }
            return retstring;
        }
        break;
    }    
}

void jsonFree(JSON *json) {
    if(json == NULL)
        return;

    if(json->type == JSON_STRING)
        free(json->string);

    if(json->type == JSON_OBJECT || json->type == JSON_ARRAY)
    {
        JSON * childPointer = json->children;
        while (childPointer != NULL)
        {;
            JSON *nextChild = childPointer->nextSibling;
            jsonFree(childPointer);
            childPointer = nextChild;
        }
    }

    if(json->label != NULL)
        free(json->label);

    free(json);
}

bool isKeySeparator(char c)
{
    return c == '\0' || c == '.' || c == '[' ||  c == ']';
}

JSON *jsonGetKey(JSON *json, const char *key, char *error)
{
    int keyIndex = 0;
    int keyLength = 0;
    JSON *currentJson = json;

    while (key[keyIndex] != '\0')
    {
        bool isNumberKey = key[keyIndex] == '[';
        if(isKeySeparator(key[keyIndex])) keyIndex++;
        keyLength = 0;
        while(!isKeySeparator(key[keyIndex+keyLength]))
        {
            keyLength++;
        }

        char keyBuffer[keyLength+1];
        strncpy(keyBuffer, key+keyIndex, keyLength);
        keyBuffer[keyLength] = '\0';

        if(isNumberKey)
        {

            if(currentJson->type != JSON_ARRAY)
            {
                snprintf(error, JSON_MAX_ERROR, "JSON element \"%s\" is not of type ARRAY, but was attempted to index into with index %s", currentJson->label, keyBuffer);
                return NULL;
            }

            JSON *child = currentJson->children;
            for(int key = atoi(keyBuffer); key > 0 && child != NULL; key--)
            {
                child = child->nextSibling;
            }

            if(child == NULL)
            {
                snprintf(error, JSON_MAX_ERROR, "index %s is out of bounds in json elment %s", keyBuffer, currentJson->label);
                //return NULL;
                child=currentJson;
            }

            currentJson = child;

            //skip the closing bracket
            keyIndex++;
        }
        else
        {
            JSON *child = currentJson->children;
            while(child != NULL && strcmp(child->label, keyBuffer) != 0)
            {
                child = child->nextSibling;
            }

            if(child == NULL)
            {
                snprintf(error, JSON_MAX_ERROR, "key %s no found in %s", keyBuffer, currentJson->label);
                return NULL;
            }

            currentJson = child;
        }
        keyIndex += keyLength;
    }

    return currentJson;
}

JSON *jsonGetObject(JSON *json, const char *key, char *error)
{
    JSON* objectJson = jsonGetKey(json, key, error);
    if(objectJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (objectJson->type != JSON_OBJECT)
    {
        snprintf(error, JSON_MAX_ERROR, "json with key %s is not an object", key);
        return NULL;
    }
    else
    {
        return objectJson;
    }
}

JSON *jsonGetArray(JSON *json, const char *key, char *error)
{
    JSON* arrayJson = jsonGetKey(json, key, error);
    if(arrayJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (arrayJson->type != JSON_OBJECT)
    {
        snprintf(error, JSON_MAX_ERROR, "json with key %s is not an array", key);
        return NULL;
    }
    else
    {
        return arrayJson;
    }
}

char *jsonGetString(JSON *json, const char *key, char *error)
{
    JSON* stringJson = jsonGetKey(json, key, error);
    if(stringJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (stringJson->type != JSON_STRING)
    {
        snprintf(error, JSON_MAX_ERROR, "json with key %s is not a string", key);
        return NULL;
    }
    else
    {
        return stringJson->string;
    }
}

double jsonGetNumber(JSON *json, const char *key, char *error)
{
    JSON* numberJson = jsonGetKey(json, key, error);
    if(numberJson == NULL) {
        // Error handled by jsonGetKey
        return 0;
    }
    else if (numberJson->type != JSON_NUMBER)
    {
        snprintf(error, JSON_MAX_ERROR, "json with key %s is not a number", key);
        return 0;
    }
    else
    {
        return numberJson->number;
    }
}

bool jsonGetBool(JSON *json, const char *key, char *error)
{
    JSON* boolJson = jsonGetKey(json, key, error);
    if(boolJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (boolJson->type != JSON_STRING)
    {
        snprintf(error, JSON_MAX_ERROR, "json with key %s is not a boolean", key);
        return NULL;
    }
    else
    {
        return boolJson->boolean;
    }
}
#pragma once
#include <stdbool.h>

/**
 * json.h is an JSONC parser implementation
 * What is JSONC? JSONC stands for JSON with Comments.
 * JSONC differs from JSON by allowing JavaScript style comments and trailing commas.
 * It is a superset of JSON and thus the parser can also parse JSON.
 * The stringifyer also outputs as JSON.
 * 
 * JSONC is unsuprisingly a name used for several projects.
 * Here it is specifically referring to https://github.com/JSONC-org/JSONC
 * 
 * For now we only support ASCII charset, which is not inline with the JSON spec which requires utf-8.
 * This is ofcourse fine as long as the data does not include any unicode characters 
 * and should be okay even with unicode charactres if none of them can be confused for ASCII when looking at single bytes".
 * 
 * This implementation is also locale dependendt. By default C defaults to the POSIX locale which is okay.
 * The bit that matters for this implementation is the decimal separator, which must be a dot.
 */

typedef enum { JSON_OBJECT, JSON_ARRAY, JSON_NUMBER, JSON_STRING, JSON_BOOLEAN, JSON_NULL } JSON_TYPES;

typedef struct JSON {
    JSON_TYPES type;
    char *label;

    union {
        struct JSON *children;
        double number;
        char *string;
        bool boolean;
        // NULL ofcourse has no data
    };
    
    struct JSON *previousSibling;
    struct JSON *nextSibling;
} JSON;

static int JSON_MAX_ERROR = 1024;

JSON *jsonParse(char *data, char *error);
char *jsonStringify(JSON *data, char *error);
void jsonFree(JSON *json);

JSON *jsonGetKey(JSON *json, const char *key, char *error);
JSON *jsonGetObject(JSON *json, const char *key, char *error);
JSON *jsonGetArray(JSON *json, const char *key, char *error);
char *jsonGetString(JSON *json, const char *key, char *error);
double jsonGetNumber(JSON *json, const char *key, char *error);
bool jsonGetBool(JSON *json, const char *key, char *error);
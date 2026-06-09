#pragma once
#include <stdbool.h>

 /**
  * JSON_TYPES:
  *     JSON_OBJECT,
  *     JSON_ARRAY,
  *     JSON_NUMBER,
  *     JSON_STRING,
  *     JSON_BOOLEAN,
  *     JSON_NULL,
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

JSON *jsonParse(char *data);
char *jsonStringify(JSON *data);
void jsonFree(JSON *json);

JSON *jsonGetKey(JSON *json, const char *key);
JSON *jsonGetObject(JSON *json, const char *key);
JSON *jsonGetArray(JSON *json, const char *key);
char *jsonGetString(JSON *json, const char *key);
double *jsonGetNumber(JSON *json, const char *key);
bool *jsonGetBool(JSON *json, const char *key);

bool jsonSetKey(JSON *parentJson, JSON *newChildJson, const char *key);
bool jsonSetObject(JSON *parentJson, const char *key);
bool jsonSetArray(JSON *parentJson, const char *key);
bool jsonSetString(JSON *parentJson, JSON *newChildJson, const char *key);
bool jsonSetNumber(JSON *parentJson, JSON *newChildJson, const char *key);
bool jsonSetBool(JSON *parentJson, JSON *newChildJson, const char *key);
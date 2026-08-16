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
    /**
     * label is empty for children of JSON_ARRAY, to make inserting elements in between easy
     */
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

/**
 * Places newChildJson inside parentJsons path according to key.
 * Will add JSON_OBJECTs and JSON_ARRAYs and JSON_NULLs (inside JSON_ARRAYs) as needed to make the path.
 * Will replace the exact key if it is pre-existing.
 * Will exit if path contains types that can't be indexed into.
 * 
 * DO NOT give newChildJson a label -- this will overwritten, and cause a memoryleak
 */
void jsonSetKey(JSON *parentJson, JSON *newChildJson, const char *key);
void jsonSetObject(JSON *parentJson, JSON *newChildJson, const char *key);
void jsonSetArray(JSON *parentJson, JSON *newChildJson, const char *key);
void jsonSetString(JSON *parentJson, const char *string, const char *key);
void jsonSetNumber(JSON *parentJson, const double number, const char *key);
void jsonSetBool(JSON *parentJson, const bool boolean, const char *key);

/*
    TODO:
    void jsonRemoveKey(JSON *json, const char *key);
    void addIntoArray(JSON *array, JSON *newChild);
*/
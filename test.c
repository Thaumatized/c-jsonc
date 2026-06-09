#include "json.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> 

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

bool allSuccess = true;

typedef union {
    struct JSON *children;
    double number;
    char *string;
    bool boolean;
    // NULL ofcourse has no data
} JSON_DATA_UNION;

char *numberToString(double number)
{
    char *string = malloc(512);
    sprintf(string, "%G", number);
    return string;
}

char *fileAsString(const char *filename)
{
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Failed to open %s\n", filename);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *string = malloc(fsize + 1);
    if (string == NULL) {
        printf("Failed to allocate space %s as a string\n", filename);
        exit(1);
    }
    fread(string, fsize, 1, fp);
    fclose(fp);
    string[fsize] = '\0';

    return string;
}

void testCategory(const char *categoryName)
{
    printf("\n" ANSI_COLOR_BLUE "CATEGORY: " ANSI_COLOR_RESET "%s\n", categoryName);
}

void expectString(const char *testName, const char *expected, const char *received)
{
    bool success = strcmp(expected, received) == 0;
    if(success)
    {
        printf("\t" ANSI_COLOR_GREEN "SUCCESS" ANSI_COLOR_RESET " %s\n", testName);
    }
    else
    {
        printf("\t" ANSI_COLOR_RED "FAILURE" ANSI_COLOR_RESET " %s\n\t\texpected %s\n\t\treceived %s\n", testName, expected, received);
    }
    allSuccess &= success;
}

void expectNumber(const char *testName, double expected, double received)
{
    char *expextedString = numberToString(expected);
    char *receivedString = numberToString(received);
    expectString(testName, expextedString, receivedString);
    free(expextedString);
    free(receivedString);
}

void expectPointer(const char *testName, void *expected, void *received)
{
    char *expextedString = malloc(256);
    sprintf(expextedString, "%p", expected);
    char *receivedString = malloc(256);
    sprintf(receivedString, "%p", received);
    expectString(testName, expextedString, receivedString);
    free(expextedString);
    free(receivedString);
}

/**
 * parses asString to json, makes sure the json seems right, and stringifies it again, comparing to expectedString, or if that is null, asString
 */
void testParseAndStringify(const char *testName, char *asString, JSON_DATA_UNION *expectedRawValue, JSON_TYPES expectedType, const char *expectedString)
{
    int testNameLongLength = strlen(testName) + 32;
    char typeTestName[testNameLongLength], valueTestName[testNameLongLength], stringifyTestName[testNameLongLength];
    memset(typeTestName, '\0', testNameLongLength);
    strcpy(typeTestName, testName);
    strcat(typeTestName, ", type");
    memset(valueTestName, '\0', testNameLongLength);
    strcpy(valueTestName, testName);
    strcat(valueTestName, ", value");
    memset(stringifyTestName, '\0', testNameLongLength);
    strcpy(stringifyTestName, testName);
    strcat(stringifyTestName, ", stringify");

    typedef enum { JSON_OBJECT, JSON_ARRAY, JSON_NUMBER, JSON_STRING, JSON_BOOLEAN, JSON_NULL } JSON_TYPES;

    JSON *testJson = jsonParse(asString);
    expectNumber(typeTestName, expectedType, testJson->type);
    if(expectedRawValue != NULL)
    {
        switch (expectedType)
        {
            case JSON_OBJECT:
                expectPointer(valueTestName, expectedRawValue->children, testJson->children);
                break;
            case JSON_ARRAY:
                expectPointer(valueTestName, expectedRawValue->children, testJson->children);
                break;
            case JSON_NUMBER:
                expectNumber(valueTestName, expectedRawValue->number, testJson->number);
                break;
            case JSON_STRING:
                expectString(valueTestName, expectedRawValue->string, testJson->string);
                break;
            case JSON_BOOLEAN:
                expectNumber(valueTestName, expectedRawValue->boolean, testJson->boolean);
                break;
            case JSON_NULL:
                break;
            default:
                printf(ANSI_COLOR_RED "WARNING: UNKNOWN JSON TYPE: %i\n" ANSI_COLOR_RESET, expectedType);
                break;
        }
    }
    char *testString = jsonStringify(testJson);
    const char *expectedStringityResult = asString;
    if(expectedString != NULL) expectedStringityResult = expectedString;
    expectString(stringifyTestName, expectedStringityResult, testString);

    jsonFree(testJson);
    free(testString);
}

int main(int argc, char const *argv[])
{
    testCategory("PREP");
    JSON *testJson = NULL;
    JSON_DATA_UNION testDataUnion = {};

    testCategory("NUMBER");
    testDataUnion.number = 42;
    testParseAndStringify("Integer 42", "42", &testDataUnion, JSON_NUMBER, NULL);
    testDataUnion.number = 3.14159;
    testParseAndStringify("Decimal 3.14159", "3.14159", &testDataUnion, JSON_NUMBER, NULL);
    testDataUnion.number = -7.18723;
    testParseAndStringify("Negative -7.18723", "-7.18723", &testDataUnion, JSON_NUMBER, NULL);
    testDataUnion.number = 3000;
    testParseAndStringify("Exponenet 3e3", "3e3", &testDataUnion, JSON_NUMBER, "3000");
    
    // TODO: unicode
    testCategory("STRING");
    char testString[1024];
    testDataUnion.string = testString;
    
    strcpy(testString, "Basic");
    testParseAndStringify("Basic String \"Basic\"", "\"Basic\"", &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "123");
    testParseAndStringify("Numeric String \"123\"", "\"123\"", &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "/*beeb-boob*/{}:[],_-_!?=<3");
    testParseAndStringify("Special characters \"/*beeb-boob*/{}:[],_-_!?=<3\"", "\"/*beeb-boob*/{}:[],_-_!?=<3\"" , &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "\"");
    testParseAndStringify("Escape Sequences \"Single\"", "\"\\\"\"", &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "a\\a");
    testParseAndStringify("Escape Sequences \"Double With junk\"", "\"a\\\\a\"", &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "\\");
    testParseAndStringify("Escape Sequences \"Double\"", "\"\\\\\"", &testDataUnion, JSON_STRING, NULL);
    strcpy(testString, "Michael,\nJonathan,\nMary,");
    testParseAndStringify("Escape Sequences \"Newline\"", "\"Michael,\nJonathan,\nMary,\"", &testDataUnion, JSON_STRING, "\"Michael,\\nJonathan,\\nMary,\"");

    testCategory("BOOLEAN");
    testDataUnion.boolean = true;
    testParseAndStringify("Boolean True", "true", &testDataUnion, JSON_BOOLEAN, NULL);
    testDataUnion.boolean = false;
    testParseAndStringify("Boolean False", "false", &testDataUnion, JSON_BOOLEAN, NULL);

    testCategory("NULL");
    testParseAndStringify("Null", "null", &testDataUnion, JSON_NULL, NULL);

    testCategory("ARRAY");
    testDataUnion.children = NULL;
    testParseAndStringify("Empty Arrary", "[]", &testDataUnion, JSON_ARRAY, NULL);
    testParseAndStringify("Filled Arrary", "[1,\"strobe\",[]]", NULL, JSON_ARRAY, NULL);

    testCategory("OBJECT");
    testDataUnion.children = NULL;
    testParseAndStringify("Empty Object", "{}", &testDataUnion, JSON_OBJECT, NULL);
    testParseAndStringify("Filled Object", "{\"number\":1,\"string\":\"strobe\",\"array\":[]}", NULL, JSON_OBJECT, NULL);

    testCategory("everyCaseInput");
    char *everyCaseInputString = fileAsString("test-data/everyCaseInput.jsonc");
    char *everyCaseoutputString = fileAsString("test-data/everyCaseOutput.jsonc");
    testParseAndStringify("EveryCaseInput", everyCaseInputString, NULL, JSON_OBJECT, everyCaseoutputString);

    jsonFree(testJson);

    //JSON *jsonParse(char *data);

    /*
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
    */

    return !allSuccess;
}

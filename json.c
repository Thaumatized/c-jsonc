#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

#include <stdio.h>

typedef struct EscapeSequence {
    char rawChar;
    char *sequence;
} EscapeSequence;

// Source: https://jsonviewertool.com/blog/how-to-escape-json-string
EscapeSequence EscapeSequences[] = {
    {'\"', "\\\""}, //Double quotation mark
    {'\\', "\\\\"}, //Backslash
    {'\n', "\\n"}, //Newline
    {'\r', "\\r"}, //Carriage Return
    {'\t', "\\t"}, //Horizontal Tab
    {'\b', "\\b"}, //Backspace
    {'\f', "\\f"}, //Formfeed Page Break
};

char *charToEscapecode(char c)
{
    int count = sizeof(EscapeSequences)/sizeof(EscapeSequences[0]);
    for (int i = 0; i < count; i++)
    {
        if (c == EscapeSequences[i].rawChar) {
            return EscapeSequences[i].sequence;
        }
    }
    return NULL;
}

// As a pointer so we can return NULL in case of invalid input
char *escapecodeToChar(char *str)
{
    int count = sizeof(EscapeSequences)/sizeof(EscapeSequences[0]);
    for (int i = 0; i < count; i++)
    {
        if (!strcmp(str, EscapeSequences[i].sequence)) {
            return &(EscapeSequences[i].rawChar);
        }
    }
    return NULL;
}

JSON *jsonParseRecursor(char *data, char* label) {

    JSON * newJson = malloc(sizeof(JSON));
    if(newJson == NULL)
    {
        printf("JSON PARSE ERROR: Failed to allocate memory");
        exit(1);
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
            //free(newJson);
            printf("JSON PARSE ERROR: data '%s' does not appear to be a valid json value", data);
            exit(1);
        }

        return newJson;
    }

    // string
    if(data[0] == '"')
    {
        int inputLength = 1;
        int escapeCharacters = 0;
        bool escaped = false;
        while ((escaped || data[inputLength] != '"') && data[inputLength] != '\0') // Null check *should* be useless. Better safe than sorry.
        {
            escaped = data[inputLength] == '\\' && !escaped;
            if(escaped) escapeCharacters++;
            inputLength++;
        }
        inputLength--; // get rid of closing quotation mark
        int stringLength = inputLength - escapeCharacters;

        *newJson = (JSON){
            .type = JSON_STRING,
            .label = label,
            .string = malloc(stringLength+1), // +1 NULL
            .previousSibling = NULL,
            .nextSibling = NULL,
        };

        if(newJson->string == NULL)
        {
            printf("JSON PARSE ERROR: Failed to allocate memory");                    
            //free(newJson);
            exit(1);
        }
        
        /* This would work if there was no escapes. Could be a potential optimization.
        strncpy(newJson->string, data+1, stringLength);
        newJson->string[stringLength] = '\0';
        */

        escapeCharacters = 0;
        for(int dataIndex = 0; dataIndex-escapeCharacters < stringLength; dataIndex++)
        {
            escaped = data[dataIndex+1] == '\\';
            if(escaped) {
                escapeCharacters++;
                dataIndex++;
                char escapeSequence[] = {data[dataIndex], data[dataIndex+1], '\0'};
                char *c = escapecodeToChar(escapeSequence);
                if(c == NULL)
                {
                    printf("JSON PARSE ERROR: Invalid escape character \\%c\n", data[dataIndex+1]);                    
                    //free(newJson);
                    exit(1);
                }
                newJson->string[dataIndex-escapeCharacters] = *c;
            }
            else
            {
                newJson->string[dataIndex-escapeCharacters] = data[dataIndex+1];
            }
        }
        newJson->string[stringLength] = '\0';

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
                    printf("JSON PARSE ERROR: JSON field inside of an object appears to be missing a label in %s", data); 
                    //goto errorCleanup; 
                    exit(1);  
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
                    printf("JSON PARSE ERROR: JSON field inside of an object appears to be missing a colon between label and data %s", data);
                    //goto errorCleanup;
                    exit(1);
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
                currentChild = jsonParseRecursor(childString, childLabel);
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
                printf("JSON PARSE ERROR: Two JSON fields inside an object/array appear to be missing a comma from between them in %s", data);
                exit(1);  
                //goto errorCleanup;
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

    printf("JSON PARSE ERROR: data '%s' does not appear to be a valid json value", data);
    exit(1);
}

JSON *jsonParse(char *data) {
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

    
    JSON *json = jsonParseRecursor(cleanedData, NULL);

    free(cleanedData);

    return json;
}

char *jsonStringify(JSON *data) {
    if(data == NULL)
    {
        char *retstring = strdup("null");
        if(retstring == NULL)
        {
            printf("JSON STRINGIFY ERROR: Failed to allocate memory");
            exit(1);
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
                childStrings[childIndex] = jsonStringify(childPointer);
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
                printf("JSON STRINGIFY ERROR: Failed to allocate memory");
                exit(1);
                /* 
                    for(int index = 0; index < childCount; index++)
                    {
                        free(childStrings[index]);
                    }
                */              
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
                printf("JSON STRINGIFY ERROR: Failed to allocate memory");
                exit(1);
            }
            snprintf(retstring, bufferSize, "%G", data->number);
            return retstring;
        }
        break;
    case JSON_STRING:
        {
            int stringLength = strlen(data->string);
            int escapeCharacters = 0;
            for(int i = 0; i < stringLength; i++)
            {
                char c = data->string[i];
                if(charToEscapecode(c) != NULL)
                {
                    escapeCharacters++;
                }
            }

            int bufferSize = stringLength+escapeCharacters+3; // +3 for quotes and null
            char *retstring = malloc(bufferSize);
            if(retstring == NULL)
            {
                printf("JSON STRINGIFY ERROR: Failed to allocate memory");
                exit(1);
            }

            /** This would work great, if it was not for escape cahracters
             * sprintf(retstring, "\"%s\"", data->string);
            */

            int handledEscapes = 0;
            for(int i = 0; i < stringLength; i++)
            {
                char c = data->string[i];
                char *EscapeSequence = charToEscapecode(data->string[i]);
                if(EscapeSequence != NULL) {
                    strcpy(&(retstring[i+handledEscapes+1]), EscapeSequence);
                    handledEscapes++;
                }
                else
                {
                    retstring[i+handledEscapes+1] = data->string[i];
                }
            }
            retstring[0] = '\"';
            retstring[bufferSize-2] = '\"';
            retstring[bufferSize-1] = '\0';

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
                printf("JSON STRINGIFY ERROR: Failed to allocate memory");
                exit(1);
            }
            return retstring;
        }
        break;
    case JSON_NULL:
        {
            char *retstring = strdup("null");
            if(retstring == NULL)
            {
                printf("JSON STRINGIFY ERROR: Failed to allocate memory");
                exit(1);
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
        {
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

JSON *jsonGetKey(JSON *json, const char *key)
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
                printf("JSON GET WARNING: JSON element \"%s\" is not of type ARRAY, but was attempted to index into with index %s", currentJson->label, keyBuffer);
                return NULL;
            }

            JSON *child = currentJson->children;
            for(int key = atoi(keyBuffer); key > 0 && child != NULL; key--)
            {
                child = child->nextSibling;
            }

            if(child == NULL)
            {
                printf("JSON GET WARNING: index %s is out of bounds in json elment %s", keyBuffer, currentJson->label);
                return NULL;
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
                printf("JSON GET WARNING: key %s no found in %s", keyBuffer, currentJson->label);
                return NULL;
            }

            currentJson = child;
        }
        keyIndex += keyLength;
    }

    return currentJson;
}

JSON *jsonGetObject(JSON *json, const char *key)
{
    JSON* objectJson = jsonGetKey(json, key);
    if(objectJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (objectJson->type != JSON_OBJECT)
    {
        printf("JSON GET ERROR: json with key %s is not an object", key);
        exit(1);
    }
    else
    {
        return objectJson;
    }
}

JSON *jsonGetArray(JSON *json, const char *key)
{
    JSON* arrayJson = jsonGetKey(json, key);
    if(arrayJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (arrayJson->type != JSON_ARRAY)
    {
        printf("JSON GET ERROR: json with key %s is not an array", key);
        exit(1);
    }
    else
    {
        return arrayJson;
    }
}

char *jsonGetString(JSON *json, const char *key)
{
    JSON* stringJson = jsonGetKey(json, key);
    if(stringJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (stringJson->type != JSON_STRING)
    {
        printf("JSON GET ERROR: json with key %s is not a string", key);
        exit(1);
    }
    else
    {
        return stringJson->string;
    }
}

double *jsonGetNumber(JSON *json, const char *key)
{
    JSON* numberJson = jsonGetKey(json, key);
    if(numberJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (numberJson->type != JSON_NUMBER)
    {
        printf("JSON GET ERROR: json with key %s is not a number", key);
        exit(1);
    }
    else
    {
        return &(numberJson->number);
    }
}

bool *jsonGetBool(JSON *json, const char *key)
{
    JSON* boolJson = jsonGetKey(json, key);
    if(boolJson == NULL) {
        // Error handled by jsonGetKey
        return NULL;
    }
    else if (boolJson->type != JSON_BOOLEAN)
    {
        printf("JSON GET ERROR: json with key %s is not a boolean", key);
        exit(1);
    }
    else
    {
        return &(boolJson->boolean);
    }
}

void jsonSetKey(JSON *parentJson, JSON *newChildJson, const char *key) {
    // First we check if the keys already exists, replacing them.
    int keyIndex = 0;
    int keyLength = 0;
    JSON *currentJson = parentJson;

    while (key[keyIndex] != '\0')
    {
        bool isNumberKey = key[keyIndex] == '[';
        if(isNumberKey) keyIndex++; // skip opening bracket
        keyLength = 0;
        while(!isKeySeparator(key[keyIndex+keyLength]))
        {
            keyLength++;
        }

        char keyBuffer[keyLength+1];
        strncpy(keyBuffer, key+keyIndex, keyLength);
        keyBuffer[keyLength] = '\0';

        keyIndex += keyLength;
        //skip the closing bracket
        if(isNumberKey) 
        {
            keyIndex++;
        }
        //skip the dot
        if(key[keyIndex] == '.')
        {
            keyIndex ++;
        }

        bool lastKey = key[keyIndex] == '\0';

        JSON_TYPES pathBuildingType = (!lastKey && key[keyIndex] == '[') ? JSON_ARRAY : JSON_OBJECT;

        if(lastKey && !isNumberKey)
        {
            newChildJson->label = strdup(keyBuffer);
        }

        if(isNumberKey)
        {
            if(currentJson->type != JSON_ARRAY)
            {
                printf("JSON SET WARNING: JSON element \"%s\" is not of type ARRAY, but was attempted to index into with index %s", currentJson->label, keyBuffer);
                return;
            }

            bool nullsInsterted = false;
            if(currentJson->children == NULL)
            {
                nullsInsterted = true;
                JSON *newJson = malloc(sizeof(JSON));
                if(newJson == NULL)
                {
                    printf("JSON PARSE ERROR: Failed to allocate memory");
                    exit(1);
                }

                *newJson = (JSON){
                    .type = JSON_NULL,
                    .label = NULL,
                    .string = NULL,
                    .previousSibling = NULL,
                    .nextSibling = NULL,
                };

                currentJson->children = newJson;
            }
            JSON *child = currentJson->children;
            int key = atoi(keyBuffer);
            for(int loopKey = 0; loopKey < key; loopKey++)
            {
                if(child->nextSibling == NULL)
                {
                    nullsInsterted = true;
                    JSON *newJson = malloc(sizeof(JSON));
                    if(newJson == NULL)
                    {
                        printf("JSON SET KEY ERROR: Failed to allocate memory");
                        exit(1);
                    }

                    *newJson = (JSON){
                        .type = JSON_NULL,
                        .label = NULL,
                        .string = NULL,
                        .previousSibling = child,
                        .nextSibling = NULL,
                    };

                    child->nextSibling = newJson;
                }
                child = child->nextSibling;
            }

            if(!lastKey)
            {
                if(!nullsInsterted)
                {
                    currentJson = child;
                }
                else
                {
                    JSON *newJson = malloc(sizeof(JSON));
                    if(newJson == NULL)
                    {
                        printf("JSON SET KEY ERROR: Failed to allocate memory");
                        exit(1);
                    }

                    *newJson = (JSON){
                        .type = pathBuildingType,
                        .label = NULL,
                        .children = NULL,
                        .previousSibling = child->previousSibling,
                        .nextSibling = NULL,
                    };

                    child->nextSibling = newJson;
                    currentJson = newJson;
                }
            }
            else
            {
                if(child->previousSibling)
                {
                    child->previousSibling->nextSibling = newChildJson;
                    newChildJson->previousSibling = child->previousSibling;
                    newChildJson->nextSibling = child->nextSibling;
                    jsonFree(child);
                    return;
                }
                else
                {
                    currentJson->children = newChildJson;
                    newChildJson->nextSibling = child->nextSibling;
                    jsonFree(child);
                    return;
                }
            }
        }
        else
        {
            JSON *lastChild = NULL;
            JSON *child = currentJson->children;
            while(child != NULL && strcmp(child->label, keyBuffer) != 0)
            {
                lastChild = child;
                child = child->nextSibling;
            }

            if(lastKey)
            {
                if(child == NULL)
                {
                    if(lastChild != NULL)
                    {
                        lastChild->nextSibling = newChildJson;
                        newChildJson->previousSibling = lastChild;
                    }
                    else
                    {
                        currentJson->children = newChildJson;
                    }
                }
                else
                {
                    newChildJson->previousSibling = child->previousSibling;
                    newChildJson->nextSibling = child->nextSibling;
                    child->previousSibling->nextSibling = newChildJson;
                    jsonFree(child);
                }
            }
            else
            {
                if(child != NULL)
                {
                    currentJson = child;
                }
                else
                {   
                    JSON *newJson = malloc(sizeof(JSON));
                    if(newJson == NULL)
                    {
                        printf("JSON SET KEY ERROR: Failed to allocate memory");
                        exit(1);
                    }

                    *newJson = (JSON){
                        .type = pathBuildingType,
                        .label = strdup(keyBuffer),
                        .children = NULL,
                        .previousSibling = NULL,
                        .nextSibling = NULL,
                    };

                    if(lastChild != NULL)
                    {
                        lastChild->nextSibling = newJson;
                        newJson->previousSibling = lastChild;
                    }
                    else
                    {
                        currentJson->children = newJson;
                    }
                    currentJson = newJson;
                }
            }
        }
    }
}

void jsonSetObject(JSON *parentJson, JSON *newChildJson, const char *key){
    if(newChildJson->type != JSON_OBJECT)
    {
        printf("JSON SET ERROR: Object is not an object");
        exit(1);
    }
    jsonSetKey(parentJson, newChildJson, key);
}

void jsonSetArray(JSON *parentJson, JSON *newChildJson, const char *key){
    if(newChildJson->type != JSON_ARRAY)
    {
        printf("JSON SET ERROR: Array is not an array");
        exit(1);
    }
    jsonSetKey(parentJson, newChildJson, key);
}

void jsonSetString(JSON *parentJson, const char *string, const char *key){
    JSON *newJson = malloc(sizeof(JSON));
    char *newString = strdup(string);
    if(newJson == NULL || newString == NULL)
    {
        printf("JSON SET ERROR: Failed to allocate memory");
        exit(1);
    }

    // boolean - true
    *newJson = (JSON){
        .type = JSON_STRING,
        .label = NULL,
        .boolean = newString,
        .previousSibling = NULL,
        .nextSibling = NULL,
    };

    jsonSetKey(parentJson, newJson, key);
}

void jsonSetNumber(JSON *parentJson, const double number, const char *key){
    JSON *newJson = malloc(sizeof(JSON));
    if(newJson == NULL)
    {
        printf("JSON SET ERROR: Failed to allocate memory");
        exit(1);
    }

    // boolean - true
    *newJson = (JSON){
        .type = JSON_NUMBER,
        .label = NULL,
        .number = number,
        .previousSibling = NULL,
        .nextSibling = NULL,
    };

    jsonSetKey(parentJson, newJson, key);
}

void jsonSetBool(JSON *parentJson, const bool boolean, const char *key){
    JSON *newJson = malloc(sizeof(JSON));
    if(newJson == NULL)
    {
        printf("JSON SET ERROR: Failed to allocate memory");
        exit(1);
    }

    // boolean - true
    *newJson = (JSON){
        .type = JSON_BOOLEAN,
        .label = NULL,
        .boolean = boolean,
        .previousSibling = NULL,
        .nextSibling = NULL,
    };

    jsonSetKey(parentJson, newJson, key);
}
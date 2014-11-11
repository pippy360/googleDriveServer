#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* strstrn(char* const haystack, const char* needle, const int haystackSize){
    int i;
    for (i = 0; i < (haystackSize - (strlen(needle) - 1)); i++){
        int j;
        for (j = 0; j < strlen(needle); j++)
            if(needle[j] != haystack[i+j])
                break;        
        if (j == strlen(needle))
            return haystack + i;
    }
    return NULL;
}
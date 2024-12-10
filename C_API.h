#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>

// 오픈 소스
#include "cJSON.h"

#define SIZE 1024 * 512


struct response_buffer {
    char *data;
    size_t size;
};

size_t write_callback(char *content, size_t size, size_t nmemb, char *userp);
char* chat(char *msg, const char *model);
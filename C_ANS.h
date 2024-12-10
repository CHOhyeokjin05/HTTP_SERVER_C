#include "C_API.h"
#include "C_DB.h"

#define MSG_SIZE 1024 * 32

void remove_quotes(char *str);
char* create_message(cJSON *root, char *req, char *lang, char *prob);
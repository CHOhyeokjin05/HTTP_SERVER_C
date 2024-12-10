#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SQL_SIZE 1024

sqlite3* open_db(const char *db_name);
int execute_query(sqlite3 *db, const char *sql, sqlite3_stmt **stmt);
void close_db(sqlite3 *db, sqlite3_stmt *stmt);

void insert_DB(char *req, char *prob, char *lang, char *result);
char* check(char *req, char *prob, char *lang);
int OQ_check(char *name);
void OQ_update(char *name);
char* MainText(int prob);
char* MainImage(int prob);
void delete_DB(const char *req, char *prob, char *lang);
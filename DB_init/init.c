// sqlite 라이브러리
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#define SQL_SIZE 1024

// 서버 프로그램과 별개라서 main 함수로 작성됨
// crontab에 UTC 15:00 (자정) init 실행파일을 실행하도록 코딩됨
int main() {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    // 토큰 10개로 초기화
    char sql_update[SQL_SIZE] = "UPDATE Regist SET Count = 10;";

    // 데이터베이스 열기
    int rc = sqlite3_open("BAA.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    // UPDATE 쿼리 준비
    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare UPDATE statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
		exit(1);
	}


    // UPDATE 쿼리 실행
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute UPDATE statement: %s\n", sqlite3_errmsg(db));
    }

    // 자원 해제
    sqlite3_finalize(stmt);
    sqlite3_close(db); // 데이터베이스 닫기
}


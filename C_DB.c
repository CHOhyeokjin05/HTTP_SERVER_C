// 데이터베이스 헤더 파일
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 명령어 문자열 크기
#define SQL_SIZE 1024

// sqlite 명령어 문자열
char sql[SQL_SIZE];

// 데이터베이스 연결 함수
// 데이터베이스 파일을 열고 연결을 반환
sqlite3* open_db(const char *db_name) {
    sqlite3 *db;
    int rc = sqlite3_open(db_name, &db);  // 데이터베이스 연결 시도
    if (rc != SQLITE_OK) {
        // 연결 실패 시 에러 메시지 출력
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);  // 열린 데이터베이스 닫기
        exit(1);  // 프로그램 종료
    }
    return db;  // 성공적으로 연결되면 db 반환
}

// 쿼리 준비 및 실행 함수
// SQL 쿼리를 준비하고 실행하는 함수
int execute_query(sqlite3 *db, const char *sql, sqlite3_stmt **stmt) {
    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, 0);  // SQL 문 준비
    if (rc != SQLITE_OK) {
        // 준비 실패 시 에러 메시지 출력
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;  // 에러 코드 반환
    }
    return SQLITE_OK;  // 준비 성공
}

// 자원 해제 함수
// 준비된 SQL 문과 데이터베이스 연결 닫기
void close_db(sqlite3 *db, sqlite3_stmt *stmt) {
    sqlite3_finalize(stmt);  // 준비된 SQL 문 종료 및 메모리 해제
    sqlite3_close(db);  // 데이터베이스 연결 종료
}

// 데이터 삽입 함수
// 개념 설명, 정답 보기에 해당하는 데이터 저장
void insert_DB(char *req, char *prob, char *lang, char *result) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;

    // SQL 삽입문 생성 (테이블 이름은 req에 의해 동적으로 결정됨)
    snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO %s VALUES (?, ?, ?);", req);

    // 쿼리 준비 및 실행
    if (execute_query(db, sql, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return;
    }

    // 매개변수 바인딩 (문제 번호, 언어, 결과 값 바인딩)
    sqlite3_bind_int(stmt, 1, atoi(prob));  // 문제 번호
    sqlite3_bind_text(stmt, 2, lang, -1, SQLITE_TRANSIENT);  // 언어
    sqlite3_bind_text(stmt, 3, result, -1, SQLITE_TRANSIENT);  // 결과

    // 쿼리 실행
    int rc = sqlite3_step(stmt);  // SQL 문 실행
    if (rc != SQLITE_DONE) {
        // 실행 실패 시 에러 메시지 출력
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    close_db(db, stmt);  // 자원 해제
}

// 데이터 조회 함수
// 주어진 테이블에서 특정 조건에 맞는 데이터를 조회하여 반환
char* check(char *req, char *prob, char *lang) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;
    char *chat_result = NULL;

    // SQL 조회문 생성
    snprintf(sql, sizeof(sql), "SELECT Chat FROM %s WHERE Id = ? AND Lang = ?;", req);

    // 쿼리 준비 및 실행
    if (execute_query(db, sql, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return NULL;
    }

    // 매개변수 바인딩 (문제 번호, 언어)
    sqlite3_bind_int(stmt, 1, atoi(prob));  // 문제 번호
    sqlite3_bind_text(stmt, 2, lang, -1, SQLITE_TRANSIENT);  // 언어

    int rc = sqlite3_step(stmt);  // 쿼리 실행
    if (rc == SQLITE_ROW) {
        // 쿼리 결과가 존재하면 Chat 컬럼 값을 chat_result에 복사
        const unsigned char *chat = sqlite3_column_text(stmt, 0);
        if (chat) {
            // char 포인터 반환을 위한 작업
            chat_result = malloc(strlen((const char *)chat) + 1);  // 메모리 할당
            strcpy(chat_result, (const char *)chat);  // 결과 복사
        }
    }
    else if (rc != SQLITE_DONE) {
        // 실행 실패 시 에러 메시지 출력
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    close_db(db, stmt);  // 자원 해제
    return chat_result;  // 결과 반환 (호출자에서 메모리 해제 필요)
}

// 사용자 문제 카운트 확인 함수
// 이 함수는 'Regist' 테이블에서 사용자의 문제 카운트를 조회하고 반환
int OQ_check(char *name) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;
    int count = -1;  // 기본 값으로 -1 설정
    char sql_select[SQL_SIZE] = "SELECT Count FROM Regist WHERE Name = ?;";
    char sql_insert[SQL_SIZE] = "INSERT INTO Regist (Name, Count) VALUES (?, 10);";

    // SQL SELECT 쿼리 실행
    if (execute_query(db, sql_select, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);  // 사용자 이름 바인딩

    int rc = sqlite3_step(stmt);  // 쿼리 실행
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);  // 조회된 카운트 값 반환
    }
    else {
        // 사용자가 없으면 새로 INSERT
        sqlite3_finalize(stmt);  // 기존 stmt 해제

        if (execute_query(db, sql_insert, &stmt) != SQLITE_OK) {
            close_db(db, stmt);  // 오류 발생 시 자원 해제
            return -1;
        }

        sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);  // 사용자 이름 바인딩
        rc = sqlite3_step(stmt);  // INSERT 실행
        if (rc != SQLITE_DONE) {
            // 실행 실패 시 에러 메시지 출력
            fprintf(stderr, "Failed to execute INSERT statement: %s\n", sqlite3_errmsg(db));
        }
    }

    close_db(db, stmt);  // 자원 해제
    return count;  // 카운트 반환
}

// 사용자 문제 카운트 감소 함수
// 이 함수는 사용자의 문제 카운트를 하나 감소시킴
void OQ_update(char *name) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;
    char sql_update[SQL_SIZE] = "UPDATE Regist SET Count = Count - 1 WHERE Name = ?;";

    // SQL UPDATE 쿼리 실행
    if (execute_query(db, sql_update, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);  // 사용자 이름 바인딩

    int rc = sqlite3_step(stmt);  // 쿼리 실행
    if (rc != SQLITE_DONE) {
        // 실행 실패 시 에러 메시지 출력
        fprintf(stderr, "Failed to update user count: %s\n", sqlite3_errmsg(db));
    }

    close_db(db, stmt);  // 자원 해제
}
// 백준 문제 텍스트를 데이터베이스에서 꺼내오는 함수
char* MainText(int prob) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;
    char *result = NULL;

    // SQL SELECT 문 생성
    snprintf(sql, sizeof(sql), "SELECT Text FROM Problem WHERE Id = ?;");

    // 쿼리 준비 및 실행
    if (execute_query(db, sql, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return NULL;
    }

    // 매개변수 바인딩 (Id에 prob 값 바인딩)
    sqlite3_bind_int(stmt, 1, prob);

    // 쿼리 실행
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 결과 값 가져오기 (Text 값 읽기)
        const unsigned char *text = sqlite3_column_text(stmt, 0);

        if (text) {
            // 결과 문자열을 동적으로 할당하여 복사
            result = strdup((const char *)text);
        }
    } else {
        fprintf(stderr, "No data found for Id = %d\n", prob);
    }

    close_db(db, stmt);  // 자원 해제
    return result;  // 결과 문자열 반환
}
// 백준 문제 이미지 주소를 데이터베이스에서 꺼내오는 함수
char* MainImage(int prob) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;
    char *result = NULL;

    // SQL SELECT 문 생성
    snprintf(sql, sizeof(sql), "SELECT Img FROM Problem WHERE Id = ?;");

    // 쿼리 준비 및 실행
    if (execute_query(db, sql, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return NULL;
    }

    // 매개변수 바인딩 (Id에 prob 값 바인딩)
    sqlite3_bind_int(stmt, 1, prob);

    // 쿼리 실행
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 결과 값 가져오기 (Text 값 읽기)
        const unsigned char *text = sqlite3_column_text(stmt, 0);

        if (text) {
            // 결과 문자열을 동적으로 할당하여 복사
            result = strdup((const char *)text);
        }
    } else {
        fprintf(stderr, "No data found for Id = %d\n", prob);
    }

    close_db(db, stmt);  // 자원 해제
    return result;  // 결과 문자열 반환
}

// 데이터 삽입 함수
// 개념 설명, 정답 보기에 해당하는 데이터 저장
void delete_DB(const char *req, char *prob, char *lang) {
    sqlite3 *db = open_db("BAA.db");  // 데이터베이스 열기
    sqlite3_stmt *stmt;

    // SQL 삽입문 생성 (테이블 이름은 req에 의해 동적으로 결정됨)
    snprintf(sql, sizeof(sql), "DELETE FROM %s WHERE Id = ? AND Lang = ?;", req);

    // 쿼리 준비 및 실행
    if (execute_query(db, sql, &stmt) != SQLITE_OK) {
        close_db(db, stmt);  // 오류 발생 시 자원 해제
        return;
    }

    // 매개변수 바인딩 (문제 번호, 언어, 결과 값 바인딩)
    sqlite3_bind_int(stmt, 1, atoi(prob));  // 문제 번호
    sqlite3_bind_text(stmt, 2, lang, -1, SQLITE_TRANSIENT);  // 언어

    // 쿼리 실행
    int rc = sqlite3_step(stmt);  // SQL 문 실행
    if (rc != SQLITE_DONE) {
        // 실행 실패 시 에러 메시지 출력
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    close_db(db, stmt);  // 자원 해제
}

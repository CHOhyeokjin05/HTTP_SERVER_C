#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 리눅스용 파일, 프로세스, 메모리, 입출력 등 시스템 자원 제어 관련 헤더 파일
#include <unistd.h>

// 네트워크 통신 관련 헤더 파일
#include <arpa/inet.h>

// 소켓 관련 헤더 파일
#include <sys/socket.h>

// JSON 파일을 다루기 위한 오픈 소스 헤더 파일
#include "cJSON.h"

// ChatGPT로부터 얻은 응답을 처리하는 코드의 헤더 파일
#include "C_ANS.h"

// 버퍼 사이즈
#define BUF_SIZE 1024 * 64

// 추가 질문 시 유저 이름과 응답을 보낼 배열 사이즈
#define OQ_SIZE 256

// 자릿수
#define DIGITS 10

void remove_control_chars(char *str) {
    char *src = str;
    char *dst = str;
    
    while (*src) {
        if (*src != '\t' && *src != '\n' && *src != '\r') {
            *dst++ = *src;  // 유효한 문자는 그대로 복사
        }
        src++;
    }
    *dst = '\0';  // 문자열 끝에 널 종료 문자 추가
}

// JSON 파일에서 문자열을 파싱하면 이스케이프 문자 존재
// 이 함수를 통해서 이스케이프 문자 제거
void remove_escape(char *origin) {
    char *remove = origin;
    char *original = origin;

    while (*original) {
        if (*original != '\\' && *original != '"') {
            *remove++ = *original;
        }
        else {
            original++;
        }
        original++;
    }
    *remove = '\0';
}

// 서버 실행 시 터미널에 ./serv.out 0.0.0.0 9190 이런 명령어로 실행함
// argv[0]에 프로그램 경로, argv[1]에 0.0.0.0 이 들어가고 argv[2]에 9190이 들어감
int main(int argc, char *argv[]) {

    // 서버, 클라이언트 소켓 정보 저장 변수
    int serv_sock;
    int clnt_sock;

    // 서버 주소를 담는 구조체
    struct sockaddr_in serv_addr;
    // 클라이언트 주소를 담는 구조체
    struct sockaddr_in clnt_addr;
    // 클라이언트 주소의 크기 저장하는 변수
    socklen_t clnt_addr_size;

    // 클라이언트가 주는 메시지를 저장하기 위한 버퍼
    // 계속 사용해야 하기 때문에 동적 할당을 사용하지 않고
    // 배열을 선언하여 사용
    char req_message[BUF_SIZE];

    // 메시지 길이
    int str_len;


    // 명령어 인자가 3개가 아니면 오류 메시지 출력
    if (argc != 3) {
        printf("Usage : %s <IP> <PORT>\n", argv[0]);  // 사용법 안내
        return EXIT_FAILURE;
    }

    // 소켓을 생성한다. PF_INET은 IPv4 프로토콜, SOCK_STREAM은 TCP 연결을 의미
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock < 0) {
        printf("socket() failed");  // 소켓 생성 실패 시 오류 메시지 출력
        return EXIT_FAILURE;
    }

    // 서버 주소 구조체에 프로토콜, IP 주소, 포트 번호 저장
    memset(&serv_addr, 0, sizeof(serv_addr));  // 주소 구조체 초기화
    serv_addr.sin_family = AF_INET;  // IPv4를 의미함 (ex. 192.128.0.1)
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);  // IP 주소
    serv_addr.sin_port = htons(atoi(argv[2]));  // 포트 번호, atoi는 문자열인 숫자를 정수형으로 바꿈

    // 서버 소켓에 주소를 바인딩
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("bind() failed");  // 바인딩 실패 시 오류 메시지 출력
        return EXIT_FAILURE;
    }

    // 클라이언트의 연결을 기다리도록 설정
    if (listen(serv_sock, 5) < 0) {
        printf("listen() failed");  // 리슨 실패 시 오류 메시지 출력
        return EXIT_FAILURE;
    }

    // 클라이언트의 주소 크기 정보 설정
    clnt_addr_size = sizeof(clnt_addr);

    while (1) {
        // 클라이언트가 요청을 보내야 밑에 있는 코드가 다 실행됨
        // 클라이언트 요청을 수락하는 부분
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock < 0) {
            printf("accept() failed");
            return EXIT_FAILURE;
        }

        // read는 데이터를 읽고 읽은 바이트 수를 반환함
        // 하지만 마지막 널문자는 자동으로 추가되지 않음
        if ((str_len = read(clnt_sock, req_message, BUF_SIZE)) != 0) {
            req_message[str_len] = '\0';
            printf("this is request_message\n\n");
            printf("%s\n", req_message);

            // 확장 프로그램이 만든 버튼을 클릭하면 서버에게 요청을 보내는데
            // OPTIONS 요청을 먼저 한 후 POST 요청을 하는 구조임을 경험적으로 파악
            // 따라서 OPTIONS 요청 먼저 처리 후 POST 요청 처리

            // 요청이 OPTIONS인지 확인
            if (strstr(req_message, "OPTIONS") != NULL) {
                const char *response_message = 
                    "HTTP/1.1 204 No Content\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
                // 응답 전달
                write(clnt_sock, response_message, strlen(response_message));
                close(clnt_sock);
                continue;    // 다시 대기
            }



            // 문제 페이지로 들어갈 때 translation API key 요청이 오면
            if (strstr(req_message, "APIKEY") != NULL) {

    
                // 응답 JSON 객체 생성
                cJSON *res_json = cJSON_CreateObject();
                cJSON_AddStringToObject(res_json, "key", "구글 translation 키");

                // 응답을 문자열로 변환
                char *json_response = cJSON_Print(res_json);
                int body_length = strlen(json_response);

                // HTTP 응답 메시지 생성 (CORS 헤더 제외)
                char *res_message = (char*)malloc(sizeof(char) * BUF_SIZE);
                snprintf(res_message, BUF_SIZE,
                        "HTTP/1.1 200 OK\r\n"
                        "Server: simple web server\r\n"
                        "Content-Length: %d\r\n"
                        "Content-Type: application/json\r\n"
                        "\r\n"
                        "%s", body_length, json_response);

                // 응답 전송
                write(clnt_sock, res_message, strlen(res_message));

                // 메모리 해제
                free(json_response);
                cJSON_Delete(res_json);
                free(res_message);

                // 소켓 닫기
                close(clnt_sock);
                continue;
            }



            // GET 요청 처리
            if (strstr(req_message, "GET ") != NULL) {
                char *start = strstr(req_message, "GET ") + 5; // "GET /" 다음 위치
                char *end = strchr(start, ' '); // 다음 '/' 찾기

                // 문제 번호를 저장할 버퍼
                char problem_number_str[DIGITS] = {0};
                size_t length = end - start;

                // 문제 번호 문자열 복사
                strncpy(problem_number_str, start, length);
                problem_number_str[length] = '\0'; // 마지막 널문자

                // 문자열을 정수로 변환
                int problem_number = atoi(problem_number_str);
                if (problem_number > 0) {
                    // MainText 호출 및 결과 문자열 받기
                    char *main_text_result = MainText(problem_number);
                    char *main_image_result = MainImage(problem_number);

                    cJSON *response_json = cJSON_CreateObject();
                    cJSON_AddStringToObject(response_json, "output", main_text_result);
                    cJSON_AddStringToObject(response_json, "image", main_image_result);
                    char *json_response = cJSON_Print(response_json);

                    // 응답 메시지 작성
                    char *response_message = (char*)malloc(sizeof(char) * BUF_SIZE);
                    snprintf(response_message, BUF_SIZE,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n"
                            "%s",
                            (int)strlen(json_response), json_response);

                    // 클라이언트에 응답 보내기
                    write(clnt_sock, response_message, strlen(response_message));

                    free(main_text_result);
                    free(json_response);
                    free(response_message);
                    cJSON_Delete(response_json);
                }
                else {
                    printf("유효하지 않은 문제 번호: %s\n", problem_number_str);
                }
                close(clnt_sock);
                continue;
            }

            // 추가 질문 시 유저 이름을 미리 서버에 전송해주기에
            // 처리가 필요
            if (strstr(req_message, "userName") != NULL) {
                char OQ_req[OQ_SIZE];
                // username 키의 시작 주소
                char *tmp = strstr(req_message, "userName");
                char OQ[OQ_SIZE];
                int cnt = 0;
                if (tmp != NULL) tmp += 11;     // 아이디 시작 부분이 11칸 뒤
                // 유저 이름 추출
                while(*tmp != '"') OQ[cnt++] = *tmp++;
                // 마지막 널문자
                OQ[cnt] = '\0';

                // CORS는 NGINX가 담당
                snprintf(OQ_req, sizeof(OQ_req), 
                     "HTTP/1.1 200 OK\r\n"
                     "Server: simple web server\r\n"
                     "Content-Length: %d\r\n"
                     "Content-Type: application/json\r\n"
                     "\r\n"
                     "%d", 1, OQ_check(OQ));
                // 응답 전송
                write(clnt_sock, OQ_req, strlen(OQ_req));
                close(clnt_sock);
                continue;
            }
            // 문제 페이지로 들어갈 때 "saveproblem" 요청이 와서
            // saveproblem이 시작하는 주소가 있을 때 수행
            if (strstr(req_message, "saveproblem") != NULL) {
                // json 데이터를 추출
                char *save_prob = strstr(req_message, "\r\n\r\n");
                if (save_prob != NULL) {
                    save_prob += 4;  // 헤더 부분 스킵
                }
                // json 데이터 파싱
                cJSON *save = cJSON_Parse(save_prob);
                if (save == NULL) {
                    printf("Error: No JSON data found in request\n");
                    close(clnt_sock);
                    continue;
                }
                // json 데이터 키를 통해서 데이터 접근
                cJSON *prob = cJSON_GetObjectItem(save, "problem");
                cJSON *text = cJSON_GetObjectItem(save, "savedtext");
                cJSON *image = cJSON_GetObjectItem(save, "problemIMG");
                // 데이터를 잘 가져왔는지 확인
                if (prob == NULL || !cJSON_IsString(prob) || 
                    text == NULL || !cJSON_IsString(text) ||
                    image == NULL || !cJSON_IsString(image)) {
                    printf("Error: Missing or invalid fields in JSON\n");
                    close(clnt_sock);
                    continue;
                }
                // 제어 문자를 제거
                remove_control_chars(text->valuestring);
                remove_control_chars(image->valuestring);

                // 문제를 데이터베이스에 저장
                insert_DB("Problem", prob->valuestring, text->valuestring, image->valuestring);

                // 응답 JSON 객체 생성
                cJSON *res_json = cJSON_CreateObject();
                cJSON_AddStringToObject(res_json, "status", "success");
                cJSON_AddStringToObject(res_json, "message", "문제가 성공적으로 저장되었습니다.");

                // 응답을 문자열로 변환
                char *json_response = cJSON_Print(res_json);
                int body_length = strlen(json_response);

                // HTTP 응답 메시지 생성 (CORS 헤더 제외)
                char *res_message = (char*)malloc(sizeof(char) * BUF_SIZE);
                snprintf(res_message, BUF_SIZE,
                        "HTTP/1.1 200 OK\r\n"
                        "Server: simple web server\r\n"
                        "Content-Length: %d\r\n"
                        "Content-Type: application/json\r\n"
                        "\r\n"
                        "%s", body_length, json_response);

                // 응답 전송
                write(clnt_sock, res_message, strlen(res_message));

                // 메모리 해제
                free(json_response);
                cJSON_Delete(res_json);
                cJSON_Delete(save);
                free(res_message);

                close(clnt_sock);
                continue;
            }

            // json 데이터 분리
            char *json_data = strstr(req_message, "\r\n\r\n");
            if (json_data != NULL) {
                json_data += 4;  // 헤더 부분 스킵
                printf("\nthis is json part:\n%s\n\n", json_data);
            } else {
                printf("Error: No JSON data found in request\n");
                close(clnt_sock);
                continue;
            }
            // json 데이터 파싱
            cJSON *root = cJSON_Parse(json_data);
            if (root == NULL) {
                printf("Error: Failed to parse JSON\n");
                close(clnt_sock);
                continue;
            }
            // 키를 통해서 json 데이터 접근
            cJSON *req_type = cJSON_GetObjectItem(root, "req_type");
            cJSON *language = cJSON_GetObjectItem(root, "language");
            cJSON *problem = cJSON_GetObjectItem(root, "problem");

            // 데이터를 잘 가져왔는지 확인
            if (req_type == NULL || !cJSON_IsString(req_type) || 
                language == NULL || !cJSON_IsString(language) ||
                problem == NULL || !cJSON_IsString(problem)) {
                printf("Error: Missing or invalid fields in JSON\n");
                close(clnt_sock);
                continue;
            }

            // remove_escape 함수를 통해서 이스케이프 문자 제거
            remove_escape(req_type->valuestring);
            remove_escape(language->valuestring);
            remove_escape(problem->valuestring);
            // ChatGPT API로부터 응답 받아오기
            char *result = create_message(root, req_type->valuestring, language->valuestring, problem->valuestring);

            // json 형식을 위해서 cJSON 라이브러리 활용
            // cJSON 구조체를 할당해주는 함수
            cJSON *res_json = cJSON_CreateObject();
            // "output" 키와 내용을 json 파일에 넣기
            cJSON_AddStringToObject(res_json, "output", result);
            free(result);

            // 문자열로 변환
            char *json_origin = cJSON_Print(res_json);
            int body_length = strlen(json_origin);

            // 응답 메시지 생성 (CORS 헤더는 NGINX에서 추가됨)
            char *res_message = (char*)malloc(sizeof(char) * BUF_SIZE);
            snprintf(res_message, BUF_SIZE,
                     "HTTP/1.1 200 OK\r\n"
                     "Server: simple web server\r\n"
                     "Content-Length: %d\r\n"
                     "Content-Type: application/json\r\n"
                     "\r\n"
                     "%s", body_length, json_origin);

            // 로그를 위한 printf
            printf("this is response_message\n\n");
            printf("%s\n", res_message);

            // 응답 전송
            write(clnt_sock, res_message, strlen(res_message));

            // 메모리 할당 해제
            cJSON_Delete(root);
            cJSON_Delete(res_json);
            cJSON_free(json_origin);
            free(res_message);
        }

        close(clnt_sock);
    }

    close(serv_sock);
    return EXIT_SUCCESS;
}
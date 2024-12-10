#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// API에 요청 보내고 받기 위한 헤더 파일
#include <curl/curl.h>

// 오픈 소스
#include "cJSON.h"

// ChatGPT에게 전달할 대답 배열 사이즈
#define SIZE 1024 * 64

// 응답을 누적할 구조체 정의
struct response_buffer {
    char *data;
    size_t size;
};

// Curl 응답 콜백 함수
size_t write_callback(void *content, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct response_buffer *resp_buf = (struct response_buffer *)userp;

    // 버퍼 크기 재할당 (현재 크기 + 새로운 데이터 크기)
    char *ptr = realloc(resp_buf->data, resp_buf->size + total_size + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 0;
    }

    // 새로운 데이터를 기존 버퍼에 추가
    resp_buf->data = ptr;
    memcpy(&(resp_buf->data[resp_buf->size]), content, total_size);
    resp_buf->size += total_size;
    resp_buf->data[resp_buf->size] = '\0';  // 마지막은 널문자

    return total_size;
}

// ChatGPT API로부터 요청을 보내고 응답을 받는 함수
char* chat(char *msg, const char *model) {
    CURL *curl;
    CURLcode res;

    // Curl 초기화
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Unable to initialize curl.\n");
        exit(1);
    }

    // Curl 설정: URL 설정
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");

    // Curl 설정: 헤더 설정
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Authorization: Bearer GPT 키");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Curl 설정: POST 요청 설정
    curl_easy_setopt(curl, CURLOPT_POST, 1);


    // Curl 설정: Json 페이로드 설정
    char *json_payload = (char*)malloc(sizeof(char) * SIZE);
    snprintf(json_payload, SIZE, "{\n"
                         "  \"model\": \"%s\",\n"
                         "  \"messages\": [\n"
                         "    {\n"
                         "      \"role\": \"assistant\",\n"
                         "      \"content\": \"%s\"\n"
                         "    }\n"
                         "  ]\n"
                         "}", model, msg);
    printf("json_payload: %s\n", json_payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);

    // 응답을 저장할 구조체 초기화
    struct response_buffer resp_buf;
    resp_buf.data = malloc(1);  // 응답을 저장할 버퍼 초기화
                                // malloc 함수 void * 반환
                                // 단순히 주소값만 저장
    resp_buf.size = 0;

    // Curl 설정: 응답 콜백 함수 설정
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);    // 응답 콜백 함수 설정
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_buf);             // 콜백 함수 데이터 저장할 버퍼 설정

    // Curl 실행
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Unable to perform request: %s\n", curl_easy_strerror(res));
        exit(1);
    }

    printf("응답: %s\n", resp_buf.data);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("HTTP 상태 코드: %ld\n", http_code);

    // JSON 파싱: 응답을 JSON 형식으로 파싱
    cJSON *root = cJSON_Parse(resp_buf.data);
    if (root == NULL) {
        fprintf(stderr, "Unable to parse JSON.\n");
        exit(1);
    }

    // JSON 데이터에서 "choices"라는 키로 배열에 접근
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (!cJSON_IsArray(choices)) {
        fprintf(stderr, "Unable to get choices.\n");
        exit(1);
    }

    // JSON 데이터에서 배열의 첫 번째 요소에 접근
    cJSON *choice = cJSON_GetArrayItem(choices, 0);
    if (choice == NULL || !cJSON_IsObject(choice)) {
        fprintf(stderr, "Unable to get choice.\n");
        exit(1);
    }

    // JSON 데이터에서 "message" 객체에 접근
    cJSON *message = cJSON_GetObjectItem(choice, "message");
    if (message == NULL || !cJSON_IsObject(message)) {
        fprintf(stderr, "Unable to get message.\n");
        exit(1);
    }

    // JSON 데이터에서 "content" 문자열에 접근
    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (content == NULL || !cJSON_IsString(content)) {
        fprintf(stderr, "Unable to get content.\n");
        exit(1);
    }

    // content->valuestring을 반환해야 하는데
    // cJSON 구조체를 할당 해제도 해야 하므로
    // 문자열 복사와 메모리 복사를 함께 해주는 strdup 사용
    // 추후 main() 에서 할당 해제됨
    char *result = strdup(content->valuestring);

    // 메모리 해제
    cJSON_Delete(root);
    free(resp_buf.data);
    free(json_payload);

    // Curl 클린업
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

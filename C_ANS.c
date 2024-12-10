#include "C_API.h"
#include "C_DB.h"

// 메세지 배열 크기
#define MSG_SIZE 1024 * 32

// 큰따옴표와 이스케이프 문자를 제거하는 함수
void remove_quotes(char *str) {
    char *src = str;
    char *dst = str;
    
    while (*src) {
        if (*src != '"' && *src != '\n') {
            *dst++ = *src;  // 유효한 문자는 그대로 복사
        }
        src++;
    }
    *dst = '\0';  // 문자열 끝에 널 종료 문자 추가
}


// 메시지를 생성하는 함수
char* create_message(cJSON *root, char *req, char *lang, char *prob) {
    // 호출 때마다 gpt-4o-mini로 됨
    const char *model = "gpt-4o-mini";

    // 처음 호출될 때 한 번만 초기화됨
    static char *prior_req = "";
    // ChatGPT에 전달할 메세지 저장할 배열 생성
    char *msg = (char*)malloc(sizeof(char) * MSG_SIZE);
    // 최종 대답 저장할 포인터
    char *create_msg;

    // 쌍따옴표 제거
    // char형 포인터를 반환하는 함수가 아니라
    // 값을 반환하지 않고 strcpy를 통해서 문자열 복사를 해서
    // 쌍따옴표 제거한 문자열 만듦
    remove_quotes(req);
    remove_quotes(lang);
    remove_quotes(prob);

    // 요청이 IH 또는 WS일 때 데이터베이스에 중복 요청이 있는지 확인
    // 있으면 데이터베이스에서 꺼내오고 없으면 API 거쳐서 대답 생성
    if ((strcmp(req, "IH") == 0) || (strcmp(req, "WS") == 0)){
        // AnswerError일 때를 위해서 if문을 더 넣음
        // check는 데이터베이스에 관련 데이터가 있는지 확인하는 함수 && IH, WS 구분
        // IH인 경우
        if ((create_msg = check(req, prob, lang)) != NULL && strcmp(req, "IH") == 0) {
            prior_req = "IH";
            return create_msg;
        }
        // WS인 경우
        else if ((create_msg = check(req, prob, lang)) != NULL && strcmp(req, "WS") == 0) {
            prior_req = "WS";
            return create_msg;
        }
    }


    // request_type과 IH가 동일할 경우
    if (strcmp(req, "IH") == 0) {
        printf("%s 개념 설명\n", req);
        prior_req = "IH";
        // 이환희 학생의 프롬프트 적용
        snprintf(msg, MSG_SIZE,
            "주제: 백준 %s번에 대한 개념 설명 \
백준 %s번 문제에서 %s 언어와 관련된 필수적인 개념들이 무엇인지 문제를 분석해라. \
그리고 다음과 같은 형식으로 대답해라. \
\
형식: \
[개념 1] - (관련 개념 1) \
   (관련 개념 1 의미) \
   (관련 개념 1 역할) \
   (관련 개념 1이 적용된 예시 코드) \
   (관련 개념 1이 적용된 예시 코드 설명(어디에서 관련 개념 1이 쓰였는지도 밝힐 것)) \
\
  - 이 문제에서의 활용 \
   (본 문제에서 관련 개념 1이 적용된 부분 설명) \
\
[개념 2] - (관련 개념 2) \
   (관련 개념 2 의미) \
   (관련 개념 2 역할) \
   (관련 개념 2이 적용된 예시 코드) \
   (관련 개념 2이 적용된 예시 코드 설명(어디에서 관련 개념 1이 쓰였는지도 밝힐 것)) \
\
  - 이 문제에서의 활용 \
   (본 문제에서 관련 개념 2가 적용된 부분 설명) \
\
... \
\
주의: \
- 문제의 정답을 설명하면 안 됨 \
- 관련된 개념을 모르겠다면 모르겠다고 알림", prob, prob, lang);
    }
    // request_type과 OQ가 동일한 경우
    else if (strcmp(req, "OQ") == 0) {
        printf("%s 추가 질문\n", req);
        prior_req = "WS";
        cJSON *other_question = cJSON_GetObjectItem(root, "textElement");
        // js에서 Id 정보 넘겨줘야 함
        cJSON *user = cJSON_GetObjectItem(root, "Id");

        // 따옴표 제거
        remove_quotes(other_question->valuestring);
        remove_quotes(user->valuestring);

        // 토큰이 남아있는지 확인
        if (OQ_check(user->valuestring) > 0) {
            snprintf(msg, MSG_SIZE, "%s", other_question->valuestring);
            OQ_update(user->valuestring);
        }
        else {
            // 나중에 create_msg를 할당 해제하는 부분이 있어
            // 메모리 할당을 함께 해주는 strdup 사용
            create_msg = strdup("횟수 제한");
            return create_msg;
        }
    }
    // request_type과 WS가 동일한 경우
    else if (strcmp(req, "WS") == 0) {
        printf("%s 정답 보기\n", req);
        // 프롬프트 적용
        snprintf(msg, MSG_SIZE,
            "주제: 백준 %s번에 대한 정답 및 해설 \
백준 %s번 문제를 %s 언어를 이용해 풀어라. \
그리고 반드시 다음과 같은 형식으로 대답해라. \
\
형식: \
(문제에 대한 정답 코드) \
(정답 코드 설명을 포함한 해설 설명) \
\
주의: \
- 정확한 답을 모르겠다면 모르겠다고 알림", prob, prob, lang);
    }
    // request_type과 Edit이 동일한 경우
    else if (strcmp(req, "Edit") == 0) {
        printf("수정\n");
        cJSON *error_type = cJSON_GetObjectItem(root, "errortype");
        // js에서 Id 정보 넘겨줘야 함
        cJSON *user_code = cJSON_GetObjectItem(root, "usercode");
        remove_quotes(error_type->valuestring);
        remove_quotes(user_code->valuestring);
        snprintf(msg, MSG_SIZE,
            "실행해보니 %s가 발생했다. 다음 코드에서 틀린 부분을 고쳐주고 어떤 부분이 잘못되었는지 설명해라. \
%s", error_type->valuestring, user_code->valuestring);
    }
    // request_type과 AnswerError가 동일한 경우
    else if (strcmp(req, "AnswerError") == 0) {
        printf("응답 에러\n");
        printf("%s\n", prior_req);
        // DB에 잘못된 응답 제거하기
        delete_DB(prior_req, prob, lang);

        // 문제 가져오기
        char *Main = MainText(atoi(prob));
        remove_quotes(Main);

        // 모델을 업그레이드 해서 오류 가능성을 줄임
        model = "gpt-4o";
        if (strcmp(prior_req, "IH") == 0) {
            snprintf(msg, MSG_SIZE, "%s 이 백준 문제에서 %s 언어와 관련된 필수적인 개념들이 무엇인지 문제를 분석해라.", Main, lang);
        }
        else {
            snprintf(msg, MSG_SIZE, "%s 이 백준 문제를 %s 언어를 이용해 풀어라. \
그리고 정답 코드를 우선적으로 적고 다음과 같은 형식으로 대답해라. \
\
형식: \
(문제에 대한 정답 코드) \
(정답 코드 설명을 포함한 해설 설명) \
\
주의: \
- 정확한 답을 모르겠다면 모르겠다고 알림", Main, lang);
        }
        // 메모리 할당 해제
        free(Main);
    }
    // request_type과 OtherError가 동일한 경우
    else if (strcmp(req, "OtherError") == 0) {
        printf("다른 에러\n");
        cJSON *other_error = cJSON_GetObjectItem(root, "textElement");
        remove_quotes(other_error->valuestring);
        insert_DB(req, prob, lang, other_error->valuestring);
        create_msg = strdup("success");
        return create_msg;
    }

    // C_API.c의 chat 함수를 통해 ChatGPT API와 데이터 주고받음
    create_msg = chat(msg, model);

    // 동적 할당한 메모리 해제
    free(msg);

    // IH, WS에 해당하는 응답 데이터베이스에 넣기
    if ((strcmp(req, "IH") == 0) || (strcmp(req, "WS") == 0))
        insert_DB(req, prob, lang, create_msg);

    if (strcmp(req, "AnswerError") == 0)
        insert_DB(prior_req, prob, lang, create_msg);
    
    return create_msg;
}

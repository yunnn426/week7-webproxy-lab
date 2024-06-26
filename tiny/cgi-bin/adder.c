/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(int argc, char **argv) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');     // &를 찾음
    *p = '\0';                // &을 \0(NULL)으로 교체
    strcpy(arg1, buf);        // 인자 1
    strcpy(arg2, p + 1);      // 인자 2
    n1 = atoi(arg1);
    n2 = atoi(arg2);
    *p = '&';
  }

  // content 버퍼에 저장
  // 바디 생성
  sprintf(content, "QUERY_STRING=%s\n", buf);
  sprintf(content, "%sWelcome to add.com: ", content);
  sprintf(content, "%sTHE Internet addition portal\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  // HTTP 응답 생성
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");    // 마지막 빈 줄 \r\n
  
  // HEAD 요청이면 바로 리턴
  // if (!strcasecmp(argv[0], "HEAD"))    // 실행파일 매개변수에서 불러온 경우 (내 풀이)
  if (!strcasecmp(getenv("REQUEST_METHOD"), "HEAD"))
    exit(0);
  // GET 요청이면 바디까지 출력
  printf("%s", content);

  fflush(stdout);

  exit(0);    // 자식 프로세스 종료
}
/* $end adder */
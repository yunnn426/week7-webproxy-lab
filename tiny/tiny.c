/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(char *method, int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(char *method, int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

// HTTP 트랜잭션 한 개 처리
void doit(int fd) {
  int is_static;
  // 파일 정보를 담는 구조체 stat,
  // 구조체 타입 변수 sbuf
  struct stat sbuf;
  // buf: 클라이언트의 요청 읽어오기 위해
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and header */ 
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // buf에 있는 내용을 m, u, v에 저장
  
  // 11.11 HEAD request
  // GET 요청 처리
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
    clienterror(fd, method, "501", "Not implemented", 
              "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio); // 요청 헤더는 읽고 무시함

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0) { // 필요한 파일이 존재하지 않는 파일인 경우
    clienterror(fd, filename, "404", "Not found",
              "Tiny couldn't find this file");
    return;
  }

  /* serve static content */
  if (is_static) {
    // 일반 파일이 아니거나
    // 사용자가 읽기 가능한 파일이 아닌 경우
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                "Tiny couldn't read this file");
      return;
    }
    serve_static(method, fd, filename, sbuf.st_size);
  }

  /* serve dynamic content */
  else {
    // 일반 파일이 아니거나
    // 사용자가 실행 가능한 파일이 아닌 경우
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                "Tiny couldn't run this CGI program");
      return;
    }
    serve_dynamic(method, fd, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  // 클라이언트에게 응답을 보내주기 위한 buf
  // 그 응답을 담고있는 body
  char buf[MAXLINE], body[MAXBUF];
  
  /* Build the HTTP body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // %s로 기존 body 내용 불러와서 뒤에 붙이고 다시 그걸 body에 저장
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  // 헤더 읽어서 출력만 함
  // do while로 바꿔봄 -> 안되는데
  // do { 
  //   Rio_readlineb(rp, buf, MAXLINE);
  //   printf("%s", buf);
  // } while (strcmp(buf, "\r\n") != 0);

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {      // 같으면 0 리턴
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;

  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");      // cgiargs init
    strcpy(filename, ".");    // filename init
    strcat(filename, uri);    // concat filename, uri ex. "./index.html"
    if (uri[strlen(uri) - 1] == '/') { // default page
      strcat(filename, "home.html");
    }
    return 1;
  }
  else {
    ptr = index(uri, '?');    // ?의 위치 반환
    if (ptr) {                // 전달할 인자가 있으면
      strcpy(cgiargs, ptr + 1);   // ptr 뒤에 인자가 위치
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(char *method, int fd, char *filename, int filesize) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD 요청이면 헤더만 출력하고 리턴 
  if (!strcasecmp(method, "HEAD"))
    return;

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);    // 파일 디스크립터 반환
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);   // 시작 주소 srcp에 파일 내용 저장
  srcp = Malloc(filesize);    // 11.9 change to malloc
  Rio_readn(srcfd, srcp, filesize); // 11.9 change to malloc
  Close(srcfd);   // 파일 읽어왔으니 디스크립터 닫아주기
  Rio_writen(fd, srcp, filesize);   // 클라이언트에 저장된 내용 반환
  // Munmap(srcp, filesize);   // 메모리 해제
  Free(srcp);   // 11.9 change to malloc
}

// html, gif, png, jpg, plaintext의 타입 지정
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else 
    strcpy(filetype, "text/plain");
}

void serve_dynamic(char *method, int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *list[] = { NULL };
  // *list[] = { method, NULL };   -> 실행 파일의 인자로 넘겨주는 경우 (내 풀이)

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // 11.10 parse edit
  // 초기: n=3&m=5\0 -> 수정: 3&5\0
  char newargs[MAXLINE];
  int is_changed = 0; // 폼에서 넘어온 경우 / uri에 인자 입력해 넘어온 경우 구분해주기 위해
  strcpy(newargs, "");
  for (int i = 0; i < strlen(cgiargs); i++) {   // sizeof(cgiargs) 로 하니까 틀림
    if (cgiargs[i] == 'n' || cgiargs[i] == 'm' || cgiargs[i] == '=') {
      is_changed = 1;
      continue;
    }
    char temp[2] = {cgiargs[i], '\0'};  // char -> string
    strcat(newargs, temp);
  }

  if (Fork() == 0) {    // 자식 프로세스 생성
    if (!is_changed)             // uri에 인자 직접 쳐서 넘어온 경우
      setenv("QUERY_STRING", cgiargs, 1);     // QUERY_STRING 환경변수를 cgiargs로 설정
    else 
      setenv("QUERY_STRING", newargs, 1);     // 사용자 입력을 받아 넘어온 경우
    setenv("REQUEST_METHOD", method, 1);      // request method를 환경변수에 저장
    Dup2(fd, STDOUT_FILENO);                // 표준 출력을 자식 프로세스의 소켓으로 지정
    Execve(filename, list, environ);   // filename 프로그램을 실행 (list에 실행시키기 위한 인자, environ에 환경 변수를 전달)
  }
  Wait(NULL);           // 자식 프로세스가 모두 종료될 때(exit(0))까지 부모 프로세스 기다림
}
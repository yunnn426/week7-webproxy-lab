#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd); // rio init

    while (Fgets(buf, MAXLINE, stdin) != NULL) { // 사용자 입력
        Rio_writen(clientfd, buf, strlen(buf)); // 클라이언트가 버퍼에 씀 (1번 짝)
        Rio_readlineb(&rio, buf, MAXLINE); // 클라가 버퍼에 쓰인 내용 읽어와서 버퍼에 저장 (2번 짝) // echo의 Rio_writen(connfd, buf, n)을 위해 필요, 안쓸거면 둘다 주석처리 해야함
        Fputs(buf, stdout); // 버퍼에 쓰인 내용 출력
    }
    Close(clientfd);
    exit(0);
}
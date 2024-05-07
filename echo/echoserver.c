#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1) {
        // sockaddr_in: IPv4, sockaddr_in6: IPv6용
        // sockaddr_storage: 둘다 대응
        clientlen = sizeof(struct sockaddr_storage); 
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    char *strr = "hello\n";

    rio_t rio;

    Rio_readinitb(&rio, connfd);
    // 버퍼에 담아놔서 한 번에 보내기 위해
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { // 서버가 버퍼에 쓰인 내용 갖고옴 (1번 짝)
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n); // 서버가 버퍼에 또 씀 (2번 짝)
        printf("Recieved from client: ");
        Fputs(buf, stdout);
    }
}
#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp, char *buf, char *hostname, char *port);
void parse_uri(char *uri, char *hostname, char *path, char *port);
void *thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("proxy: Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp);     
    }
}

/* Thread routine */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);                                             
    Close(connfd);  
    return NULL;
}


/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    char port[16];
    rio_t rio_client, rio_server;

    /* Read request line */
    Rio_readinitb(&rio_client, fd);
    if (!Rio_readlineb(&rio_client, buf, MAXLINE))  
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);      
    if (strcasecmp(method, "GET")) {                   
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }                 

    parse_uri(uri, hostname, path, port);

    /* send request to server */
    printf("proxy connet to host %s port %s\n", hostname, port);
    int serverfd = Open_clientfd(hostname, port);
    Rio_readinitb(&rio_server, serverfd);

    char write_buf[MAXBUF];
    sprintf(write_buf, "GET %s HTTP/1.0\r\n", path);
    read_requesthdrs(&rio_client, write_buf, hostname, port);
    Rio_writen(serverfd, write_buf, strlen(write_buf));
    printf("writebuf\n %s\n", write_buf);

    /* transfer response from server to client */
    int n;
    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE))) {
        //printf("proxy received %d bytes,then send\n",n);
        Rio_writen(fd, buf, n);
    }

    Close(serverfd);
}
/* $end doit */

/* read request headers, append them to buf */
void read_requesthdrs(rio_t *rp, char *buf, char *hostname, char *port) 
{
    char read_buf[MAXLINE];

    while (Rio_readlineb(rp, read_buf, MAXLINE)) {   
    if (!strcmp(read_buf, "\r\n")) {
        break;
    }
    if (strstr(read_buf, "Host:")) {
        sscanf(read_buf, "Host: %s\r\n", hostname);
        continue;
    }
    if (strstr(read_buf, "User-Agent:") || strstr(read_buf, "Connection:") || strstr(read_buf, "Proxy-Connection:")) {
        continue;
    }

    sprintf(buf, "%s%s", buf, read_buf);
	
	printf("%s", buf);
    }

    sprintf(buf, "%sHost: %s:%s\r\n", buf, hostname, port);
    sprintf(buf, "%s%s%s%s", buf, user_agent_hdr, conn_hdr, prox_hdr);
    sprintf(buf, "%s\r\n", buf);
    
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into hostname, path and port
 */
void parse_uri(char *uri, char *hostname, char *path, char *port) 
{
    char *ptr, *ptr2;
    int port_num;

    if ((ptr = strstr(uri, "//")) != NULL) { /* http://xxx */
    ptr += 2;
    }
    else {
    ptr = uri;
    }

    if ((ptr2 = strstr(ptr, ":")) != NULL) { /* port number specified */
    *ptr2 = '\0';
    strcpy(hostname, ptr);
    sscanf(ptr2+1, "%d%s", &port_num, path);
    sprintf(port, "%d", port_num);    
    *ptr2 = ':';
    }
    else {
    sprintf(port, "80");
        if ((ptr2 = strstr(ptr, "/")) != NULL) { /* xxx.yyy.zzz/balabala */
        *ptr2 = '\0';
        strcpy(hostname, ptr);
        *ptr2 = '/';
        strcpy(path, ptr2);
        }
        else {
        strcpy(hostname, ptr);
        sprintf(path, "/");
        }
    }
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
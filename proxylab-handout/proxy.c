#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void *doit(void *vargp);
void parse_uri(char *uri, char *host, char *port, char *path);
void send_request(int fd, char *uri);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


int main(int argc, char **argv)
{
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    pthread_t tid;
    int *connfd, listenfd;
    if (argc !=2) {
        fprintf(stderr,"usage: %s <port>\n", argv[0]);
        exit(1);
    }
    cache_init(MAX_CACHE_SIZE);
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);
        Getnameinfo((SA*) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, doit, connfd);
    }
    cache_free();
    exit(0);
}

void *doit(void *vargp)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    int fd = *((int *) vargp); Free(vargp);
    Pthread_detach(pthread_self());
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return NULL;
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return NULL;
    }
    send_request(fd, uri);
    Close(fd);
    return NULL;
}

void parse_uri(char *uri, char *host, char *port, char *path)
{
    char *ptr;
    if ((ptr = strstr(uri, "http")) == NULL)
        ptr = uri;
    else
        ptr = (ptr[4]=='s') ? ptr+8 : ptr+7;
    
    if (strchr(ptr,':'))
        sscanf(ptr, "%[^:]:%[^/]%s", host, port, path);
    else {
        sscanf(ptr, "%[^/]%s", host, path);
        sprintf(port, "80");
    }
    if (strlen(path)==0)
        snprintf(path, MAXLINE, "/");
}

void send_request(int clientfd, char *uri)
{
    char cbuf[MAX_OBJECT_SIZE], buf[MAXLINE];
    char key[MAXLINE], host[MAXLINE], path[MAXLINE], port[8];
    volatile node_t *cache_data = NULL;
    unsigned int filesize = 0, totalsize = 0;
    parse_uri(uri, host, port, path);
    snprintf(key, MAXLINE, "%s%s", host, path);
    cache_data = cache_get(key);
    if (cache_data != NULL)
        Rio_writen(clientfd, cache_data->val, cache_data->fsize);
    else
    {
        int fd = Open_clientfd(host, port);
        rio_t rio;
        sprintf(buf, "GET %s HTTP/1.0\r\n", path);
        Rio_writen(fd, buf, strlen(buf));
        sprintf(buf, "Host: %s\r\n", host);
        Rio_writen(fd, buf, strlen(buf));
        // User-agent
        Rio_writen(fd, (char *) user_agent_hdr, strlen(user_agent_hdr));
        sprintf(buf, "Connection: close\r\n");
        Rio_writen(fd, buf, strlen(buf));
        sprintf(buf, "Proxy-Connection: close\r\n");
        Rio_writen(fd, buf, strlen(buf));
        sprintf(buf, "\r\n");
        Rio_writen(fd, buf, strlen(buf));

        Rio_readinitb(&rio, fd);
        totalsize = 0;
        filesize = 0;
        while ( (filesize = Rio_readnb(&rio, buf, sizeof(buf)))>0 )
        {
            Rio_writen(clientfd, buf, filesize);
            totalsize += filesize;
            if (totalsize<MAX_OBJECT_SIZE) memcpy(cbuf+totalsize-filesize, buf, filesize);
        }
        Close(fd);
        if (totalsize<=MAX_OBJECT_SIZE)
            cache_set(key, cbuf, totalsize);
    }
    return;
}

void clienterror(int fd, char* cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
}

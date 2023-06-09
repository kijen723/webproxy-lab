#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";

static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

void doit(int connfd);
void parse_uri(char *uri,char *hostname,char *path,int *port);
void build_http_header(char *http_header,char *hostname,char *path,int port, rio_t *client_rio);
int connect_endServer(char *hostname,int port,char *http_header);
void *thread(void *vargsp);

void init_cache(void);
int reader(int connfd, char *url);
void writer(char *url, char *buf);

typedef struct {
  char *url;
  int *flag;
  int *cnt;
  int *content;
} Cache_info;

Cache_info *cache;
int readcnt;
sem_t mutex, w;

int main(int argc, char **argv) {

  if(argc != 2) {
    fprintf(stderr, "usage :%s <port> \n", argv[0]);
    exit(1);
  }

  init_cache();
  int listenfd, connfd;
  socklen_t clientlen;
  char hostname[MAXLINE], port[MAXLINE];
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  listenfd = Open_listenfd(argv[1]);

  while(1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

    Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s %s).\n",hostname, port);

    Pthread_create(&tid, NULL, thread, (void *)connfd);
  }
  return 0;
}

void *thread(void *vargs) {
  int connfd = (int)vargs;
  Pthread_detach(pthread_self()); 
  doit(connfd);
  Close(connfd);
}

void doit(int connfd) {
  int end_serverfd;

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char endserver_http_header[MAXLINE];
  
  char hostname[MAXLINE], path[MAXLINE];
  int port;

  rio_t rio, server_rio;

  char url[MAXLINE];
  char content_buf[MAX_OBJECT_SIZE];

  Rio_readinitb(&rio, connfd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);
  strcpy(url, uri);

  if (strcasecmp(method, "GET")) {
    printf("Proxy does not implement the method");
    return;
  }
  
  if (reader(connfd, url)) {
    return ;
  }

  parse_uri(uri, hostname, path, &port);
  build_http_header(endserver_http_header, hostname, path, port, &rio);

  end_serverfd = connect_endServer(hostname, port, endserver_http_header);
  if (end_serverfd < 0) {
    printf("connection failed\n");
    return;
  }

  Rio_readinitb(&server_rio, end_serverfd);
  Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

  size_t n;
  int total_size = 0;
  while((n = Rio_readlineb(&server_rio,buf, MAXLINE)) != 0) {
    printf("proxy received %d bytes,then send\n",n);
    Rio_writen(connfd, buf, n);

    if (total_size + n < MAX_OBJECT_SIZE) {
      strcpy(content_buf + total_size, buf);
    }
    total_size += n;
  }

  if (total_size < MAX_OBJECT_SIZE) {
    writer(url, content_buf);
  }

  Close(end_serverfd);
}

void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio) {
  char buf[MAXLINE],request_hdr[MAXLINE],other_hdr[MAXLINE],host_hdr[MAXLINE];

  sprintf(request_hdr, requestlint_hdr_format, path);

  while(Rio_readlineb(client_rio, buf, MAXLINE)>0) {
    if (strcmp(buf, endof_hdr) == 0) break;

    if (!strncasecmp(buf, host_key, strlen(host_key))) {
        strcpy(host_hdr, buf);
        continue;
    }

    if (strncasecmp(buf, connection_key, strlen(connection_key))
          && strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
          && strncasecmp(buf, user_agent_key, strlen(user_agent_key))) {
      strcat(other_hdr,buf);
    }
  }
  if (strlen(host_hdr) == 0) {
      sprintf(host_hdr,host_hdr_format,hostname);
  }

  sprintf(http_header,"%s%s%s%s%s%s%s", request_hdr, host_hdr, conn_hdr, prox_hdr, user_agent_hdr, other_hdr, endof_hdr);
  
  return ;
}

inline int connect_endServer(char *hostname,int port,char *http_header) {
  char portStr[100];
  sprintf(portStr,"%d",port);
  return Open_clientfd(hostname,portStr);
}

void parse_uri(char *uri,char *hostname,char *path,int *port) {
  *port = 80;
  char* pos = strstr(uri,"//");

  pos = pos!=NULL? pos+2:uri;

  char *pos2 = strstr(pos,":");
  if (pos2!=NULL) {
    *pos2 = '\0';
    sscanf(pos,"%s",hostname);
    sscanf(pos2+1,"%d%s",port,path);
  } else {
    pos2 = strstr(pos,"/");
    if (pos2!=NULL) {
      *pos2 = '\0';
      sscanf(pos,"%s",hostname);
      *pos2 = '/';
      sscanf(pos2,"%s",path);
    } else {
      sscanf(pos,"%s",hostname);
    }
  }
  return;
}

void init_cache() {
  Sem_init(&mutex, 0, 1);
  Sem_init(&w, 0, 1);
  readcnt = 0;
  cache = (Cache_info *)Malloc(sizeof(Cache_info) * 10);
  for (int i = 0; i < 10; i++) {
      cache[i].url = (char *)Malloc(sizeof(char) * 256);
      cache[i].flag = (int *)Malloc(sizeof(int));
      cache[i].cnt = (int *)Malloc(sizeof(int));
      cache[i].content = (char *)Malloc(sizeof(char) * MAX_OBJECT_SIZE);
      *(cache[i].flag) = 0;
      *(cache[i].cnt) = 1;
  }
}

int reader(int connfd, char *url) {
  int return_flag = 0;
  P(&mutex);              
  readcnt++;
  if(readcnt == 1) {
    P(&w);
  }
  V(&mutex);

  for(int i = 0; i < 10; i++) {
    if(*(cache[i].flag) == 1 && !strcmp(cache[i].url, url)) {
      Rio_writen(connfd, cache[i].content, MAX_OBJECT_SIZE);
      return_flag = 1;
      *(cache[i].cnt) = 1;
      break;
    }
  }    

  for(int i = 0; i < 10; i++) {
    (*(cache[i].cnt))++;
  }

  P(&mutex);
  readcnt--;
  if(readcnt == 0) {
    V(&w);
  }
  V(&mutex);
  return return_flag;
}

void writer(char *url, char *buf) {
  P(&w);

  int idx = 0;
  int max_cnt = 0;

  for(int i = 0; i < 10; i++) {
    if(*(cache[i].flag) == 0) {
      idx = i;
      break;
    }
    if(*(cache[i].cnt) > max_cnt) {
      idx = i;
      max_cnt = *(cache[i].cnt);
    }
  }

  *(cache[idx].flag) = 1;
  strcpy(cache[idx].url, url);
  strcpy(cache[idx].content, buf);
  *(cache[idx].cnt) = 1;

  V(&w);
}
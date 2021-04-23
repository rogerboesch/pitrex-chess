
#include <arpa/inet.h>          
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define LISTENQ 1024
#define MAXLINE 1024
#define RIO_BUFSIZE 1024

// MARK: - Callbacks

void log_message(char* msg) { printf(" SERVER: %s\n", msg); }
void display_code(char* ip) {}
extern int process_request(char* request);

// MARK: - Internal structures

typedef struct {
    int rio_fd;                 // descriptor for this buffer
    int rio_cnt;                // unread byte in this buffer
    char* rio_bufptr;           // next unread byte in this buffer
    char rio_buf[RIO_BUFSIZE];  // internal buffer
} rio_t;

typedef struct sockaddr SA;

typedef struct {
    char uri[MAXLINE*4];
    char filename[512];
    off_t offset;
    size_t end;
} http_request;

// MARK: - Buffer handling

static void read_init(rio_t* rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

static ssize_t write_buffer(int fd, void* usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;
    
    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;
            }
            else {
                return -1;
            }
        }
        
        nleft -= nwritten;
        bufp += nwritten;
    }
    
    return n;
}

static ssize_t read_buffer(rio_t* rp, char* usrbuf, size_t n) {
    int cnt;
    
    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = (int)read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        
        if (rp->rio_cnt < 0){
            if (errno != EINTR) {
                return -1;
            }
        }
        else if (rp->rio_cnt == 0) {
            return 0;
        }
        else {
            rp->rio_bufptr = rp->rio_buf;
        }
    }

    cnt = (int)n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }
    
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    
    return cnt;
}

static ssize_t read_line(rio_t* rp, void* usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char* bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = (int)read_buffer(rp, &c, 1)) == 1) {
            *bufp++ = c;
            
            if (c == '\n') {
                break;
            }
        }
        else if (rc == 0) {
            if (n == 1) {
                return 0;
            }
            else {
                break;
            }
        }
        else {
            return -1;
        }
    }
    
    *bufp = 0;
    return n;
}

// MARK: - Internal methods

static void client_result(int fd, int status, char* msg, char* longmsg) {
    char buf[MAXLINE];
    
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf), "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "Cache-Control: no-cache\r\n");
    sprintf(buf + strlen(buf), "%s", longmsg);

    write_buffer(fd, buf, strlen(buf));

    sprintf(buf, "HTTP/1.1 %d %s (%s)", status, msg, longmsg);
    log_message(buf);
}

static void log_access(int status, struct sockaddr_in* c_addr, http_request* req) {
    char buf[MAXLINE];
    sprintf(buf, "Access %s:%d %d - %s", inet_ntoa(c_addr->sin_addr), ntohs(c_addr->sin_port), status, req->filename);
    
    log_message(buf);
}

static void url_decode(char* src, char* dest, int max) {
    char *p = src;
    char code[3] = { 0 };
    
    while (*p && --max) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        }
        else {
            *dest++ = *p++;
        }
    }
    
    *dest = '\0';
}

// MARK: - Request handling

static void parse_request(int fd, http_request* req){
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    rio_t rio;
    req->offset = 0;
    req->end = 0;
    
    read_init(&rio, fd);
    read_line(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s", method, uri);
    
    // read all
    while (buf[0] != '\n' && buf[1] != '\n') { //  \n || \r\n
        read_line(&rio, buf, MAXLINE);
        
        if (buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n') {
            sscanf(buf, "Range: bytes=%llu-%lu", &req->offset, &req->end);
            
            // Range: [start, end]
            if (req->end != 0) {
                req->end ++;
            }
        }
    }
    
    // Decode & save URI
    url_decode(uri, req->uri, MAXLINE*4);
    
    // Save filename
    char* filename = uri;
    if (uri[0] == '/') {
        filename = uri + 1;
        int length = (int)strlen(filename);
        
        if (length == 0) {
            filename = ".";
        }
        else {
            for (int i = 0; i < length; ++ i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    
    url_decode(filename, req->filename, MAXLINE);
}

static void process(int fd, struct sockaddr_in* clientaddr) {
    char buf[MAXLINE];
    sprintf(buf, "Accept request, fd is %d, pid is %d", fd, getpid());
    log_message(buf);
    
    // Parse
    http_request req;
    parse_request(fd, &req);
    
    // Call callback
    int result = process_request(req.uri);

    if (result == 1) {
        client_result(fd, 200, "OK", "OK");
        log_access(200, clientaddr, &req);
    }
    else {
        client_result(fd, 400, "Error", "Process request failed");
        log_access(400, clientaddr, &req);
    }
    
    close(fd);
}

// MARK: - Socket handling

static int listAddress() {
    struct ifreq buffer[32];
    struct ifconf intfc;
    struct ifreq* pIntfc;
    int i, fd, num_intfc;
    
    intfc.ifc_len = sizeof(buffer);
    intfc.ifc_buf = (char*)buffer;
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        char buf[MAXLINE];
        sprintf(buf, "Error: socket() failed");
        log_message(buf);
        
        return -1;
    }
    
    if (ioctl(fd, SIOCGIFCONF, &intfc) < 0) {
        char buf[MAXLINE];
        sprintf(buf, "Error: ioctl SIOCGIFCONF failed");
        log_message(buf);
        
        return -2;
    }
    
    pIntfc = intfc.ifc_req;
    num_intfc = intfc.ifc_len / sizeof(struct ifreq);
    
    char buf[MAXLINE];
    
    for (i = 0; i < num_intfc; i++) {
        struct ifreq *item = &(pIntfc[i]);
        sprintf(buf, "Interface # %d -> %s: IP %s", i, item->ifr_name, inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));
        log_message(buf);
		display_code(inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));
    }
    
    return num_intfc;
}

static int openSocket(int port) {
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    // Create a socket descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    // Eliminates "Address already in use" error from bind
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0) {
        return -1;
    }

    // 6 is TCP's protocol number. Enable this, much faster : 4000 req/s -> 17000 req/s
#ifdef __APPLE__
#else
    if (setsockopt(listenfd, 6, TCP_CORK, (const void *)&optval , sizeof(int)) < 0) {
        return -1;
    }
#endif
    
    // Listenfd will be an endpoint for all requests to port on any IP address for this host
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) {
        return -1;
    }

    // Make it a listening socket ready to accept connection requests
    if (listen(listenfd, LISTENQ) < 0) {
        return -1;
    }
    
    return listenfd;
}

int run_web_server(int port) {
    struct sockaddr_in clientaddr;
    int listenfd, connfd;
    socklen_t clientlen = sizeof clientaddr;
    
    listenfd = openSocket(port);
    if (listenfd > 0) {
        char buf[MAXLINE];
        sprintf(buf, "Listen on port %d, fd is %d", port, listenfd);
        log_message(buf);
    }
    else {
        char buf[MAXLINE];
        sprintf(buf, "Error listen on port %d, fd is %d", port, listenfd);
        log_message(buf);

        return -1;
    }
    
    // Ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);

    listAddress();

    while (1) {
        char buf[MAXLINE];
        sprintf(buf, "Wait on port %d, fd is %d", port, listenfd);
        log_message(buf);
 
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
 
        process(connfd, &clientaddr);
        close(connfd);
    }

    return 0;
}

// adapted from http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "net_lib.h"

#ifdef USE_LIBCURL
#include <curl/curl.h>
#endif

#define PORT 80
#define USERAGENT "HTMLGET 1.0"

#define BUFFER_SIZE 4096
#define VERBOSE 0

int get_socket_connection_OLD(char *host, struct sockaddr_in **premote) {

    struct sockaddr_in *remote = *premote;

    int tmpres;

    //fprintf(stdout, "DEBUG: create_tcp_socket in\n");
    fflush(stdout);
    int sock = create_tcp_socket();
    //fprintf(stdout, "DEBUG: create_tcp_socket return: sock %d\n", sock);
    fflush(stdout);

    //fprintf(stdout, "DEBUG: get_ip in\n");
    fflush(stdout);
    char *ip = get_ip(host);

    if (VERBOSE) fprintf(stdout, "IP is %s\n", ip);
    //fprintf(stdout, "DEBUG: get_socket_connection: IP is %s\n", ip);
    fflush(stdout);
    //remote = (struct sockaddr_in *) malloc(sizeof (struct sockaddr_in *));
    remote = (struct sockaddr_in *) malloc(sizeof (struct sockaddr_in));
    remote->sin_family = AF_INET;
    tmpres = inet_pton(AF_INET, ip, (void *) (&(remote->sin_addr.s_addr)));
    //fprintf(stdout, "DEBUG: inet_pton return: tmpres %d\n", tmpres);
    fflush(stdout);
    if (tmpres < 0) {
        perror("net.get_socket_connection: Can't set remote->sin_addr.s_addr");
        free(ip);
        return (-1);
    } else if (tmpres == 0) {
        fprintf(stderr, "net.get_socket_connection: %s is not a valid IP address\n", ip);
        free(ip);
        return (-1);
    }
    remote->sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *) remote, sizeof (struct sockaddr)) < 0) {
        perror("net.get_socket_connection: Could not connect");
        free(ip);
        return (-1);
    }
    //fprintf(stdout, "DEBUG: connect return: sock %d\n", sock);
    fflush(stdout);

    free(ip);

    *premote = remote;

    return (sock);

}

//static struct addrinfo res_saved;
//static char host_name_saved[1024] = "$$$";
//static char service_name_saved[1024] = "$$$";

int get_socket_connection(char *host_name, char *service_name) {

    struct addrinfo hints, *res, *res0, addrinfo_work;
    int error;
    int sock = -1;
    const char *cause = NULL;

    // check if we already have address for host, avoiding excessive calls to getaddrinfo which sometimes blocks
    res0 = NULL;
    /*if (0 && strcmp(host_name, host_name_saved) == 0 && strcmp(service_name, service_name_saved) == 0) {
        addrinfo_work = res_saved;
    } else {*/
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    //hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //fprintf(stdout, "DEBUG: getaddrinfo in: host_name |%s|, service_name |%s|\n", host_name, service_name);
    error = getaddrinfo(host_name, service_name, &hints, &res0);
    //fprintf(stdout, "DEBUG: getaddrinfo out: host_name |%s|, service_name |%s|\n", host_name, service_name);
    if (error) {
        perror(gai_strerror(error));
        if (res0 != NULL)
            freeaddrinfo(res0);
        return (-1);
        /*NOTREACHED*/
    }
    addrinfo_work = *res0;
    //}
    sock = -1;
    for (res = &addrinfo_work; res; res = res->ai_next) {
        //fprintf(stdout, "DEBUG: socket in: sock %d\n", sock);
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        //fprintf(stdout, "DEBUG: socket return: sock %d\n", sock);
        if (sock < 0) {
            cause = "get_socket_connection: socket";
            continue;
        }

        //fprintf(stdout, "DEBUG: connect in: sock %d\n", sock);
        if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
            cause = "get_socket_connection: connect";
            close(sock);
            sock = -1;
            continue;
        }
        //fprintf(stdout, "DEBUG: connect return: sock %d\n", sock);

        /*res_saved = *res;
        res_saved.ai_next = 0;
        strcpy(host_name_saved, host_name);
        strcpy(service_name_saved, service_name);
         */

        break; // okay we got one
    }
    //fprintf(stdout, "DEBUG: freeaddrinfo in: sock %d\n", sock);
    if (res0 != NULL)
        freeaddrinfo(res0);
    //fprintf(stdout, "DEBUG: freeaddrinfo return: sock %d\n", sock);
    if (sock < 0) {
        perror(cause);
        return (-1);
        /*NOTREACHED*/
    }

    // set options
    // timeout 20101217 AJL
    static struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof (timeout)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_SNDTIMEO");
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof (timeout)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_RCVTIMEO");

    // linger 20110204 AJL
    static struct linger ling;
    // 20160728 AJL  ling.l_onoff = 0;
    // 20160728 AJL - try immediate close of socket when close() called, MAY reduce delays on slow or bad connections???)
    ling.l_onoff = 1;
    ling.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &ling, sizeof (ling)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_LINGER");

    return (sock);

}

char *get_page(char *host, int sock, char *page, int *ppage_length) {

    char *get;
    char buf[BUFFER_SIZE + 1];

    int page_length = 0;
    char *page_contents = (char *) malloc(2 * BUFFER_SIZE);
    int buffer_size = 2 * BUFFER_SIZE;
    page_contents[0] = '\0';

    get = build_get_query(host, page);
    if (VERBOSE) fprintf(stdout, "Query is:\n<<START>>\n%s<<END>>\n", get);

    //Send the query to the server
    int sent = 0;
    int tmpres;
    while (sent < (int) strlen(get)) {
        //fprintf(stdout, "DEBUG: send(sock, get + sent, strlen(get) - sent, 0): sock %d\n", sock);
        fflush(stdout);
        tmpres = send(sock, get + sent, strlen(get) - sent, 0);
        if (tmpres == -1) {
            perror("net.get_page: Can't send query");
            free(page_contents);
            free(get);
            return (NULL);
        }
        sent += tmpres;
    }
    //now it is time to receive the page
    memset(buf, 0, sizeof (buf));
    int htmlstart = 0;
    char * htmlcontent;
    int read_len;
    while ((read_len = recv(sock, buf, BUFFER_SIZE, 0)) > 0) {
        /* AJL 20100216 - disabled, probably OK to have pre-HTML content
        if (htmlstart == 0) {
            // Under certain conditions this will not work.
            // If the \r\n\r\n part is splitted into two messages
            // it will fail to detect the beginning of HTML content
            htmlcontent = strstr(buf, "\r\n\r\n");
            if (htmlcontent != NULL) {
                htmlstart = 1;
                htmlcontent += 4;
                read_len -= htmlcontent - buf;
            }
        } else {
            htmlcontent = buf;
        }
         */
        htmlcontent = buf;
        htmlstart = 1;
        // END - AJL 20100216
        if (htmlstart) {
            ////fprintf(stdout, "DEBUG: htmlcontent= %s\n", htmlcontent);
            if (page_length + read_len >= buffer_size) {
                buffer_size = page_length + read_len + BUFFER_SIZE;
                page_contents = realloc(page_contents, buffer_size);
            }
            memcpy(page_contents + page_length, htmlcontent, read_len);
            page_length += read_len;
            page_contents[page_length] = '\0';
        }

        memset(buf, 0, read_len);
    }
    if (read_len < 0) {
        perror("net.get_page: Error receiving data");
        free(page_contents);
        page_contents = NULL;
        page_length = 0;
    }
    if (page_length == 0) {
        perror("net.get_page: Receiving 0 length data");
        free(page_contents);
        page_contents = NULL;
    }

    free(get);

    *ppage_length = page_length;

    return (page_contents);
}

/* following value made static to avoid valgrind error:
==53206== Syscall param socketcall.setsockopt(optval) points to uninitialised byte(s)
==53206==    at 0x1016A3502: setsockopt (in /usr/lib/libSystem.B.dylib)
...
==53206==  Uninitialised value was created by a stack allocation
==53206==    at 0x10003CA4C: create_tcp_socket (in /Users/anthony/bin/miniseed_process)
 */

int create_tcp_socket() {
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("net.create_tcp_socket: Can't create TCP socket");
        return (-1);
    }
    // set options
    // timeout 20101217 AJL
    static struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof (timeout)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_SNDTIMEO");
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof (timeout)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_RCVTIMEO");

    // linger 20110204 AJL
    static struct linger ling;
    ling.l_onoff = 0;
    ling.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &ling, sizeof (ling)) < 0)
        perror("net.create_tcp_socket: Can't setsockopt SO_LINGER");

    return sock;
}

//static int ip_count = 0;

char *get_ip(char *host) {
    struct hostent *hent;
    int iplen = 15; //XXX.XXX.XXX.XXX
    char *ip = (char *) malloc(iplen + 1);
    memset(ip, 0, iplen + 1);
    //fprintf(stdout, "DEBUG: get_ip: gethostbyname in: host %s\n", host);
    fflush(stdout);
    if ((hent = gethostbyname(host)) == NULL) {
        herror("Can't get IP");
        return (NULL);
    }
    //fprintf(stdout, "DEBUG: get_ip: inet_ntop in host %s\n", host);
    fflush(stdout);
    if (inet_ntop(AF_INET, (void *) hent->h_addr_list[0], ip, iplen) == NULL) {
        perror("net.create_tcp_socket: Can't resolve host");
        return (NULL);
    }
    //fprintf(stdout, "DEBUG: get_ip: inet_ntop return host %s\n", host);
    fflush(stdout);
    //fprintf(stdout, "gethostbyname: ip_count %d\n", ip_count++);
    return ip;
}

char *build_get_query(char *host, char *page) {
    char *query;
    char *getpage = page;
    char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
    if (getpage[0] == '/') {
        getpage = getpage + 1;
        if (VERBOSE) fprintf(stdout, "Removing leading \"/\", converting %s to %s\n", page, getpage);
    }
    // -5 is to consider the %s %s %s in tpl and the ending \0
    query = (char *) malloc(strlen(host) + strlen(getpage) + strlen(USERAGENT) + strlen(tpl) - 5);
    sprintf(query, tpl, getpage, host, USERAGENT);
    return query;
}

#ifdef USE_LIBCURL

/* 20260712 - patch: TLS-capable replacement for the internet web-service query
 * path (get_socket_connection()+get_page(), always plain HTTP on port 80, no
 * way to reach an HTTPS-only endpoint -- see PATCHES.md for why this matters).
 * Kept separate from the functions above rather than modifying them in place,
 * so the original socket-based implementation is left untouched and available.
 */

struct http_fetch_buffer {
    char *data;
    size_t len;
};

static size_t http_fetch_write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct http_fetch_buffer *mem = (struct http_fetch_buffer *) userp;
    char *ptr = realloc(mem->data, mem->len + realsize + 1);
    if (ptr == NULL)
        return (0); // out of memory; libcurl treats a short write as an error
    mem->data = ptr;
    memcpy(&(mem->data[mem->len]), contents, realsize);
    mem->len += realsize;
    mem->data[mem->len] = '\0';
    return (realsize);
}

// single attempt at one URL; returns NULL on any failure (bad connect, TLS error, timeout, empty response)
static char *http_fetch_try(const char *url, int *ppage_length) {

    CURL *curl = curl_easy_init();
    if (curl == NULL)
        return (NULL);

    struct http_fetch_buffer buf;
    buf.data = (char *) malloc(1);
    buf.data[0] = '\0';
    buf.len = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_fetch_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buf);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // merge headers into the body output, matching the historical get_page() contract
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow http->https redirects
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // matches the original socket SO_SNDTIMEO/SO_RCVTIMEO of 10s
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || buf.len == 0) {
        if (VERBOSE) fprintf(stderr, "net.http_fetch: %s failed: %s\n", url, curl_easy_strerror(res));
        free(buf.data);
        return (NULL);
    }

    *ppage_length = (int) buf.len;
    return (buf.data);
}

char *http_fetch(char *host, char *page, int *ppage_length) {

    char *getpage = page;
    if (getpage[0] == '/')
        getpage = getpage + 1;

    char url[2048];
    char *page_contents;

    snprintf(url, sizeof (url), "https://%s/%s", host, getpage);
    page_contents = http_fetch_try(url, ppage_length);
    if (page_contents != NULL)
        return (page_contents);

    if (VERBOSE) fprintf(stdout, "net.http_fetch: HTTPS failed for %s, retrying over HTTP\n", url);
    snprintf(url, sizeof (url), "http://%s/%s", host, getpage);
    page_contents = http_fetch_try(url, ppage_length);

    return (page_contents); // NULL if this also failed

}

#endif // USE_LIBCURL


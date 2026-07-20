// adapted from http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client

#include <stdio.h>
#include <string.h>

#include "net_lib.h"

void usage();

#define PAGE "/"

int main(int argc, char **argv) {

    //struct sockaddr_in *remote;
#ifndef USE_LIBCURL
    int sock;
#endif
    char *host;
    char *page;

    if (argc == 1) {
        usage();
        exit(2);
    }
    host = argv[1];
    if (argc > 2) {
        page = argv[2];
    } else {
        page = PAGE;
    }

#ifdef USE_LIBCURL
    // 20260720 - patch: TLS-capable fetch (see PATCHES.md); tries HTTPS first,
    // falling back to HTTP, instead of the plain-port-80-only path below.
    int page_length = 0;
    char *page_contents = http_fetch(host, page, &page_length);
    if (page_contents == NULL)
        return (1);
    fprintf(stdout, "%s\n", page_contents);
    free(page_contents);
#else
    //sock = get_socket_connection(host, &remote);
    sock = get_socket_connection(host, "http");
    if (sock < 0)
        return (sock);

    int page_length = 0;
    char *page_contents = get_page(host, sock, page, &page_length);
    fprintf(stdout, "%s\n", page_contents);
    free(page_contents);

    //free(remote);
    close(sock);
#endif

    return (0);

}

void usage() {
    fprintf(stderr, "USAGE: htmlget host [page]\n\
\thost: the website hostname. ex: coding.debuntu.org\n\
\tpage: the page to retrieve. ex: index.html, default: /\n");
}


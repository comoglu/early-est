/* 
 * File:   net_lib.h
 * Author: anthony
 *
 * Created on February 16, 2010, 3:15 PM
 */

#ifndef _NET_LIB_H
#define	_NET_LIB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>

int get_socket_connection_OLD(char *host, struct sockaddr_in **premote);
int get_socket_connection(char *host, char *servname);
char *get_page(char *host, int sock, char *page, int *ppage_length);
int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host, char *page);

#ifdef USE_LIBCURL
/* 20260712 - patch: fetches host/page over HTTPS (falling back to HTTP if
 * HTTPS fails), via libcurl. Replaces the get_socket_connection()+get_page()
 * pair for internet web-service queries -- see PATCHES.md. Returns the full
 * raw response text (headers + body), matching the historical get_page()
 * output contract, so existing callers/parsers need no changes. Returned
 * buffer must be free()'d by the caller; returns NULL on failure. */
char *http_fetch(char *host, char *page, int *ppage_length);
#endif



#ifdef	__cplusplus
}
#endif

#endif	/* _NET_LIB_H */


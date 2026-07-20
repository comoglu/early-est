/* vim:set ft=c ts=2 sw=2 sts=2 et cindent: */
/*
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MIT
 *
 * Portions created by Alan Antonuk are Copyright (c) 2012-2013
 * Alan Antonuk. All Rights Reserved.
 *
 * Portions created by VMware are Copyright (c) 2007-2012 VMware, Inc.
 * All Rights Reserved.
 *
 * Portions created by Tony Garnock-Jones are Copyright (c) 2009-2010
 * VMware, Inc. and Tony Garnock-Jones. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ***** END LICENSE BLOCK *****
 */

/* amqp_listen_test.c
 *
 * 20171221 Anthony Lomax
 *
 * Modified from rabbitmq-c-0.8.0/examples/amqp_listen.c for testing sending a message
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <assert.h>

#include "utils.h"

void amqp_dump(void const *buffer, size_t len);

int main(int argc, char **argv) {


    int status;
    amqp_rpc_reply_t rpc_reply;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;

    amqp_bytes_t queuename;

    if (argc < 8) {
        fprintf(stderr, "Usage: amqp_listen_test host port vhost username password exchange routingkey\n");
        fprintf(stderr, "   Ex: amqp_listen_test localhost 5672 / guest guest test test\n");
        return 1;
    }

    char const *hostname = argv[1];
    int port = atoi(argv[2]);
    char const *vhost = argv[3];
    char const *username = argv[4];
    char const *password = argv[5];
    char const *exchange = argv[6];
    char const *routingkey = argv[7];

    fprintf(stdout, "hostname=%s\n", hostname);
    fprintf(stdout, "port=%d\n", port);
    fprintf(stdout, "vhost=%s\n", vhost);
    fprintf(stdout, "username=%s\n", username);
    fprintf(stdout, "password=%s\n", password);
    fprintf(stdout, "exchange=%s\n", exchange);
    fprintf(stdout, "routingkey=%s\n", routingkey);


    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        fprintf(stderr, "ERROR: creating TCP socket\n");
    }

    status = amqp_socket_open(socket, hostname, port);
    if (status) {
        fprintf(stderr, "ERROR: opening TCP socket: is the rabbitmq-server running?  Try:\n");
        fprintf(stderr, "/usr/local/sbin/rabbitmq-server &\n");
    }

    rpc_reply = amqp_login(conn, vhost, AMQP_DEFAULT_MAX_CHANNELS, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN, username, password);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Logging in: is the rabbitmq-server running?  Try:\n");
        fprintf(stderr, "/usr/local/sbin/rabbitmq-server &\n");
    }
    amqp_channel_open(conn, 1);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Opening channel\n");
    }

    {
        amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
                amqp_empty_table);
        rpc_reply = amqp_get_rpc_reply(conn);
        if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
            fprintf(stderr, "ERROR: Declaring queue\n");
        }
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name\n");
            return 1;
        }
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(routingkey),
            amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Binding queue\n");
    }

    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Consuming\n");
    }

    {
        for (;;) {
            amqp_rpc_reply_t res;
            amqp_envelope_t envelope;

            amqp_maybe_release_buffers(conn);

            res = amqp_consume_message(conn, &envelope, NULL, 0);

            if (AMQP_RESPONSE_NORMAL != res.reply_type) {
                break;
            }

            int VERBOSE = 0;
            if (VERBOSE) {
                printf("----\n");
                printf("Delivery %u, exchange %.*s routingkey %.*s\n",
                        (unsigned) envelope.delivery_tag,
                        (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
                        (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

                if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
                    printf("Content-type: %.*s\n",
                            (int) envelope.message.properties.content_type.len,
                            (char *) envelope.message.properties.content_type.bytes);
                }
                amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
            } else {
                unsigned char *buf = (unsigned char *) envelope.message.body.bytes;
                for (int i = 0; i < envelope.message.body.len; i++) {
                    printf("%c", (unsigned char) buf[i]);
                }
                printf("\n");
            }

            amqp_destroy_envelope(&envelope);
        }
    }

    rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Closing channel\n");
    }
    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: Closing connection\n");
    }
    status = amqp_destroy_connection(conn);
    if (status) {
        fprintf(stderr, "ERROR: Ending connection\n");
    }

    return 0;
}

static void dump_row(long count, int numinrow, int *chs) {
    int i;

    printf("%08lX:", count - numinrow);

    if (numinrow > 0) {
        for (i = 0; i < numinrow; i++) {
            if (i == 8) {
                printf(" :");
            }
            printf(" %02X", chs[i]);
        }
        for (i = numinrow; i < 16; i++) {
            if (i == 8) {
                printf(" :");
            }
            printf("   ");
        }
        printf("  ");
        for (i = 0; i < numinrow; i++) {
            if (isprint(chs[i])) {
                printf("%c", chs[i]);
            } else {
                printf(".");
            }
        }
    }
    printf("\n");
}

static int rows_eq(int *a, int *b) {
    int i;

    for (i = 0; i < 16; i++)
        if (a[i] != b[i]) {
            return 0;
        }

    return 1;
}

void amqp_dump(void const *buffer, size_t len) {
    unsigned char *buf = (unsigned char *) buffer;
    long count = 0;
    int numinrow = 0;
    int chs[16];
    int oldchs[16] = {0};
    int showed_dots = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        int ch = buf[i];

        if (numinrow == 16) {
            int j;

            if (rows_eq(oldchs, chs)) {
                if (!showed_dots) {
                    showed_dots = 1;
                    printf("          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
                }
            } else {
                showed_dots = 0;
                dump_row(count, numinrow, chs);
            }

            for (j = 0; j < 16; j++) {
                oldchs[j] = chs[j];
            }

            numinrow = 0;
        }

        count++;
        chs[numinrow++] = ch;
    }

    dump_row(count, numinrow, chs);

    if (numinrow != 0) {
        printf("%08lX:\n", count);
    }
}


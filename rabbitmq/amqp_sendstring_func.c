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

/* amqp_sendstring_func.c
 *
 * 20171221 Anthony Lomax
 *
 * Modified from rabbitmq-c-0.8.0/examples/amqp_sendstring.c to provide function access to sending a message
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include "utils.h"

static char routingkey_temp[4096];

int amqp_sendstring(char const *hostname, int port, char *vhost, char *username, char *password,
        char const *exchange, char const *exchangetype, char const *routingkey_root, char const *routingkey_topics, char const *messagebody) {

    int status;
    amqp_rpc_reply_t rpc_reply;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;

    // allow NULL defaults for testing
    if (vhost == NULL) {
        strcpy(vhost, "/");
    }
    if (username == NULL) {
        strcpy(username, "guest");
    }
    if (password == NULL) {
        strcpy(password, "guest");
    }

    int ireturn = 0;

    conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): creating TCP socket\n");
        ireturn = -1;
    }

    status = amqp_socket_open(socket, hostname, port);
    if (status) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): opening TCP socket: is the rabbitmq-server running?  Try:\n");
        fprintf(stderr, "/usr/local/sbin/rabbitmq-server &\n");
        ireturn = -1;
    }

    rpc_reply = amqp_login(conn, vhost, AMQP_DEFAULT_MAX_CHANNELS, AMQP_DEFAULT_FRAME_SIZE, 0, AMQP_SASL_METHOD_PLAIN, username, password);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Logging in: is the rabbitmq-server running?  Try:\n");
        fprintf(stderr, "/usr/local/sbin/rabbitmq-server &\n");
        ireturn = -1;
    }
    amqp_channel_open(conn, 1);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Opening channel: lib_err: %s, serv_err: %d %s\n", amqp_error_string2(rpc_reply.library_error), rpc_reply.reply.id, rpc_reply.reply.decoded);
        ireturn = -1;
    }
    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange), amqp_cstring_bytes(exchangetype), 0, 0, 0, 0, amqp_empty_table);
    rpc_reply = amqp_get_rpc_reply(conn);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Declaring exchange: lib_err: %s, serv_err: %d %s\n", amqp_error_string2(rpc_reply.library_error), rpc_reply.reply.id, rpc_reply.reply.decoded);
        ireturn = -1;
    }

    // construct rootingkey  // 20211020 AJL - added
    snprintf(routingkey_temp, sizeof(routingkey_temp), "%s.%s", routingkey_root, routingkey_topics);

    {
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2; /* persistent delivery mode */
        status = amqp_basic_publish(conn,
                1,
                amqp_cstring_bytes(exchange),
                amqp_cstring_bytes(routingkey_temp),
                0,
                0,
                &props,
                amqp_cstring_bytes(messagebody));
        if (status) {
            fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Publishing: %d\n", status);
            ireturn = -1;
        }
    }

    rpc_reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Closing channel: lib_err: %s, serv_err: %d %s\n", amqp_error_string2(rpc_reply.library_error), rpc_reply.reply.id, rpc_reply.reply.decoded);
        ireturn = -1;
    }
    rpc_reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    if (rpc_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Closing connection: lib_err: %s, serv_err: %d %s\n", amqp_error_string2(rpc_reply.library_error), rpc_reply.reply.id, rpc_reply.reply.decoded);
        ireturn = -1;
    }
    status = amqp_destroy_connection(conn);
    if (status) {
        fprintf(stderr, "ERROR: rabbitmq.amqp_sendstring(): Ending connection: %d\n", status);
        ireturn = -1;
    }

    return ireturn;

}

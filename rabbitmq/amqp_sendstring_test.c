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

#include "rabbitmq.h"

int main(int argc, char **argv) {

    if (argc < 10) {
        fprintf(stderr, "Usage: amqp_sendstring_test host port vhost username password exchange exchangetype routingkey messagebody\n");
        fprintf(stderr, "   Ex: amqp_sendstring_test localhost 5672  / guest guest test topic test \"Hello World\"\n");
        return 1;
    }

    char const *hostname = argv[1];
    int port = atoi(argv[2]);
    char *vhost = argv[3];
    char *username = argv[4];
    char *password = argv[5];
    char const *exchange = argv[6];
    char const *exchangetype = argv[7];
    char const *routingkey = argv[8];
    char const *messagebody = argv[9];

    amqp_sendstring(hostname, port, vhost, username, password, exchange, exchangetype, routingkey, "", messagebody);

}


/*
 * This file is part of the Anthony Lomax Java Library.
 *
 * Copyright (C) 2017 Anthony Lomax <anthony@alomax.net www.alomax.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * File:   rabbitmq.h
 * Author: Anthony Lomax
 *
 * Created on 21 December 2017, 17:43
 */

#ifndef RABBITMQ_H
#define RABBITMQ_H

#ifdef __cplusplus
extern "C" {
#endif

// 20211020 AJL - added routing key topics
#define RMQ_ROUTINGKEY_TOPICS_PICKS "earlyest.realtime.picks"


int amqp_sendstring(char const *hostname, int port, char *vhost, char *username, char *password,
        char const *exchange, char const *exchangetype, char const *routingkey_root, char const *routingkey_topics, char const *messagebody);

#ifdef __cplusplus
}
#endif

#endif /* RABBITMQ_H */


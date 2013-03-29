/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <libfreecwmp.h>

#include <amqp.h>
#include <amqp_framing.h>

#include "config.h"

int amqp_notify(const cwmp_str_t *msg)
{
	amqp_connection_state_t conn;
	amqp_rpc_reply_t reply;
	int sockfd;
	int rc;

	conn = amqp_new_connection();

	sockfd = amqp_open_socket(amqp.host, amqp.port);
	if (sockfd < 0) {
		return -1;
	}

	amqp_set_sockfd(conn, sockfd);

	reply = amqp_login(conn, amqp.virtual_host, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp.user, amqp.pass);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_channel_open(conn, 1);

	reply = amqp_get_rpc_reply(conn);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_bytes_t b_exchange = {
		.bytes	= amqp_exchange.broadcast.data,
		.len	= amqp_exchange.broadcast.len
	};

	amqp_bytes_t b_msg = {
		.bytes	= msg->data,
		.len	= msg->len
	};

	amqp_exchange_declare(conn, 1, b_exchange, amqp_cstring_bytes("fanout"), 0, 1, amqp_empty_table);
	reply = amqp_get_rpc_reply(conn);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	rc = amqp_basic_publish(conn, 1, b_exchange, amqp_empty_bytes, 0, 0, NULL, b_msg);
	if (rc < 0) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn);

	return 0;
}

int amqp_fetch_pending(cwmp_str_t *queue, cwmp_str_t *msg)
{
	amqp_connection_state_t conn;
	amqp_rpc_reply_t reply;
	int sockfd;
	int rc;

	conn = amqp_new_connection();

	sockfd = amqp_open_socket(amqp.host, amqp.port);
	if (sockfd < 0) {
		return -1;
	}

	amqp_set_sockfd(conn, sockfd);

	reply = amqp_login(conn, amqp.virtual_host, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, amqp.user, amqp.pass);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_channel_open(conn, 1);

	reply = amqp_get_rpc_reply(conn);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_bytes_t provisioning_exchange = {
		.bytes	= amqp_exchange.provisioning.data,
		.len	= amqp_exchange.provisioning.len
	};

	amqp_exchange_declare(conn, 1, provisioning_exchange, amqp_cstring_bytes("topic"), 0, 1, amqp_empty_table);
	reply = amqp_get_rpc_reply(conn);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_bytes_t provisioning_queue = {
		.bytes	= queue->data,
		.len	= queue->len
	};

	reply = amqp_basic_get(conn, 1, provisioning_queue, 1);

	/* on exception just assume that there is no queue */
	if (reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return 0;
	}

	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	if (reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return 0;
	}

	if (reply.reply.id != AMQP_BASIC_GET_OK_METHOD) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	amqp_basic_get_ok_t *ok;
	ok = (amqp_basic_get_ok_t *) reply.reply.decoded;

	amqp_frame_t frame;

	rc = amqp_simple_wait_frame(conn, &frame);
	if (rc != 0) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	if (frame.frame_type != AMQP_FRAME_HEADER) {
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}

	size_t body_len = frame.payload.properties.body_size;

	msg->data = (u_char *) calloc(1, (body_len + 1) * sizeof(u_char));
	if (msg->data == NULL) {
		fprintf(stderr, "couldn't allocate memory for incoming AMQP message\n");
		amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(conn);
		return -1;
	}
	msg->len = body_len;

	while (body_len) {
		rc = amqp_simple_wait_frame(conn, &frame);
		if (rc != 0) {
			amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
			amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(conn);
			return -1;
		}

		if (frame.frame_type != AMQP_FRAME_BODY) {
			amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
			amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(conn);
			return -1;
		}

		/* we got whole message in the first chunk */
		if (frame.payload.body_fragment.len == (msg->len)) {
			if (snprintf(msg->data, msg->len + 1, "%.*s",
				     frame.payload.body_fragment.len,
				     frame.payload.body_fragment.bytes) == -1)
			{
				amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
				amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
				amqp_destroy_connection(conn);
				return -1;
			}
			goto done;
		}

		char *c = strdup(msg->data);
		if (snprintf(msg->data, msg->len + 1, "%s%.*s",
			     (c != NULL) ? c : "",
			     frame.payload.body_fragment.len,
			     frame.payload.body_fragment.bytes) == -1)
		{
			amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
			amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
			amqp_destroy_connection(conn);
			return -1;
		}
		body_len -= frame.payload.body_fragment.len;
	}

done:
	amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(conn);

	return 0;
}

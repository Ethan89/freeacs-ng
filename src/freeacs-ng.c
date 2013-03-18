/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <json.h>
#include <scgi.h>

#include "freeacs-ng.h"

#include "amqp.h"
#include "config.h"
#include "xml.h"


static struct event_base *base;
static struct evconnlistener *listener;

static struct connection_t *prepare_connection()
{
	struct scgi_limits limits = {
		.max_head_size =  4*1024,
		.max_body_size = 60*1024,
	};

	/* allocate memory for bookkeeping */
	struct connection_t *connection = calloc(1, sizeof(struct connection_t));
	if (connection == NULL) {
		fprintf(stderr, "Failed to allocate connection object.\n");
		return NULL;
	}

	connection->head = NULL;
	connection->body = NULL;

	/* prepare buffer for HTTP headers */
	connection->head = evbuffer_new();
	if (connection->head == NULL) {
		fprintf(stderr, "Failed to allocate memory for HTTP headers.\n");
		goto error;
	}

	if (evbuffer_expand(connection->head, limits.max_head_size) != 0) {
		fprintf(stderr, "Failed to expand memory for HTTP headers.\n");
		goto error;
	}

	/* prepare buffer for HTTP body */
	connection->body = evbuffer_new();
	if (connection->body == NULL) {
		fprintf(stderr, "Failed to allocate memory for HTTP body.\n");
		goto error;
	}

	if (evbuffer_expand(connection->head, limits.max_body_size) != 0) {
		fprintf(stderr, "Failed to expand memory for HTTP body.\n");
		goto error;
	}

	/* prepare the SCGI connection parser */
	scgi_setup(&limits, &connection->parser);

	/* register SCGI parser callbacks */
	connection->parser.accept_field = accept_field;
	connection->parser.finish_field = finish_field;
	connection->parser.accept_value = accept_value;
	connection->parser.finish_value = finish_value;
	connection->parser.finish_head = finish_head;
	connection->parser.accept_body = accept_body;

	/* make sure SCGI callbacks can access the connection object */
	connection->parser.object = connection;

	/* headers initialization */
	connection->header[_REMOTE_ADDR].data = NULL;
	connection->header[_CONTENT_LENGTH].data = NULL;
	connection->header[_REMOTE_ADDR].data = NULL;

	/* request status initialization */
	connection->request_status = REQUEST_RECEIVED;

	/* will be initialized later */
	connection->stream = 0;

	fprintf(stderr, "Connection object ready.\n");

	return connection;

error:
	if (connection->head) evbuffer_free(connection->head);
	if (connection->body) evbuffer_free(connection->body);
	free(connection);
	return NULL;
}

void release_connection(struct connection_t *connection, bool error)
{
	if (!error)
		fprintf(stderr, "Dropping connection.\n");
	else
		fprintf(stderr, "Error occurred, dropping connection.\n");

	/* close socket */
	bufferevent_free(connection->stream);

	/* release buffers */
	evbuffer_free(connection->head);
	evbuffer_free(connection->body);

	/* release bookkeeping data */
	free(connection);

#ifdef DUMMY_MODE
	/* stop the event loop */
	(void) event_base_loopbreak(base);
#endif /* DUMMY_MODE */
}

/* buffer header name inside connection object */
static void accept_field(struct scgi_parser *parser,
			 const char *data, size_t size)
{
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->head, data, size);
}

/* null-terminate the HTTP header name */
static void finish_field(struct scgi_parser *parser)
{
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->head, "\0", 1);
}

/* buffer header data inside connection object */
static void accept_value(struct scgi_parser *parser,
			 const char *data, size_t size)
{
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->head, data, size);
}

/* null-terminate the HTTP header data */
static void finish_value(struct scgi_parser *parser)
{
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->head, "\0", 1);
}

/* double-null terminate the HTTP headers */
static void finish_head(struct scgi_parser *parser)
{
	struct evbuffer_ptr here;
	struct evbuffer_ptr next;
	struct evbuffer_iovec name;
	struct evbuffer_iovec data;
	struct connection_t *connection = parser->object;
	struct evbuffer *output;

	fprintf(stderr, "Headers done.\n");

	/* iterate over HTTP headers */
	memset(&here, 0, sizeof(here));
	evbuffer_ptr_set(connection->head, &here, 0, EVBUFFER_PTR_SET);
	next = evbuffer_search(connection->head, "\0", 1, &here);
	while (next.pos >= 0) {
		/* locate the header name */
		evbuffer_peek(connection->head, next.pos - here.pos, &here, &name, 1);
		name.iov_len = next.pos - here.pos;

		/* locate the header data */
		evbuffer_ptr_set(connection->head, &next, 1, EVBUFFER_PTR_ADD);
		here = next;
		next = evbuffer_search(connection->head, "\0", 1, &next);
		evbuffer_peek(connection->head, next.pos - here.pos, &here, &data, 1);
		data.iov_len = next.pos - here.pos;

		if (strcmp((const char *) name.iov_base, CONTENT_LENGTH) == 0) {
			connection->header[_CONTENT_LENGTH].data = (char *) data.iov_base;
			connection->header[_CONTENT_LENGTH].len = strlen((const char *) data.iov_base);
			goto next;
		};

		if (strcmp((const char *) name.iov_base, REQUEST_METHOD) == 0) {
			connection->header[_REQUEST_METHOD].data = (char *) data.iov_base;
			connection->header[_REQUEST_METHOD].len = strlen((const char *) data.iov_base);
			goto next;
		};

		if (strcmp((const char *) name.iov_base, REMOTE_ADDR) == 0) {
			connection->header[_REMOTE_ADDR].data = (char *) data.iov_base;
			connection->header[_REMOTE_ADDR].len = strlen((const char *) data.iov_base);
			goto next;
		};

next:
		/* locate the next header */
		evbuffer_ptr_set(connection->head, &next, 1, EVBUFFER_PTR_ADD);
		here = next;
		next = evbuffer_search(connection->head, "\0", 1, &next);
	}

	/* headers checking */
	char *header = NULL;

	if (connection->header[_REMOTE_ADDR].data == NULL) {
		header = REQUEST_METHOD;
		goto error;
	};

	if (connection->header[_CONTENT_LENGTH].data == NULL) {
		header = CONTENT_LENGTH;
		goto error;
	};

	if (connection->header[_REMOTE_ADDR].data == NULL) {
		header = REMOTE_ADDR;
		goto error;
	};

	/* needed headers have been found */
	connection->request_status = REQUEST_HEAD_APPROVED;

	return;

error:
	/* FIXME: throw internal server error */
	fprintf(stderr, "%s header is missing\n", header);
	output = bufferevent_get_output(connection->stream);
	evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
	connection->request_status = REQUEST_FINISHED;
	return;
}

/* buffer body data inside connection object */
static size_t accept_body(struct scgi_parser *parser,
			  const char *data, size_t size)
{
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->body, data, size);

	return size;
}

static void finish_body(struct scgi_parser *parser)
{
	/* null-terminate the body data */
	struct connection_t *connection = parser->object;
	evbuffer_add(connection->body, "\0", 1);

	/* get the body */
	struct evbuffer_ptr here;
	struct evbuffer_ptr next;
	struct evbuffer_iovec msg;

	memset(&here, 0, sizeof(here));
	evbuffer_ptr_set(connection->body, &here, 0, EVBUFFER_PTR_SET);
	next = evbuffer_search(connection->body, "\0", 1, &here);
	if (next.pos < 0) {
		connection->msg.data = NULL;
		connection->msg.len = 0;
	} else {
		evbuffer_peek(connection->body, next.pos - here.pos, &here, &msg, 1);
		connection->msg.data = msg.iov_base;
		connection->msg.len = next.pos - here.pos;
	}

	/* finish by generating response */
	send_response(parser);
}

static void send_response(struct scgi_parser *parser)
{
	struct connection_t *connection = parser->object;
	json_object *json_msg_in = NULL;
	json_object *json_msg_out = NULL;
	uintptr_t msg_tag = 0;
	int rc;

	if (connection->request_status == REQUEST_FINISHED) return;

	fprintf(stderr, "Starting response.\n");

	struct evbuffer *output = bufferevent_get_output(connection->stream);

	/* FIXME: ignore everything that is not POST method */
	if (strcmp(connection->header[_REQUEST_METHOD].data, REQUEST_METHOD_POST)) {
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		return;
	}

	json_msg_in = json_object_new_object();
	if (json_msg_in == NULL) {
		fprintf(stderr, "failed to initialize json object\n");
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		goto clean;
	}

	/* initialize xml parser only once */
	lxml2_parser_init();

	/* analyze received message */
	if (atoi(connection->header[_CONTENT_LENGTH].data) != 0
	    && xml_message_analyze(&connection->msg, &msg_tag, &json_msg_in))
	{
		fprintf(stderr, "failed while analyzing xml message\n");
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		goto clean;
	}

	json_object *remote_addr;
	remote_addr = json_object_new_string(connection->header[_REMOTE_ADDR].data);
	json_object_object_add(json_msg_in, "remote_addr", remote_addr);

	if (msg_tag & XML_CWMP_TYPE_INFORM) {
		u_char *c;

		c = (u_char *) json_object_to_json_string(json_msg_in);
		cwmp_str_t msg = {
			.data = c,
			.len = strlen(c)
		};

		rc = amqp_notify(&msg);
		if (rc != 0) {
			fprintf(stderr, "failed to send AMQP notification\n");
		}

		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_200_CONTENT_XML) - 1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_GENERIC_HEAD) - 1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_INFORM_RESPONSE) - 1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_GENERIC_TAIL) - 1);

		goto clean;
	}

	if (atoi(connection->header[_CONTENT_LENGTH].data) == 0
	    || msg_tag & XML_CWMP_TYPE_SET_PARAM_RES)
	{
		cwmp_str_t queue = {
			.data	= amqp_queue.provisioning.data,
			.len	= amqp_queue.provisioning.len
		};

		if (strcmp("@remote_addr", amqp_queue.provisioning.data) == 0)
		{
			queue.data = connection->header[_REMOTE_ADDR].data;
			queue.len = connection->header[_REMOTE_ADDR].len;
		};

		cwmp_str_t msg = {
			.data	= NULL,
			.len	= 0
		};

		rc = amqp_fetch_pending(&queue, &msg);
		if (rc != 0) {
			fprintf(stderr, "failed to fetch AMQP action\n");
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
			goto clean;
		}

		if (msg.data == NULL) {
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
			goto clean;
		}

		json_msg_out = json_tokener_parse(msg.data);

		free(msg.data);
		msg.data = NULL;
		msg.len = 0;

		if (json_msg_out == NULL) {
			fprintf(stderr, "failed to parse json data\n");
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
			goto clean;
		}

		rc = xml_message_create(&msg, json_msg_out);
		if (rc == 0) {
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_200_CONTENT_XML) - 1);
			/* FIXME: ugly hack to remove xml node from the xml message */
			int trim = strlen(XML_PROLOG) - 1;
			evbuffer_add(output, msg.data + trim, msg.len - trim);
		} else {
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		}

		free(msg.data);
		msg.data = NULL;
		msg.len = 0;

		goto clean;
	}

	evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);

clean:
	if (json_msg_in) json_object_put(json_msg_in);
	if (json_msg_out) json_object_put(json_msg_out);
	lxml2_parser_cleanup();
	connection->request_status = REQUEST_FINISHED;
}

static void read_cb(struct bufferevent *stream, void *context)
{
	struct connection_t *connection = context;

	/* extract all data from the buffer event */
	struct evbuffer *input = bufferevent_get_input(stream);
	size_t size = evbuffer_get_length(input);
	char *data = malloc(size);

	if (data == NULL) {
		fprintf(stderr, "Failed to allocate temporary data memory.\n");
		goto error;
	}

	fprintf(stderr, "Reading request data (%d bytes).\n", (int)size);

	if (evbuffer_remove(input, data, size) == -1) {
		fprintf(stderr, "Could not read data from the buffer.\n");
		goto error;
	}

	/*
	 * Feed the input data to the SCGI request parser; all actual
	 * processing is done inside the SCGI callbacks registered by our
	 * application.
	 */
	(void) scgi_consume(&connection->parser, data, size);
	if (connection->parser.error != scgi_error_ok) {
		fprintf(stderr, "SCGI request error: \"%s\".\n", \
			scgi_error_message(connection->parser.error));

		goto error;
	}

	int content_length = -1;

	if (connection->request_status == REQUEST_HEAD_APPROVED) {
		content_length = atoi(connection->header[_CONTENT_LENGTH].data);
	}

	if (connection->parser.state == scgi_parser_body
	    && connection->parser.body_size == content_length)
	{
		finish_body(context);
	}

	free(data);
	return;

error:
	free(data);
	release_connection(connection, true);
}

static void write_cb(struct bufferevent *stream, void *context)
{
	struct connection_t *connection = context;
	struct evbuffer *output = bufferevent_get_output(stream);
	struct evbuffer *input  = bufferevent_get_input(stream);

	if (connection->request_status < REQUEST_HEAD_APPROVED)
		return;

	if (connection->request_status == REQUEST_FINISHED
	    && evbuffer_get_length(output) == 0
	    && evbuffer_get_length(input) == 0)
	{
		fprintf(stderr, "Done with connection.\n");
		release_connection(connection, false);
	}
}

static void echo_event_cb(struct bufferevent *stream,
			  short events, void *context)
{
	struct connection_t *connection = context;

	fprintf(stderr, "Error on socket.\n");

	if (events & BEV_EVENT_ERROR) {
		fprintf(stderr, "Error in bufferevent.\n");
	}

	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		release_connection(connection, true);
	}
}

static void accept_conn_cb(struct evconnlistener *listener,
			   evutil_socket_t socket, struct sockaddr *peer,
			   int size, void *context)
{
	struct bufferevent *stream;

	fprintf(stdout, "Accepting connection.\n");

	/* prepare to read and write over the connected socket */
	stream = bufferevent_socket_new(base, socket, BEV_OPT_CLOSE_ON_FREE);
	if (stream == NULL) {
		fprintf(stderr, "Failed to allocate stream.\n");
		return;
	}

	fprintf(stderr, "Configuring stream.\n");

	/* prepare an SCGI request parser for the new connection */
	struct connection_t *connection = prepare_connection();
	if (connection == NULL) {
		fprintf(stderr, "Failed to allocate connection object.\n");
		bufferevent_free(stream);
		return;
	}

	connection->stream = stream;

	bufferevent_setcb(stream, read_cb, write_cb, echo_event_cb, connection);
	bufferevent_enable(stream, EV_READ|EV_WRITE);

	fprintf(stderr, "Stream ready.\n");
}

static void accept_error_cb(struct evconnlistener *listener, void *context)
{
	const int error = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
			"Shutting down.\n", error, evutil_socket_error_to_string(error));

	/* stop the event loop */
	(void) event_base_loopexit(base, NULL);
}

int main(int argc, char **argv)
{
	config_load();

	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Couldn't create event base.\n");
		return EXIT_FAILURE;
	}

	/* start listening for incomming connections */
	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
			-1, (struct sockaddr*)&scgi.host, sizeof(scgi.host));

	if (listener == NULL) {
		fprintf(stderr, "Couldn't create listener.\n");
		return EXIT_FAILURE;
	}

	/* handle errors with the listening socket (e.g. network goes down) */
	evconnlistener_set_error_cb(listener, accept_error_cb);

	/* process event notifications forever */
	event_base_dispatch(base);

	/* free allocated event loop resources */
	evconnlistener_free(listener);
	event_base_free(base);

	config_exit();

	return EXIT_SUCCESS;
}

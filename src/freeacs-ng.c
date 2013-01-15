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

#include <scgi.h>

#include "freeacs-ng.h"

#include "amqp.h"
#include "config.h"
#include "http.h"
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

	connection->request_status = REQUEST_RECEIVED;

	/* will be initialized later */
	connection->stream = 0;

	/* http specific data */
	connection->http.content_length = 0;
	connection->http.request_method = HTTP_UNKNOWN;

	/* xml specific data */
	connection->doc_in = NULL;

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
	event_base_loopbreak(base);

	/* free allocated event loop resources */
	evconnlistener_free(listener);
	event_base_free(base);

	config_exit();
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

		http_parse_param((const char *)name.iov_base, (const char *)data.iov_base,
				 &connection->http);

		/* locate the next header */
		evbuffer_ptr_set(connection->head, &next, 1, EVBUFFER_PTR_ADD);
		here = next;
		next = evbuffer_search(connection->head, "\0", 1, &next);
	}
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
		connection->msg_in.data = NULL;
		connection->msg_in.len = 0;
	} else {
		evbuffer_peek(connection->body, next.pos - here.pos, &here, &msg, 1);
		connection->msg_in.data = msg.iov_base;
		connection->msg_in.len = next.pos - here.pos;
	}

	/* finish by generating response */
	send_response(parser);
}

static void send_response(struct scgi_parser *parser)
{
	struct connection_t *connection = parser->object;
	int rc;

	fprintf(stderr, "Starting response.\n");

	rc = amqp_notify(&connection->msg_in);
	if (rc != 0) {
		fprintf(stderr, "failed to send AMQP notification.\n");
	}

	struct evbuffer *output = bufferevent_get_output(connection->stream);

	/* FIXME: for now we are just ignoring GET method */
	if (connection->http.request_method == HTTP_GET) {
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		return;
	}

	/* FIXME: for now we are only sending reboot requests */
	if (connection->http.request_method == HTTP_POST
	    && connection->http.content_length == 0)
	{
		cwmp_str_t action = {
			.data	= NULL,
			.len	= 0
		};

		rc = amqp_fetch_pending(&action);
		if (rc != 0) {
			fprintf(stderr, "failed to fetch AMQP action.\n");
		}

		if (action.len) {
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_200_CONTENT_XML) - 1);
			evbuffer_add(output, ARRAY_AND_SIZE(XML_SOAP_ENVELOPE_HEAD) - 1);
			evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_HEAD) - 1);
			evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_BODY_HEAD) - 1);
			evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_CMD_REBOOT) - 1);
			evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_GENERIC_TAIL) - 1);
		} else {
			evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		}

		free(action.data);
		goto clean;
	}

	lxml2_parser_init();

	/* FIXME: for now we are just ignoring case when reading XML failed */
	if (xml_read_message(&connection->doc_in, &connection->msg_in)) {
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		goto clean;
	}

	if (xml_message_tag(connection->doc_in, &connection->msg_tag)) {
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);
		goto clean;
	}

	if (connection->msg_tag & XML_CWMP_TYPE_INFORM) {
		evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_200_CONTENT_XML) - 1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_GENERIC_HEAD) - 1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_INFORM_RESPONSE) -1);
		evbuffer_add(output, ARRAY_AND_SIZE(XML_CWMP_GENERIC_TAIL) - 1);
		goto clean;
	}

	evbuffer_add(output, ARRAY_AND_SIZE(HTTP_HEADER_204 NEWLINE) - 1);

clean:
	if (connection->doc_in) lxml2_doc_free(connection->doc_in);
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

	if (connection->parser.state == scgi_parser_body
	    && connection->parser.body_size == connection->http.content_length)
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

	if (connection->request_status == REQUEST_FINISHED
	    && connection->parser.body_size == connection->http.content_length
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
	event_base_loopexit(base, NULL);

	/* free allocated event loop resources */
	evconnlistener_free(listener);
	event_base_free(base);
	event_base_loopexit(base, NULL);
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

	/* free configuration allocations */
	free(amqp.host);
	free(amqp.user);
	free(amqp.pass);
	free(amqp.virtual_host);
	free(amqp_exchange.broadcast.data);
	free(amqp_exchange.provisioning.data);
	free(amqp_queue.broadcast.data);
	free(amqp_queue.provisioning.data);

	return EXIT_SUCCESS;
}

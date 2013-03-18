/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#ifndef _FREEACS_NG_H__
#define _FREEACS_NG_H__

#include <scgi.h>
#include <libfreecwmp.h>

#define HTTP_UNKNOWN	0x1
#define HTTP_GET	0x2
#define HTTP_POST	0x4

#define HTTP_HEADER_200 \
	"Status: 200 OK" NEWLINE

#define HTTP_HEADER_204 \
	"Status: 204 No Content" NEWLINE

#define HTTP_HEADER_CONTENT_XML \
	"Content-Type: text/xml" NEWLINE

#define HTTP_HEADER_200_CONTENT_XML \
	HTTP_HEADER_200 \
	HTTP_HEADER_CONTENT_XML \
	NEWLINE

#define REQUEST_METHOD		"REQUEST_METHOD"
#define REQUEST_METHOD_POST	"POST"
#define CONTENT_LENGTH		"CONTENT_LENGTH"
#define REMOTE_ADDR		"REMOTE_ADDR"

enum {
	_REQUEST_METHOD,
	_CONTENT_LENGTH,
	_REMOTE_ADDR,
	__HEADER_MAX
};

enum {
	REQUEST_RECEIVED,
	REQUEST_HEAD_APPROVED,
	REQUEST_FINISHED
};

/* bookkeeping for each connection */
struct connection_t
{
	/* SCGI request parser */
	struct scgi_parser parser;

	/* libevent data socket */
	struct bufferevent *stream;

	/* request buffers */
	struct evbuffer *head;
	struct evbuffer *body;

	/* request headers */
	cwmp_str_t header[__HEADER_MAX];

	/* request status */
	uintptr_t request_status;

	/* received message */
	cwmp_str_t msg;
};

/* SCGI callback functions */
static void accept_field(struct scgi_parser *, const char *, size_t);
static void finish_field(struct scgi_parser *);
static void accept_value(struct scgi_parser *, const char *, size_t);
static void finish_value(struct scgi_parser *);
static void finish_head(struct scgi_parser *);
static size_t accept_body(struct scgi_parser *, const char *, size_t);
static void finish_body(struct scgi_parser *);

/* SCGI response */
static void send_response(struct scgi_parser *);

#endif /* _FREEACS_NG_H__ */

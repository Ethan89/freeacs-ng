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

#include <stdint.h>
#include <stdlib.h>

/* supported HTTP methods */
#define HTTP_UNKNOWN	0x1
#define HTTP_GET	0x2
#define HTTP_POST	0x4

/* HTTP related data */
struct http_t
{
	size_t content_length;
	uintptr_t request_method;
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

	/* HTTP related data */
	struct http_t http;
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

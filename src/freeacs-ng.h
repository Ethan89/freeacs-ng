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

#include "http.h"
#include "xml.h"

#define REQUEST_RECEIVED	0x1
#define REQUEST_FINISHED	0x2

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

	/* request status */
	uintptr_t request_status;

	/* HTTP related data */
	struct http_t http;

	/* XML related data */
	cwmp_str_t msg_in;
	uintptr_t msg_tag;
	lxml2_doc *doc_in;
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

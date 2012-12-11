/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "http.h"

__inline void http_parse_param(const char *name, const char *data,
			       struct http_t *http)
{
	fprintf(stderr, "'%s': '%s'\n", name, data);

	if (!strcmp("CONTENT_LENGTH", name)) {
		http->content_length = atoi(data);
		return;
	}

	if (!strcmp ("REQUEST_METHOD", name)) {
		if (!strcmp ("POST", data)) {
			http->request_method = HTTP_POST;
			return;
		}

		if (!strcmp ("GET", data)) {
			http->request_method = HTTP_GET;
			return;
		}

		http->request_method = HTTP_GET;
		return;
	}
}

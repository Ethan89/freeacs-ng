/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#ifndef _FREEACS_NG_HTTP_H__
#define _FREEACS_NG_HTTP_H__

#include <stdint.h>
#include <stdlib.h>

/* supported HTTP methods */
#define HTTP_UNKNOWN    0x1
#define HTTP_GET        0x2
#define HTTP_POST       0x4

struct http_t
{
        size_t content_length;
        uintptr_t request_method;
};

__inline void http_parse_param(const char *, const char *, struct http_t *);

#endif /* _FREEACS_NG_HTTP_H__ */

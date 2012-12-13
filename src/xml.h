/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#ifndef _FREEACS_NG_XML_H__
#define _FREEACS_NG_XML_H__

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libfreecwmp.h>

/*
 * rename libxml2 data types
 */
typedef xmlDoc lxml2_doc;
typedef xmlXPathContext lxml2_xpath_ctx;
typedef xmlXPathObject lxml2_xpath_obj;
typedef xmlNode lxml2_node;

/*
 * rename libxml2 functions
 */
#define lxml2_doc_free(a) (xmlFreeDoc(a));
#define lxml2_parser_init() (xmlInitParser());
#define lxml2_parser_cleanup() (xmlCleanupParser());
#define lxml2_xpath_new_ctx(a) (xmlXPathNewContext(a));
#define lxml2_xpath_eval_expr(a, b) (xmlXPathEvalExpression(a, b));
#define lxml2_xpath_free_ctx(a) (xmlXPathFreeContext(a));
#define lxml2_xpath_free_obj(a) (xmlXPathFreeObject(a));
#define lxml2_mem_read(a, b, c, d, e) (xmlReadMemory(a, b, c, d, e));
#define lxml2_node_get_content(a) (xmlNodeGetContent(a));

int xml_read_message(lxml2_doc **, const cwmp_str_t *);

#endif /* _FREEACS_NG_XML_H__ */

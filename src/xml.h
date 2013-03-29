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

#include <json.h>
#include <libfreecwmp.h>

/*
 * rename libxml2 data types
 */
typedef xmlDoc lxml2_doc;
typedef xmlXPathContext lxml2_xpath_ctx;
typedef xmlXPathObject lxml2_xpath_obj;
typedef xmlNode lxml2_node;
typedef xmlNs lxml2_ns;

/*
 * rename libxml2 functions
 */
#define lxml2_doc_free(a) (xmlFreeDoc(a));
#define lxml2_ns_free(a) (xmlFreeNs(a));
#define lxml2_parser_init() (xmlInitParser());
#define lxml2_parser_cleanup() (xmlCleanupParser());
#define lxml2_xpath_new_ctx(a) (xmlXPathNewContext(a));
#define lxml2_xpath_eval_expr(a, b) (xmlXPathEvalExpression(a, b));
#define lxml2_xpath_free_ctx(a) (xmlXPathFreeContext(a));
#define lxml2_xpath_free_obj(a) (xmlXPathFreeObject(a));
#define lxml2_xpath_register_ns(a, b, c) (xmlXPathRegisterNs(a, b, c));
#define lxml2_mem_read(a, b, c, d, e) (xmlReadMemory(a, b, c, d, e));
#define lxml2_doc_dump_memory(a, b ,c) (xmlDocDumpMemory(a, b, c));
#define lxml2_node_get_content(a) (xmlNodeGetContent(a));
#define lxml2_new_doc_raw_node(a, b ,c, d) (xmlNewDocRawNode(a, b, c, d));
#define lxml2_new_child(a, b ,c, d) (xmlNewChild(a, b, c, d));
#define lxml2_new_ns(a, b ,c) (xmlNewNs(a, b, c));
#define lxml2_set_prop(a, b ,c) (xmlSetProp(a, b, c));

int xml_message_analyze(cwmp_str_t *, uintptr_t *, json_object **);
int xml_message_create(cwmp_str_t *, json_object *);

#endif /* _FREEACS_NG_XML_H__ */

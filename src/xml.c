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

#include <libfreecwmp.h>

#include "xml.h"

int xml_read_message(lxml2_doc **doc, const cwmp_str_t *msg)
{
	*doc = lxml2_mem_read(msg->data, msg->len, NULL, NULL,
			     XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (!(*doc)) return -1;

	return 0;
}

int xml_message_tag(lxml2_doc *doc, uintptr_t *tag)
{
	lxml2_xpath_ctx *xpath_ctx;
	lxml2_xpath_obj *xpath_obj;
	lxml2_node *busy_node;
	u_char *xpath_expr;
	int rc;

	*tag = XML_CWMP_NONE;

	xpath_ctx = lxml2_xpath_new_ctx(doc);
	if(xpath_ctx == NULL) {
		return -1;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "soap_env",
				     "http://schemas.xmlsoap.org/soap/envelope/");
	if (rc == -1) {
		lxml2_xpath_free_ctx(xpath_ctx);
		fprintf(stderr, "could not register namespace\n");
		return -1;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_0",
				     CWMP_VERSION_1_0);
	if (rc == -1) {
		lxml2_xpath_free_ctx(xpath_ctx);
		fprintf(stderr, "could not register namespace\n");
		return -1;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_1",
				     CWMP_VERSION_1_1);
	if (rc == -1) {
		lxml2_xpath_free_ctx(xpath_ctx);
		fprintf(stderr, "could not register namespace\n");
		return -1;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_2",
				     CWMP_VERSION_1_2);
	if (rc == -1) {
		lxml2_xpath_free_ctx(xpath_ctx);
		fprintf(stderr, "could not register namespace\n");
		return -1;
	}

	/* CWMP-1-0 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_0:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		lxml2_xpath_free_ctx(xpath_ctx);
		return -1;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_0;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	xmlXPathFreeObject(xpath_obj);

	/* CWMP-1-1 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_1:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		lxml2_xpath_free_ctx(xpath_ctx);
		return -1;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_1;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	xmlXPathFreeObject(xpath_obj);

	/* CWMP-1-2 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_2:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		lxml2_xpath_free_ctx(xpath_ctx);
		return -1;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_2;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	xmlXPathFreeObject(xpath_obj);

	xmlXPathFreeContext(xpath_ctx);

	if (*tag == XML_CWMP_NONE) *tag = XML_CWMP_TYPE_UNKNOWN;

	return 0;
}

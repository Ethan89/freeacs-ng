/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json.h>
#include <libfreecwmp.h>

#include "xml.h"

#include "freeacs-ng.h"

int xml_message_analyze(cwmp_str_t *msg, uintptr_t *tag, json_object **json_obj)
{
	lxml2_doc *doc;
	lxml2_xpath_ctx *xpath_ctx;
	lxml2_xpath_obj *xpath_obj;
	u_char *xpath_expr;
	int rc;

	*tag = XML_CWMP_NONE;

	doc = lxml2_mem_read(msg->data, msg->len, NULL, NULL,
			     XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == NULL) {
		fprintf(stderr, "could not read the xml message\n");
		goto error;
	}

	xpath_ctx = lxml2_xpath_new_ctx(doc);
	if (xpath_ctx == NULL) {
		fprintf(stderr, "could not create xpath context\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "soap_env",
				     "http://schemas.xmlsoap.org/soap/envelope/");
	if (rc == -1) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_0",
				     CWMP_VERSION_1_0);
	if (rc == -1) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_1",
				     CWMP_VERSION_1_1);
	if (rc == -1) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx,
				     "cwmp_1_2",
				     CWMP_VERSION_1_2);
	if (rc == -1) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	/* CWMP-1-0 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_0:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_0;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	/* CWMP-1-1 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_1:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_1;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	/* CWMP-1-2 inform message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_2:Inform";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_2;
			*tag |= XML_CWMP_TYPE_INFORM;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	/* CWMP-1-0 set parameter response message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_0:SetParameterValuesResponse";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_0;
			*tag |= XML_CWMP_TYPE_SET_PARAM_RES;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	/* CWMP-1-1 set parameter response message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_1:SetParameterValuesResponse";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_1;
			*tag |= XML_CWMP_TYPE_SET_PARAM_RES;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	/* CWMP-1-2 set parameter response message */
	xpath_expr = "/soap_env:Envelope/soap_env:Body/cwmp_1_2:SetParameterValuesResponse";
	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			*tag |= XML_CWMP_VERSION_1_2;
			*tag |= XML_CWMP_TYPE_SET_PARAM_RES;
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	if (*tag & XML_CWMP_VERSION_1_0) xpath_expr = "//cwmp_1_0:ID";
	if (*tag & XML_CWMP_VERSION_1_1) xpath_expr = "//cwmp_1_1:ID";
	if (*tag & XML_CWMP_VERSION_1_2) xpath_expr = "//cwmp_1_2:ID";

	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			u_char *id = lxml2_node_get_content(xpath_obj->nodesetval->nodeTab[i]);
			json_object_object_add(*json_obj, "id", json_object_new_string((char *) id));
			free(id);
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	if (*tag & XML_CWMP_TYPE_INFORM) {
		xpath_expr = "//EventCode";

		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not create xpath object\n");
			goto error;
		}

		json_object *json_events = json_object_new_array();
		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				u_char *e1 = lxml2_node_get_content(xpath_obj->nodesetval->nodeTab[i]);
				u_char *e2 = lfc_str_event_code(lfc_int_event_code((char *) e1), 1);
				json_object_array_add(json_events, json_object_new_string(e2));
				free(e1);
			}
		}
		lxml2_xpath_free_obj(xpath_obj);
		json_object_object_add(*json_obj, "events", json_events);

		xpath_expr = "//ParameterList/ParameterValueStruct/Name";
		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not create xpath object\n");
			goto error;
		}

		json_object *json_parameters = json_object_new_object();

		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				u_char *parameter = lxml2_node_get_content(xpath_obj->nodesetval->nodeTab[i]);
				lxml2_node *sibling = xmlNextElementSibling(xpath_obj->nodesetval->nodeTab[i]);
				u_char *value = lxml2_node_get_content(sibling);
				json_object_object_add(json_parameters, parameter, json_object_new_string(value));
				free(parameter);
				free(value);
			}
		}
		lxml2_xpath_free_obj(xpath_obj);

		json_object_object_add(*json_obj, "type", json_object_new_string("inform"));
		json_object_object_add(*json_obj, "parameters", json_parameters);
	}

	if (*tag & XML_CWMP_VERSION_1_0) xpath_expr = "//cwmp_1_0:ID";
	if (*tag & XML_CWMP_VERSION_1_1) xpath_expr = "//cwmp_1_1:ID";
	if (*tag & XML_CWMP_VERSION_1_2) xpath_expr = "//cwmp_1_2:ID";

	xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		fprintf(stderr, "could not create xpath object\n");
		goto error;
	}

	for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
		if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
			u_char *id = lxml2_node_get_content(xpath_obj->nodesetval->nodeTab[i]);
			free(id);
		}
	}
	lxml2_xpath_free_obj(xpath_obj);

	if (*tag & XML_CWMP_TYPE_SET_PARAM_RES) {
		xpath_expr = "//Status";

		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not create xpath object\n");
			goto error;
		}

		u_char *value;
		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				value = lxml2_node_get_content(xpath_obj->nodesetval->nodeTab[i]);
			}
		}
		lxml2_xpath_free_obj(xpath_obj);

		json_object_object_add(*json_obj, "type", json_object_new_string("set_parameter_value_response"));
		if (value) json_object_object_add(*json_obj, "status", json_object_new_string(value));
		free(value);
	}

	lxml2_xpath_free_ctx(xpath_ctx);
	lxml2_doc_free(doc);

	if (*tag == XML_CWMP_NONE) *tag = XML_CWMP_TYPE_UNKNOWN;

	return 0;

error:
	if (xpath_ctx) lxml2_xpath_free_ctx(xpath_ctx);
	if (doc) lxml2_doc_free(doc);
}

int xml_message_create(cwmp_str_t *msg, json_object *json_obj)
{
	if (json_obj == NULL) return -1;

	lxml2_doc *doc = NULL;
	lxml2_ns *ns_soap_env = NULL, *ns_cwmp = NULL;
	lxml2_xpath_ctx *xpath_ctx = NULL;
	lxml2_xpath_obj *xpath_obj = NULL;
	u_char *xpath_expr;
	int rc;

	char *xml_template = \
		XML_PROLOG \
		XML_SOAP_ENVELOPE_HEAD \
		XML_CWMP_HEAD \
		XML_CWMP_BODY_HEAD \
		XML_CWMP_BODY_TAIL \
		XML_SOAP_ENVELOPE_TAIL;

	doc = lxml2_mem_read(xml_template, strlen(xml_template), NULL, NULL,
			     XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (!doc) return -1;

	xpath_ctx = lxml2_xpath_new_ctx(doc);
	if (xpath_ctx == NULL) {
		fprintf(stderr, "could not create xpath context\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx, "soap_env", "http://schemas.xmlsoap.org/soap/envelope/");
	if (rc == -1) {
		fprintf(stderr, "could not register xpath namespace\n");
		goto error;
	}

	rc = lxml2_xpath_register_ns(xpath_ctx, "cwmp_1_0", CWMP_VERSION_1_0);
	if (rc == -1) {
		fprintf(stderr, "could not register xpath namespace\n");
		goto error;
	}

	ns_soap_env = lxml2_new_ns(NULL, "http://schemas.xmlsoap.org/soap/envelope/", "soap_env");
	if (ns_soap_env == NULL) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	ns_cwmp = lxml2_new_ns(NULL, "urn:dslforum-org:cwmp-1-0", "cwmp");
	if (ns_cwmp == NULL) {
		fprintf(stderr, "could not register namespace\n");
		goto error;
	}

	if (json_object_object_get(json_obj, "id")
	    && json_object_get_string(json_object_object_get(json_obj, "id")))
	{
		xpath_expr = "/soap_env:Envelope/soap_env:Header";
		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not evaluate xpath expression\n");
			goto error;
		}

		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				lxml2_node *n = xmlNewChild(xpath_obj->nodesetval->nodeTab[i],
							   ns_cwmp,
							   (u_char *) "ID",
							   (u_char *) json_object_get_string(json_object_object_get(json_obj, "id")));
				(void) lxml2_set_prop(n, (u_char *) "soap_env:mustUnderstand", (u_char *) "1");
			}
		}
		lxml2_xpath_free_obj(xpath_obj);
	}

	if (json_object_object_get(json_obj, "set_parameter_values") != NULL) {
		json_object *j = json_object_object_get(json_obj, "set_parameter_values");

		xpath_expr = "/soap_env:Envelope/soap_env:Body";
		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not evaluate xpath expression\n");
			goto error;
		}

		lxml2_node *node_paramaters, *n;
		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				n = lxml2_new_child(xpath_obj->nodesetval->nodeTab[i],
						    ns_cwmp,
						    (u_char *) "SetParameterValues", NULL);
				if (!n) goto error;

				node_paramaters = lxml2_new_child(n, NULL, (u_char *) "ParameterList", NULL);
				if (!node_paramaters) goto error;
				node_paramaters->ns = NULL;

				n = lxml2_new_child(n, NULL, (u_char *) "ParameterKey", NULL);
				if (!n) goto error;
				n->ns = NULL;
			}
		}
		lxml2_xpath_free_obj(xpath_obj);

		u_char *parameter = NULL, *value = NULL;
		int counter = 0;
		json_object_object_foreach(j, key, val) {
			parameter = (u_char *) key;
			value = (u_char *) json_object_get_string(val);

			lxml2_node *n = lxml2_new_child(node_paramaters, NULL, (u_char *) "ParameterValueStruct", NULL);
			if (!n) goto error;

			(void) lxml2_new_child(n, NULL, (u_char *) "Name", parameter);
			(void) lxml2_new_child(n, NULL, (u_char *) "Value", value);

			counter++;
		}

		char *c;
		if (asprintf(&c, "cwmp:ParameterValueStruct[%d]", counter) == -1) {
		        goto error;
		}

		(void) lxml2_set_prop(node_paramaters, (u_char *) "soap_enc:arrayType", (u_char *) c);
		free(c);
		goto done;
	}

	if (json_object_object_get(json_obj, "download") != NULL) {
		json_object *j = json_object_object_get(json_obj, "download");

		xpath_expr = "/soap_env:Envelope/soap_env:Body";
		xpath_obj = lxml2_xpath_eval_expr(xpath_expr, xpath_ctx);
		if (xpath_obj == NULL) {
			fprintf(stderr, "could not evaluate xpath expression\n");
			goto error;
		}

		u_char *value;
		lxml2_node *node_download, *n;
		for(int i = 0; i < ((xpath_obj->nodesetval) ? xpath_obj->nodesetval->nodeNr : 0); ++i) {
			if(xpath_obj->nodesetval->nodeTab[i]->type == XML_ELEMENT_NODE) {
				node_download = lxml2_new_child(xpath_obj->nodesetval->nodeTab[i],
								ns_cwmp,
								(u_char *) "Download", NULL);
				if (!node_download) goto error;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "command_key"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "CommandKey", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "file_type"));
				if (strcmp(value, "firmware_upgrade") == 0) {
					value = "1 Firmware Upgrade Image";
				} else {
					goto error;
				}
				n = lxml2_new_child(node_download, NULL, (u_char *) "FileType", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "url"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "URL", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "username"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "Username", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "password"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "Password", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "file_size"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "FileSize", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "target_file_name"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "TargetFileName", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "delay_seconds"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "DelaySeconds", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "success_url"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "SuccessURL", value);
				if (!n) goto error;
				n->ns = NULL;

				value = (u_char *) json_object_get_string(json_object_object_get(j, "failure_url"));
				n = lxml2_new_child(node_download, NULL, (u_char *) "FailureURL", value);
				if (!n) goto error;
				n->ns = NULL;
			}
		}
		lxml2_xpath_free_obj(xpath_obj);
		goto done;
	}

done:
	lxml2_doc_dump_memory(doc, &msg->data, &msg->len);

	if (xpath_ctx) lxml2_xpath_free_ctx(xpath_ctx);
	if (ns_soap_env) lxml2_ns_free(ns_soap_env);
	if (ns_cwmp) lxml2_ns_free(ns_cwmp);
	if (doc) lxml2_doc_free(doc);

	return 0;

error:
	if (xpath_obj) lxml2_xpath_free_obj(xpath_obj);
	if (xpath_ctx) lxml2_xpath_free_ctx(xpath_ctx);
	if (ns_soap_env) lxml2_ns_free(ns_soap_env);
	if (ns_cwmp) lxml2_ns_free(ns_cwmp);
	if (doc) lxml2_doc_free(doc);

	return -1;
}

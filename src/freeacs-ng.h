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

#define HTTP_HEADER_401 \
	"Status: 401 Unauthorized" NEWLINE \
	"WWW-Authenticate: Basic realm=\"freeacs-ng\"" NEWLINE

#define HTTP_HEADER_500 \
	"Status: 500 Internal Server Error" NEWLINE

#define HTTP_HEADER_CONTENT_XML \
	"Content-Type: text/xml" NEWLINE

#define HTTP_HEADER_SET_COOKIE_PREFIX \
	"Set-Cookie: "

#define HTTP_HEADER_200_CONTENT_XML \
	HTTP_HEADER_200 \
	HTTP_HEADER_CONTENT_XML

#define XML_CWMP_INITIAL_PROVISIONING \
	XML_CWMP_GENERIC_HEAD \
	"<cwmp:SetParameterValues>" \
	  "<ParameterList soap_enc:arrayType=\"cwmp:ParameterValueStruct[4]\">" \
	    "<ParameterValueStruct>" \
	      "<Name>InternetGatewayDevice.ManagementServer.Username</Name>" \
	      "<Value>freeacs-ng</Value>" \
	    "</ParameterValueStruct>" \
	    "<ParameterValueStruct>" \
	      "<Name>InternetGatewayDevice.ManagementServer.Password</Name>" \
	      "<Value>freeacs-ng</Value>" \
	    "</ParameterValueStruct>" \
	    "<ParameterValueStruct>" \
	      "<Name>InternetGatewayDevice.ManagementServer.PeriodicInformEnable</Name>" \
	      "<Value>1</Value>" \
	    "</ParameterValueStruct>" \
	    "<ParameterValueStruct>" \
	      "<Name>InternetGatewayDevice.ManagementServer.PeriodicInformInterval</Name>" \
	      "<Value>3</Value>" \
	    "</ParameterValueStruct>" \
	  "</ParameterList>" \
	  "<ParameterKey />" \
	"</cwmp:SetParameterValues>" \
	XML_CWMP_GENERIC_TAIL


#define CONTENT_LENGTH		"CONTENT_LENGTH"
#define HTTP_AUTHORIZATION	"HTTP_AUTHORIZATION"
#define HTTP_COOKIE		"HTTP_COOKIE"
#define REQUEST_METHOD		"REQUEST_METHOD"
#define REQUEST_METHOD_POST	"POST"
#define REMOTE_ADDR		"REMOTE_ADDR"

enum {
	_CONTENT_LENGTH,
	_HTTP_AUTHORIZATION,
	_HTTP_COOKIE,
	_REMOTE_ADDR,
	_REQUEST_METHOD,
	__HEADER_MAX
};

#define TAG_EMPTY			0x000

#define REQUEST_RECEIVED		0x001
#define REQUEST_HEAD_APPROVED		0x002
#define REQUEST_FINISHED		0x004

#define XML_CWMP_NONE			0x010
#define XML_CWMP_VERSION_1_0		0x020
#define XML_CWMP_VERSION_1_1		0x040
#define XML_CWMP_VERSION_1_2		0x080
#define XML_CWMP_TYPE_UNKNOWN		0x100
#define XML_CWMP_TYPE_INFORM		0x200
#define XML_CWMP_TYPE_SET_PARAM_RES	0x400
#define XML_CWMP_TYPE_ADD_OBJECT_RES	0x800


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

	/* received message */
	cwmp_str_t msg;

	/* request tag */
	uintptr_t tag;
};

#endif /* _FREEACS_NG_H__ */

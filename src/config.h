/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#ifndef _FREEACS_NG_CONFIG_H__
#define _FREEACS_NG_CONFIG_H__

#include <arpa/inet.h>
#include <sys/queue.h>

#include <libfreecwmp.h>

void config_exit(void);
void config_load(void);

struct scgi_t {
	struct sockaddr_in host;
};

struct amqp_t {
	char *host;
	int port;
	char *user;
	char *pass;
	char *virtual_host;
};

struct amqp_exchange_t {
	cwmp_str_t broadcast;
	cwmp_str_t provisioning;
};

struct amqp_queue_t {
	cwmp_str_t provisioning;
};

LIST_HEAD(authorization_head, authorization_t) auth_head;
 
struct authorization_t {
	cwmp_str_t factory;
	LIST_ENTRY(authorization_t) entries;
} *auth;

extern struct scgi_t scgi;
extern struct amqp_t amqp;
extern struct amqp_exchange_t amqp_exchange;
extern struct amqp_queue_t amqp_queue;

#endif /* _FREEACS_NG_CONFIG_H__ */

/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freeacs-ng@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include <uci.h>

#include "config.h"

static struct uci_context *uci_ctx;
static struct uci_package *uci_freeacs_ng;

struct scgi_t scgi;

static int config_init_scgi()
{
	struct uci_section *s;
	struct uci_element *e1, *e2;
	int counter_section = 0;
	int counter_address = 0;
	int counter_port = 0;

	uci_foreach_element(&uci_freeacs_ng->sections, e1) {
		s = uci_to_section(e1);
		if (strcmp(s->type, "scgi") == 0) {
			counter_section++;
			break;
		}
	}

	if (counter_section == 0) {
		fprintf(stderr, "uci section scgi not found...\n");
		return -1;
	} else if (counter_section > 1) {
		fprintf(stderr, "only one uci scgi section is allowed...\n");
		return -1;
	}

	/* only one scgi section was found */

	memset(&scgi.host, 0, sizeof(scgi.host));
	scgi.host.sin_family = AF_INET;

	uci_foreach_element(&s->options, e1) {
		if (!strcmp((uci_to_option(e1))->e.name, "address")) {
			fprintf(stderr, "freeacs-ng.@scgi[0].address=%s\n", uci_to_option(e1)->v.string);
			scgi.host.sin_addr.s_addr = inet_addr(uci_to_option(e1)->v.string);
			counter_address++;
		}

		if (!strcmp((uci_to_option(e1))->e.name, "port")) {
			if (!atoi((uci_to_option(e1))->v.string)) {
				fprintf(stderr, "in section scgi port has invalid value...\n");
				return -1;
			}
			fprintf(stderr, "freeacs-ng.@scgi[0].port=%s\n", uci_to_option(e1)->v.string);
			scgi.host.sin_port = htons(atoi(uci_to_option(e1)->v.string));
			counter_port++;
		}
	}

	if (counter_address == 0) {
		fprintf(stderr, "uci option address must be defined in scgi section...\n");
		return -1;
	}

	if (counter_port == 0) {
		fprintf(stderr, "uci option port must be defined in scgi section...\n");
		return -1;
	}

	return 0;
}

static struct uci_package *
config_init_package(const char *c)
{
	struct uci_context *ctx = uci_ctx;
	struct uci_package *p = NULL;

	if (!ctx) {
		ctx = uci_alloc_context();
		if (!ctx) return NULL;
		uci_ctx = ctx;

#ifdef DUMMY_MODE
		uci_set_confdir(ctx, "./config");
		uci_set_savedir(ctx, "./config/tmp");
#endif
	} else {
		p = uci_lookup_package(ctx, c);
		if (p)
			uci_unload(ctx, p);
	}

	if (uci_load(ctx, c, &p)) {
		uci_free_context(ctx);
		return NULL;
	}

	return p;
}

void config_exit(void)
{
	uci_free_context(uci_ctx);
}

void config_load(void)
{
	uci_freeacs_ng = config_init_package("freeacs-ng");
	if (!uci_freeacs_ng) goto error;

	if (config_init_scgi()) goto error;

	return;

error:
	fprintf(stderr, "configuration loading failed\n");
	exit(EXIT_FAILURE);
}

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
struct amqp_t amqp;
struct amqp_exchange_t amqp_exchange;
struct amqp_queue_t amqp_queue;

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

static int config_init_amqp()
{
	struct uci_section *s;
	struct uci_element *e1, *e2;
	int counter_section = 0;
	amqp.host = NULL;
	amqp.port = 0;
	amqp.user = NULL;
	amqp.pass = NULL;
	amqp.virtual_host = NULL;

	uci_foreach_element(&uci_freeacs_ng->sections, e1) {
		s = uci_to_section(e1);
		if (strcmp(s->type, "amqp") == 0) {
			counter_section++;
			break;
		}
	}

	if (counter_section == 0) {
		fprintf(stderr, "uci section amqp not found...\n");
		return -1;
	} else if (counter_section > 1) {
		fprintf(stderr, "only one uci amqp section is allowed...\n");
		return -1;
	}

	/* only one amqp section was found */

	uci_foreach_element(&s->options, e1) {
		if (!strcmp((uci_to_option(e1))->e.name, "host")) {
			fprintf(stderr, "freeacs-ng.@amqp[0].host=%s\n", uci_to_option(e1)->v.string);
			amqp.host = strdup(uci_to_option(e1)->v.string);
		}

		if (!strcmp((uci_to_option(e1))->e.name, "port")) {
			if (!atoi((uci_to_option(e1))->v.string)) {
				fprintf(stderr, "in section amqp port has invalid value...\n");
				goto error;
			}
			fprintf(stderr, "freeacs-ng.@amqp[0].port=%s\n", uci_to_option(e1)->v.string);
			amqp.port = atoi(uci_to_option(e1)->v.string);
		}

		if (!strcmp((uci_to_option(e1))->e.name, "user")) {
			fprintf(stderr, "freeacs-ng.@amqp[0].user=%s\n", uci_to_option(e1)->v.string);
			amqp.user = strdup(uci_to_option(e1)->v.string);
		}

		if (!strcmp((uci_to_option(e1))->e.name, "pass")) {
			fprintf(stderr, "freeacs-ng.@amqp[0].pass=%s\n", uci_to_option(e1)->v.string);
			amqp.pass = strdup(uci_to_option(e1)->v.string);
		}

		if (!strcmp((uci_to_option(e1))->e.name, "virtual_host")) {
			fprintf(stderr, "freeacs-ng.@amqp[0].virtual_host=%s\n", uci_to_option(e1)->v.string);
			amqp.virtual_host = strdup(uci_to_option(e1)->v.string);
		}
	}

	if (!amqp.host) {
		fprintf(stderr, "uci option host must be defined in amqp section...\n");
		goto error;
	}

	if (!amqp.port) {
		fprintf(stderr, "uci option port must be defined in scgi section...\n");
		goto error;
	}

	if (!amqp.user) {
		fprintf(stderr, "uci option user must be defined in amqp section...\n");
		goto error;
	}

	if (!amqp.pass) {
		fprintf(stderr, "uci option pass must be defined in amqp section...\n");
		goto error;
	}

	if (!amqp.virtual_host) {
		fprintf(stderr, "uci option virtual_host must be defined in amqp section...\n");
		goto error;
	}

	return 0;

error:
	free(amqp.host);
	free(amqp.user);
	free(amqp.pass);
	free(amqp.virtual_host);
	return -1;
}

static int config_init_amqp_exchange()
{
	struct uci_section *s;
	struct uci_element *e1, *e2;
	int counter_section = 0;
	amqp_exchange.broadcast.data = NULL;
	amqp_exchange.broadcast.len = 0;
	amqp_exchange.provisioning.data = NULL;
	amqp_exchange.provisioning.len = 0;

	uci_foreach_element(&uci_freeacs_ng->sections, e1) {
		s = uci_to_section(e1);
		if (strcmp(s->type, "amqp_exchange") == 0) {
			counter_section++;
			break;
		}
	}

	if (counter_section == 0) {
		fprintf(stderr, "uci section amqp_exchange not found...\n");
		return -1;
	} else if (counter_section > 1) {
		fprintf(stderr, "only one uci amqp_exchange section is allowed...\n");
		return -1;
	}

	/* only one amqp_exchange section was found */

	uci_foreach_element(&s->options, e1) {
		if (!strcmp((uci_to_option(e1))->e.name, "broadcast")) {
			fprintf(stderr, "freeacs-ng.@amqp_exchange[0].broadcast=%s\n", uci_to_option(e1)->v.string);

			amqp_exchange.broadcast.len = strlen(uci_to_option(e1)->v.string);

			/* this *should* not happen but just in case */
			if (amqp_exchange.broadcast.data) free(amqp_exchange.broadcast.data);

			amqp_exchange.broadcast.data = strdup(uci_to_option(e1)->v.string);

			if (!amqp_exchange.broadcast.data) {
				fprintf(stderr, "can't allocate enough memory...\n");
				return -1;
			}
		}

		if (!strcmp((uci_to_option(e1))->e.name, "provisioning")) {
			fprintf(stderr, "freeacs-ng.@amqp_exchange[0].provisioning=%s\n", uci_to_option(e1)->v.string);

			amqp_exchange.provisioning.len = strlen(uci_to_option(e1)->v.string);

			/* this *should* not happen but just in case */
			if (amqp_exchange.provisioning.data) free(amqp_exchange.provisioning.data);

			amqp_exchange.provisioning.data = strdup(uci_to_option(e1)->v.string);

			if (!amqp_exchange.provisioning.data) {
				fprintf(stderr, "can't allocate enough memory...\n");
				return -1;
			}
		}
	}

	if (!amqp_exchange.broadcast.data) {
		fprintf(stderr, "uci option broadcast must be defined in amqp_exchange section...\n");
		return -1;
	}

	if (!amqp_exchange.provisioning.data) {
		fprintf(stderr, "uci option provisioning must be defined in amqp_exchange section...\n");
		return -1;
	}

	return 0;
}

static int config_init_amqp_queue()
{
	struct uci_section *s;
	struct uci_element *e1, *e2;
	int counter_section = 0;
	amqp_queue.broadcast.data = NULL;
	amqp_queue.broadcast.len = 0;
	amqp_queue.provisioning.data = NULL;
	amqp_queue.provisioning.len = 0;

	uci_foreach_element(&uci_freeacs_ng->sections, e1) {
		s = uci_to_section(e1);
		if (strcmp(s->type, "amqp_queue") == 0) {
			counter_section++;
			break;
		}
	}

	if (counter_section == 0) {
		fprintf(stderr, "uci section amqp_queue not found...\n");
		return -1;
	} else if (counter_section > 1) {
		fprintf(stderr, "only one uci amqp_queue section is allowed...\n");
		return -1;
	}

	/* only one amqp_queue section was found */

	uci_foreach_element(&s->options, e1) {
		if (!strcmp((uci_to_option(e1))->e.name, "broadcast")) {
			fprintf(stderr, "freeacs-ng.@amqp_queue[0].broadcast=%s\n", uci_to_option(e1)->v.string);

			amqp_queue.broadcast.len = strlen(uci_to_option(e1)->v.string);

			/* this *should* not happen but just in case */
			if (amqp_queue.broadcast.data) free(amqp_queue.broadcast.data);

			amqp_queue.broadcast.data = strdup(uci_to_option(e1)->v.string);

			if (!amqp_queue.broadcast.data) {
				fprintf(stderr, "can't allocate enough memory...\n");
				return -1;
			}
		}

		if (!strcmp((uci_to_option(e1))->e.name, "provisioning")) {
			fprintf(stderr, "freeacs-ng.@amqp_queue[0].provisioning=%s\n", uci_to_option(e1)->v.string);

			amqp_queue.provisioning.len = strlen(uci_to_option(e1)->v.string);

			/* this *should* not happen but just in case */
			if (amqp_queue.provisioning.data) free(amqp_queue.provisioning.data);

			amqp_queue.provisioning.data = strdup(uci_to_option(e1)->v.string);

			if (!amqp_queue.provisioning.data) {
				fprintf(stderr, "can't allocate enough memory...\n");
				return -1;
			}
		}
	}

	if (!amqp_queue.broadcast.data) {
		fprintf(stderr, "uci option broadcast must be defined in amqp_queue section...\n");
		return -1;
	}

	if (!amqp_queue.provisioning.data) {
		fprintf(stderr, "uci option provisioning must be defined in amqp_queue section...\n");
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
	if (config_init_amqp()) goto error;
	if (config_init_amqp_exchange()) goto error;
	if (config_init_amqp_queue()) goto error;

	return;

error:
	fprintf(stderr, "configuration loading failed\n");
	exit(EXIT_FAILURE);
}

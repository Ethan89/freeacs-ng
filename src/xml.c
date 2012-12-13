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

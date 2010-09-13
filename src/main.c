/* $Id$ */
/* Copyright (c) 2010 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop XMLEditor */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */



#include <unistd.h>
#include <stdio.h>
#include "xmleditor.h"
#include "../config.h"


/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif


/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: xmleditor [file]\n", stderr);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	int o;
	XMLEditor * xmleditor;

	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "")) != -1)
		switch(o)
		{
			default:
				return _usage();
		}
	if(optind != argc && optind + 1 != argc)
		return _usage();
	if((xmleditor = xmleditor_new()) == NULL)
		return 2;
	if(argc - optind == 1)
		xmleditor_open(xmleditor, argv[optind]);
	gtk_main();
	xmleditor_delete(xmleditor);
	return 0;
}

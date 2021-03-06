/*
 * unshare(1) - command-line interface for unshare(2)
 *
 * Copyright (C) 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "nls.h"

#ifndef CLONE_NEWSNS
# define CLONE_NEWNS 0x00020000
#endif
#ifndef CLONE_NEWUTS
# define CLONE_NEWUTS 0x04000000
#endif
#ifndef CLONE_NEWIPC
# define CLONE_NEWIPC 0x08000000
#endif
#ifndef CLONE_NEWNET
# define CLONE_NEWNET 0x40000000
#endif

#ifndef HAVE_UNSHARE
# include <sys/syscall.h>

static int unshare(int flags)
{
	return syscall(SYS_unshare, flags);
}
#endif

static void usage(int status)
{
	FILE *out = status == EXIT_SUCCESS ? stdout : stderr;

	fprintf(out, _("Usage: %s [options] <program> [args...]\n"),
		program_invocation_short_name);

	fputs(_("Run program with some namespaces unshared from parent\n\n"
		"  -h, --help        usage information (this)\n"
		"  -m, --mount       unshare mounts namespace\n"
		"  -u, --uts         unshare UTS namespace (hostname etc)\n"
		"  -i, --ipc         unshare System V IPC namespace\n"
		"  -n, --net         unshare network namespace\n"), out);

	fprintf(out, _("\nFor more information see unshare(1).\n"));
	exit(status);
}

int main(int argc, char *argv[])
{
	struct option longopts[] = {
		{ "help", no_argument, 0, 'h' },
		{ "mount", no_argument, 0, 'm' },
		{ "uts", no_argument, 0, 'u' },
		{ "ipc", no_argument, 0, 'i' },
		{ "net", no_argument, 0, 'n' },
	};

	int unshare_flags = 0;

	int c;

	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	while((c = getopt_long(argc, argv, "hmuin", longopts, NULL)) != -1) {
		switch(c) {
		case 'h':
			usage(EXIT_SUCCESS);
		case 'm':
			unshare_flags |= CLONE_NEWNS;
			break;
		case 'u':
			unshare_flags |= CLONE_NEWUTS;
			break;
		case 'i':
			unshare_flags |= CLONE_NEWIPC;
			break;
		case 'n':
			unshare_flags |= CLONE_NEWNET;
			break;
		default:
			usage(EXIT_FAILURE);
		}
	}

	if(optind >= argc)
		usage(EXIT_FAILURE);

	if(-1 == unshare(unshare_flags))
		err(EXIT_FAILURE, _("unshare failed"));

	execvp(argv[optind], argv + optind);

	err(EXIT_FAILURE, _("exec %s failed"), argv[optind]);
}

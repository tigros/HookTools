/*
 * Process Hacker Extended Tools -
 *   main program
 *
 * Copyright (C) 2010-2015 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gethooks.h"

struct snapshot *glbupdated = NULL;
struct snapshot *glbcurrent = NULL;

BOOL Isdiffhooks()
{
	int ret = 0;

	/* take a snapshot */
	ret = init_snapshot_store(glbupdated);

	return hasdiff(glbcurrent->desktop_hooks, glbupdated->desktop_hooks);
}

int gethooks()
{
	const char *const objname = "GetHooks";
	struct desktop_hook_item *dh = NULL;
	struct snapshot *temp = NULL;
	int ret = 0;

	if (!G)
	{
		create_global_store();
		init_global_prog_store(0, NULL);
		init_global_config_store();
		init_global_desktop_store();
		/* allocate the memory needed to take a snapshot */
		create_snapshot_store(&glbcurrent);
		create_snapshot_store(&glbupdated);
	}

	FAIL_IF(!G);   // The global store must exist.
	FAIL_IF(!G->prog->init_time);   // The program store must be initialized.
	FAIL_IF(!G->config->init_time);   // The configuration store must be initialized.
	FAIL_IF(!G->desktops->init_time);   // The desktop store must be initialized.

	/* take a snapshot */
	ret = init_snapshot_store(glbcurrent);

	if (!ret)
	{
		MSG_FATAL("The snapshot store failed to initialize.");
		exit(1);
	}

	return TRUE;
}
/*****************************************************************************\
 *  jobacct_gather_linux.c - slurm job accounting gather plugin for linux.
 *****************************************************************************
 *  Copyright (C) 2005 Hewlett-Packard Development Company, L.P.
 *  Written by Andy Riebs, <andy.riebs@hp.com>, who borrowed heavily
 *  from other parts of Slurm, and Danny Auble, <da@llnl.gov>
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  This file is patterned after jobcomp_linux.c, written by Morris Jette and
 *  Copyright (C) 2002 The Regents of the University of California.
\*****************************************************************************/

#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include "src/common/slurm_xlator.h"
#include "src/interfaces/jobacct_gather.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/slurm_protocol_defs.h"
#include "src/interfaces/acct_gather_energy.h"
#include "src/interfaces/acct_gather_interconnect.h"
#include "src/interfaces/proctrack.h"
#include "../common/common_jag.h"

#define _DEBUG 0

/*
 * These variables are required by the generic plugin interface.  If they
 * are not found in the plugin, the plugin loader will ignore it.
 *
 * plugin_name - a string giving a human-readable description of the
 * plugin.  There is no maximum length, but the symbol must refer to
 * a valid string.
 *
 * plugin_type - a string suggesting the type of the plugin or its
 * applicability to a particular form of data or method of data handling.
 * If the low-level plugin API is used, the contents of this string are
 * unimportant and may be anything.  Slurm uses the higher-level plugin
 * interface which requires this string to be of the form
 *
 *	<application>/<method>
 *
 * where <application> is a description of the intended application of
 * the plugin (e.g., "jobacct" for Slurm job completion logging) and <method>
 * is a description of how this plugin satisfies that application.  Slurm will
 * only load job completion logging plugins if the plugin_type string has a
 * prefix of "jobacct/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */
const char plugin_name[] = "Job accounting gather LINUX plugin";
const char plugin_type[] = "jobacct_gather/linux";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

extern int init(void)
{
	if (running_in_slurmstepd())
		jag_common_init(jobacct_gather_get_clk_tck());

	debug("%s loaded", plugin_name);

	return SLURM_SUCCESS;
}

extern void fini(void)
{
	if (running_in_slurmstepd()) {
		/* just to make sure it closes things up since we call it
		 * from here */
		acct_gather_energy_fini();
	}
}

/*
 * jobacct_gather_p_poll_data() - Build a table of all current processes
 *
 * IN/OUT: task_list - list containing current processes.
 * IN: cont_id - container id of processes if not running with pgid.
 *
 * OUT:	none
 *
 * THREADSAFE! Only one thread ever gets here.  It is locked in
 * slurm_jobacct_gather.
 *
 * Assumption:
 *    Any file with a name of the form "/proc/[0-9]+/stat"
 *    is a Linux-style stat entry. We disregard the data if they look
 *    wrong.
 */
extern void jobacct_gather_p_poll_data(
	list_t *task_list, int64_t cont_id, bool profile)
{
	static jag_callbacks_t callbacks;

	xassert(running_in_slurmstepd());
	memset(&callbacks, 0, sizeof(jag_callbacks_t));

	jag_common_poll_data(task_list, cont_id, &callbacks, profile);
	return;
}

extern int jobacct_gather_p_endpoll(void)
{
	jag_common_fini();

	return SLURM_SUCCESS;
}

extern int jobacct_gather_p_add_task(pid_t pid, jobacct_id_t *jobacct_id)
{
	return SLURM_SUCCESS;
}

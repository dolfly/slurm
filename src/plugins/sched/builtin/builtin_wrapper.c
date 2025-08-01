/*****************************************************************************\
 *  builtin_wrapper.c - NO-OP plugin for Slurm's internal scheduler.
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Kevin Tew <tew1@llnl.gov> et. al.
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
\*****************************************************************************/

#include <stdio.h>

#include "slurm/slurm_errno.h"

#include "src/common/plugin.h"
#include "src/common/log.h"
#include "src/interfaces/select.h"
#include "src/slurmctld/job_scheduler.h"
#include "src/slurmctld/reservation.h"
#include "src/slurmctld/slurmctld.h"
#include "src/plugins/sched/builtin/builtin.h"

const char		plugin_name[]	= "Slurm Built-in Scheduler plugin";
const char		plugin_type[]	= "sched/builtin";
const uint32_t		plugin_version	= SLURM_VERSION_NUMBER;

static pthread_t builtin_thread = 0;
static pthread_mutex_t thread_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int init(void)
{
	sched_verbose("Built-in scheduler plugin loaded");

	slurm_mutex_lock( &thread_flag_mutex );
	if ( builtin_thread ) {
		debug2( "Built-in scheduler thread already running, "
			"not starting another" );
		slurm_mutex_unlock( &thread_flag_mutex );
		return SLURM_ERROR;
	}

	/* since we do a join on this later we don't make it detached */
	slurm_thread_create(&builtin_thread, builtin_agent, NULL);

	slurm_mutex_unlock( &thread_flag_mutex );

	return SLURM_SUCCESS;
}

extern void fini(void)
{
	slurm_mutex_lock( &thread_flag_mutex );
	if ( builtin_thread ) {
		verbose( "Built-in scheduler plugin shutting down" );
		stop_builtin_agent();
		slurm_thread_join(builtin_thread);
	}
	slurm_mutex_unlock( &thread_flag_mutex );
}

extern int sched_p_reconfig(void)
{
	builtin_reconfig();
	return SLURM_SUCCESS;
}

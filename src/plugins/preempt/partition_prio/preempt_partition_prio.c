/*****************************************************************************\
 *  preempt_partition_prio.c - job preemption plugin that selects preemptable
 *  jobs based upon their partition's priority.
 *****************************************************************************
 *  Copyright (C) 2009-2010 Lawrence Livermore National Security.
 *  Copyright (C) SchedMD LLC.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris jette <jette1@llnl.gov>
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

#include "src/common/bitstring.h"
#include "src/common/list.h"
#include "src/common/log.h"
#include "src/common/plugin.h"
#include "src/common/xstring.h"

#include "src/interfaces/preempt.h"

#include "src/slurmctld/slurmctld.h"
#include "src/slurmctld/job_scheduler.h"

const char	plugin_name[]	= "Preempt by partition priority plugin";
const char	plugin_type[]	= "preempt/partition_prio";
const uint32_t	plugin_version	= SLURM_VERSION_NUMBER;

extern uint16_t preempt_p_get_mode(job_record_t *job_ptr)
{
	uint16_t mode = slurm_conf.preempt_mode;
	part_record_t *part_ptr = job_ptr->part_ptr;

	if (part_ptr && (part_ptr->preempt_mode != NO_VAL16))
		mode = part_ptr->preempt_mode;

	mode &= ~PREEMPT_MODE_GANG;
	mode &= ~PREEMPT_MODE_PRIORITY;

	return mode;
}

/* Generate a job priority. It is partly based upon the partition priority_tier
 * and partly based upon the job size. We want to put smaller jobs at the top
 * of the preemption queue and use a sort algorithm to minimize the number of
 * job's preempted. */
extern uint32_t preempt_p_get_prio(job_record_t *job_ptr)
{
	uint32_t job_prio;

	if (job_ptr->part_ptr)
		job_prio = job_ptr->part_ptr->priority_tier << 16;
	else
		job_prio = 0;

	if (job_ptr->node_cnt >= 0xffff)
		job_prio += 0xffff;
	else
		job_prio += job_ptr->node_cnt;

	return job_prio;
}

extern uint32_t preempt_p_get_grace_time(job_record_t *job_ptr)
{
	if (!job_ptr->part_ptr)
		return 0;

	return job_ptr->part_ptr->grace_time;
}

extern int init(void)
{
	verbose("%s loaded", plugin_type);
	return SLURM_SUCCESS;
}

extern void fini(void)
{
	return;
}

/* Return true if the preemptor can preempt the preemptee, otherwise false */
extern bool preempt_p_job_preempt_check(job_queue_rec_t *preemptor,
					job_queue_rec_t *preemptee)
{
	if (((preemptor->part_ptr->preempt_mode != NO_VAL16) &&
	     (preemptor->part_ptr->preempt_mode & PREEMPT_MODE_PRIORITY)) ||
	    (slurm_conf.preempt_mode & PREEMPT_MODE_PRIORITY))
		if (preemptor->priority < preemptee->priority)
			return false;

	if (preemptor->part_ptr && preemptee->part_ptr &&
	    (bit_overlap_any(preemptor->part_ptr->node_bitmap,
			     preemptee->part_ptr->node_bitmap)) &&
	    (preemptor->part_ptr->priority_tier >
	     preemptee->part_ptr->priority_tier))
			return true;

	return false;
}

extern bool preempt_p_preemptable(
	job_record_t *preemptee, job_record_t *preemptor)
{
	if (((preemptor->part_ptr->preempt_mode != NO_VAL16) &&
	     (preemptor->part_ptr->preempt_mode & PREEMPT_MODE_PRIORITY)) ||
	    (slurm_conf.preempt_mode & PREEMPT_MODE_PRIORITY))
		if (preemptor->priority < preemptee->priority)
			return false;

	if ((preemptee->part_ptr == NULL) ||
	    (preemptee->part_ptr->priority_tier >=
	     preemptor->part_ptr->priority_tier) ||
	    (preemptee->part_ptr->preempt_mode == PREEMPT_MODE_OFF))
		return false;

	return true;
}

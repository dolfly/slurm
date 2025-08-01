/*****************************************************************************\
 *  gres_select_filter.h - filters used in the select plugin
 *****************************************************************************
 *  Copyright (C) SchedMD LLC.
 *  Derived in large part from code previously in interfaces/gres.h
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

#ifndef _GRES_SELECT_FILTER_H
#define _GRES_SELECT_FILTER_H

#include "src/interfaces/gres.h"

typedef struct {
	bool *avail_cores_by_sock;
	uint16_t *avail_gpus; /* Count of available GPUs on this node */
	uint64_t avail_mem; /* memory available for the job or NO_VAL64
			       (when no SELECT_MEMORY) */
	uint16_t cores_per_sock; /* Count of cores per socket on this node */
	bitstr_t *core_bitmap; /* available cores on this node */
	uint16_t cpus_per_core; /* Count of CPUs per core on this node */
	uint16_t cpus_per_task; /* Count of CPUs per task */
	bool enforce_binding; /* GRES must be co-allocated with cores */
	uint16_t max_cpus; /* maximum CPUs available on this node (limited by
			      specialized cores and partition CPUs-per-node) */
	uint16_t *near_gpus; /* # of GPUs avail on sockets with avail CPUs */
	uint16_t sockets; /* Count of sockets on the node*/
	uint32_t sock_per_node; /* sockets wanted by job per node or NO_VAL */
	uint16_t task_per_node; /* tasks wanted by job per node or NO_VAL16 */
	bool whole_node; /* we are requesting the whole node or not */
} gres_remove_unused_args_t;

/*
 * Determine which GRES can be used on this node given the available cores.
 *	Filter out unusable GRES.
 * IN sock_gres_list - list of sock_gres_t entries built by
 *                     gres_sock_list_create()
 * IN args->avail_mem - memory available for the job or NO_VAL64
 *                      (when no SELECT_MEMORY)
 * IN args->max_cpus - maximum CPUs available on this node (limited by
 *                     specialized cores and partition CPUs-per-node)
 * IN args->enforce_binding - GRES must be co-allocated with cores
 * IN args->core_bitmap - Identification of available cores on this node
 * IN args->sockets - Count of sockets on the node
 * IN args->cores_per_sock - Count of cores per socket on this node
 * IN args->cpus_per_core - Count of CPUs per core on this node
 * IN args->sock_per_node - sockets requested by job per node or NO_VAL
 * IN args->task_per_node - tasks requested by job per node or NO_VAL16
 * IN args->cpus_per_task - Count of CPUs per task
 * IN args->whole_node - we are requesting the whole node or not
 * OUT args->avail_gpus - Count of available GPUs on this node
 * OUT args->near_gpus - Count of GPUs available on sockets with available CPUs
 * RET - 0 if job can use this node, -1 otherwise (some GRES limit prevents use)
 */
extern int gres_select_filter_remove_unusable(list_t *sock_gres_list,
					      gres_remove_unused_args_t *args);

/*
 * Make final GRES selection for the job
 * sock_gres_list IN - per-socket GRES details, one record per allocated node
 * IN job_ptr - job's pointer
 * tres_mc_ptr IN - job's multi-core options
 * RET SLURM_SUCCESS or error code
 */
extern int gres_select_filter_select_and_set(list_t **sock_gres_list,
					     job_record_t *job_ptr,
					     gres_mc_data_t *tres_mc_ptr);

#endif /* _GRES_SELECT_FILTER_H */

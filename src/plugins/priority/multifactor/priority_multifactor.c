/*****************************************************************************\
 *  priority_multifactor.c - slurm multifactor priority plugin.
 *****************************************************************************
 *
 *  Copyright (C) 2012  Aalto University
 *  Written by Janne Blomqvist <janne.blomqvist@aalto.fi>
 *
 *  Based on priority_multifactor.c, whose copyright information is
 *  reproduced below:
 *
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Danny Auble <da@llnl.gov>
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

#include "config.h"

#if HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/stat.h>

#include "slurm/slurm_errno.h"

#include "src/common/parse_time.h"
#include "src/common/slurm_time.h"
#include "src/common/state_save.h"
#include "src/common/xstring.h"

#include "src/interfaces/gres.h"
#include "src/interfaces/mcs.h"
#include "src/interfaces/priority.h"
#include "src/interfaces/site_factor.h"

#include "src/slurmctld/acct_policy.h"
#include "src/slurmctld/licenses.h"
#include "src/slurmctld/read_config.h"
#include "src/slurmctld/state_save.h"

#include "fair_tree.h"

#define SECS_PER_DAY	(24 * 60 * 60)
#define SECS_PER_WEEK	(7 * SECS_PER_DAY)

/* These are defined here so when we link with something other than
 * the slurmctld we will have these symbols defined.  They will get
 * overwritten when linking with the slurmctld.
 */
#if defined (__APPLE__)
extern void *acct_db_conn  __attribute__((weak_import));
extern uint32_t cluster_cpus __attribute__((weak_import));
extern list_t *job_list  __attribute__((weak_import));
extern time_t last_job_update __attribute__((weak_import));
extern time_t last_part_update __attribute__((weak_import));
extern slurm_conf_t slurm_conf __attribute__((weak_import));
extern int slurmctld_tres_cnt __attribute__((weak_import));
extern uint16_t accounting_enforce __attribute__((weak_import));
extern int active_node_record_count __attribute__((weak_import));
#else
void *acct_db_conn = NULL;
uint32_t cluster_cpus = NO_VAL;
list_t *job_list = NULL;
time_t last_job_update = (time_t) 0;
time_t last_part_update = (time_t) 0;
slurm_conf_t slurm_conf;
int slurmctld_tres_cnt = 0;
uint16_t accounting_enforce = 0;
int active_node_record_count;
#endif

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
 * the plugin (e.g., "jobcomp" for Slurm job completion logging) and <method>
 * is a description of how this plugin satisfies that application.  Slurm will
 * only load job completion logging plugins if the plugin_type string has a
 * prefix of "jobcomp/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */
const char plugin_name[]	= "Priority MULTIFACTOR plugin";
const char plugin_type[]	= "priority/multifactor";
const uint32_t plugin_version	= SLURM_VERSION_NUMBER;

static pthread_t decay_handler_thread;
static pthread_mutex_t decay_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t decay_cond = PTHREAD_COND_INITIALIZER;
static bool running_decay = 0, reconfig = 0, calc_fairshare = 1;
static time_t plugin_shutdown = 0;
static uint16_t damp_factor = 1;  /* weight for age factor */
static uint32_t max_age; /* time when not to add any more
			  * priority to a job if reached */
static uint32_t weight_age;  /* weight for age factor */
static uint32_t weight_assoc;/* weight for assoc factor */
static uint32_t weight_fs;   /* weight for Fairshare factor */
static uint32_t weight_js;   /* weight for Job Size factor */
static uint32_t weight_part; /* weight for Partition factor */
static uint32_t weight_qos;  /* weight for QOS factor */
static double  *weight_tres; /* tres weights */
static uint32_t flags;       /* Priority Flags */
static time_t g_last_ran = 0; /* when the last poll ran */
static time_t g_last_reset = 0; /* when the last reset was done */
static double decay_factor = 1; /* The decay factor when decaying time. */

typedef struct {
	job_record_t *job_ptr;
	part_record_t *part_ptr;
	list_t *ret_list;
} create_prio_list_t;

/* variables defined in priority_multifactor.h */

static void _priority_p_set_assoc_usage_debug(slurmdb_assoc_rec_t *assoc);
static void _set_assoc_usage_efctv(slurmdb_assoc_rec_t *assoc);

static void _destroy_priority_factors_obj_light(void *object)
{
	priority_factors_object_t *obj_ptr = object;

	if (!obj_ptr)
		return;

	slurm_destroy_priority_factors(obj_ptr->prio_factors);
	xfree(obj_ptr);
}

/*
 * apply decay factor to all associations usage_raw
 * IN: real_decay - decay to be applied to each associations' used
 * shares.  This should already be modified with the amount of delta
 * time from last application..
 * RET: SLURM_SUCCESS on SUCCESS, SLURM_ERROR else.
 */
static int _apply_decay(double real_decay)
{
	int i;
	list_itr_t *itr = NULL;
	slurmdb_assoc_rec_t *assoc = NULL;
	slurmdb_qos_rec_t *qos = NULL;
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, WRITE_LOCK, NO_LOCK,
				   NO_LOCK, NO_LOCK, NO_LOCK };

	/* continue if real_decay is 0 or 1 since that doesn't help
	   us at all. 1 means no decay and 0 will just zero
	   everything out so don't waste time doing it */
	if (!real_decay)
		return SLURM_ERROR;
	else if (!calc_fairshare || (real_decay == 1))
		return SLURM_SUCCESS;

	assoc_mgr_lock(&locks);

	xassert(assoc_mgr_assoc_list);
	xassert(assoc_mgr_qos_list);

	itr = list_iterator_create(assoc_mgr_assoc_list);
	/* We want to do this to all associations including root.
	   All usage_raws are calculated from the bottom up.
	*/
	while ((assoc = list_next(itr))) {
		assoc->usage->usage_raw *= real_decay;
		for (i=0; i<slurmctld_tres_cnt; i++)
			assoc->usage->usage_tres_raw[i] *= real_decay;
		assoc->usage->grp_used_wall *= real_decay;

		if (assoc->leaf_usage && (assoc->leaf_usage != assoc->usage)) {
			assoc->leaf_usage->usage_raw *= real_decay;
			for (i = 0; i < slurmctld_tres_cnt; i++)
				assoc->leaf_usage->usage_tres_raw[i] *=
					real_decay;
			assoc->leaf_usage->grp_used_wall *= real_decay;
		}
	}
	list_iterator_destroy(itr);

	itr = list_iterator_create(assoc_mgr_qos_list);
	while ((qos = list_next(itr))) {
		if (qos->flags & QOS_FLAG_NO_DECAY)
			continue;
		qos->usage->usage_raw *= real_decay;
		for (i=0; i<slurmctld_tres_cnt; i++)
			qos->usage->usage_tres_raw[i] *= real_decay;
		qos->usage->grp_used_wall *= real_decay;
	}
	list_iterator_destroy(itr);
	assoc_mgr_unlock(&locks);

	return SLURM_SUCCESS;
}

/*
 * reset usage_raw, and grp_used_wall on all assocs
 * This should be called every PriorityUsageResetPeriod
 * RET: SLURM_SUCCESS on SUCCESS, SLURM_ERROR else.
 */
static int _reset_usage(void)
{
	list_itr_t *itr = NULL;
	slurmdb_assoc_rec_t *assoc = NULL;
	slurmdb_qos_rec_t *qos = NULL;
	int i;
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, WRITE_LOCK,
				   NO_LOCK, NO_LOCK, NO_LOCK, NO_LOCK };

	if (!calc_fairshare)
		return SLURM_SUCCESS;

	assoc_mgr_lock(&locks);

	xassert(assoc_mgr_assoc_list);

	itr = list_iterator_create(assoc_mgr_assoc_list);
	/* We want to do this to all associations including root.
	 * All usage_raws are calculated from the bottom up.
	 */
	while ((assoc = list_next(itr))) {
		assoc->usage->usage_raw = 0;
		for (i=0; i<slurmctld_tres_cnt; i++)
			assoc->usage->usage_tres_raw[i] = 0;
		assoc->usage->grp_used_wall = 0;

		if (assoc->leaf_usage && (assoc->leaf_usage != assoc->usage)) {
			slurmdb_destroy_assoc_usage(assoc->leaf_usage);
			assoc->leaf_usage = NULL;
		}
	}
	list_iterator_destroy(itr);

	itr = list_iterator_create(assoc_mgr_qos_list);
	while ((qos = list_next(itr))) {
		qos->usage->usage_raw = 0;
		for (i=0; i<slurmctld_tres_cnt; i++)
			qos->usage->usage_tres_raw[i] = 0;
		qos->usage->grp_used_wall = 0;
	}
	list_iterator_destroy(itr);
	assoc_mgr_unlock(&locks);

	return SLURM_SUCCESS;
}

static void _read_last_decay_ran(time_t *last_ran, time_t *last_reset)
{
	char *state_file;
	buf_t *buffer;

	xassert(last_ran);
	xassert(last_reset);

	(*last_ran) = 0;
	(*last_reset) = 0;

	/* read the file */
	buffer = state_save_open("priority_last_decay_ran", &state_file);
	if (!buffer) {
		info("No last decay (%s) to recover", state_file);
		xfree(state_file);
		return;
	}
	xfree(state_file);

	safe_unpack_time(last_ran, buffer);
	safe_unpack_time(last_reset, buffer);
	FREE_NULL_BUFFER(buffer);
	log_flag(PRIO, "Last ran decay on jobs at %ld", (long) *last_ran);

	return;

unpack_error:
	if (!ignore_state_errors)
		fatal("Incomplete priority last decay file exiting, start with '-i' to ignore this. Warning: using -i will lose the data that can't be recovered.");
	error("Incomplete priority last decay file returning");
	FREE_NULL_BUFFER(buffer);
	return;

}

static int _write_last_decay_ran(time_t last_ran, time_t last_reset)
{
	/* Save high-water mark to avoid buffer growth with copies */
	static uint32_t high_buffer_size = BUF_SIZE;
	int error_code = SLURM_SUCCESS;
	buf_t *buffer;

	if (!xstrcmp(slurm_conf.state_save_location, "/dev/null")) {
		error("Can not save priority state information, "
		      "StateSaveLocation is /dev/null");
		return error_code;
	}

	buffer = init_buf(high_buffer_size);
	pack_time(last_ran, buffer);
	pack_time(last_reset, buffer);

	error_code = save_buf_to_state("priority_last_decay_ran", buffer,
				       &high_buffer_size);

	debug4("done writing time %ld", (long)last_ran);
	FREE_NULL_BUFFER(buffer);

	return error_code;
}


/* This should initially get the children list from assoc_mgr_root_assoc.
 * Since our algorithm goes from top down we calculate all the non-user
 * associations now.  When a user submits a job, that norm_fairshare is
 * calculated.  Here we will set the usage_efctv to NO_VAL for users to not have
 * to calculate a bunch of things that will never be used. (Fair Tree calls a
 * different function.)
 *
 * NOTE: acct_mgr_assoc_lock must be locked before this is called.
 */
static int _set_children_usage_efctv(list_t *children_list)
{
	slurmdb_assoc_rec_t *assoc = NULL;
	list_itr_t *itr = NULL;

	if (!children_list || !list_count(children_list))
		return SLURM_SUCCESS;

	itr = list_iterator_create(children_list);
	while ((assoc = list_next(itr))) {
		if (assoc->user) {
			assoc->usage->usage_efctv = (long double)NO_VAL;
			continue;
		}
		priority_p_set_assoc_usage(assoc);
		_set_children_usage_efctv(assoc->usage->children_list);
	}
	list_iterator_destroy(itr);
	return SLURM_SUCCESS;
}


/* job_ptr should already have the partition priority and such added here
 * before had we will be adding to it
 */
static double _get_fairshare_priority(job_record_t *job_ptr)
{
	slurmdb_assoc_rec_t *job_assoc;
	slurmdb_assoc_rec_t *fs_assoc = NULL;
	double priority_fs = 0.0;
	assoc_mgr_lock_t locks = { READ_LOCK, NO_LOCK, NO_LOCK, NO_LOCK,
				   NO_LOCK, NO_LOCK, NO_LOCK };

	if (!calc_fairshare)
		return 0;

	assoc_mgr_lock(&locks);

	job_assoc = job_ptr->assoc_ptr;

	if (!job_assoc) {
		assoc_mgr_unlock(&locks);
		error("Job %u has no association.  Unable to "
		      "compute fairshare.", job_ptr->job_id);
		return 0;
	}

	/* Use values from parent when FairShare=SLURMDB_FS_USE_PARENT */
	if (job_assoc->shares_raw == SLURMDB_FS_USE_PARENT)
		fs_assoc = job_assoc->usage->fs_assoc_ptr;
	else
		fs_assoc = job_assoc;

	if (fuzzy_equal(fs_assoc->usage->usage_efctv, NO_VAL))
		priority_p_set_assoc_usage(fs_assoc);

	/* Priority is 0 -> 1 */
	if (flags & PRIORITY_FLAGS_FAIR_TREE) {
		priority_fs = job_assoc->usage->fs_factor;
		log_flag(PRIO, "Fairshare priority of job %u for user %s in acct %s is %f",
			 job_ptr->job_id, job_assoc->user, job_assoc->acct,
			 priority_fs);
	} else {
		priority_fs = priority_p_calc_fs_factor(
			fs_assoc->usage->usage_efctv,
			(long double)fs_assoc->usage->shares_norm);
		log_flag(PRIO, "Fairshare priority of job %u for user %s in acct %s is 2**(-%Lf/%f) = %f",
			 job_ptr->job_id, job_assoc->user, job_assoc->acct,
			 fs_assoc->usage->usage_efctv,
			 fs_assoc->usage->shares_norm, priority_fs);
	}
	assoc_mgr_unlock(&locks);

	return priority_fs;
}

static void _get_tres_factors(job_record_t *job_ptr, part_record_t *part_ptr,
			      double *tres_factors)
{
	int i;

	xassert(tres_factors);

	/* can't memcpy because of different types
	 * uint64_t vs. double */
	for (i = 0; i < slurmctld_tres_cnt; i++) {
		uint64_t value = 0;
		if (job_ptr->tres_alloc_cnt &&
		    (job_ptr->tres_alloc_cnt[i] != NO_CONSUME_VAL64))
			value = job_ptr->tres_alloc_cnt[i];
		else if (job_ptr->tres_req_cnt)
			value = job_ptr->tres_req_cnt[i];

		if (flags & PRIORITY_FLAGS_NO_NORMAL_TRES)
			tres_factors[i] = value;
		else if (value &&
			 part_ptr &&
			 part_ptr->tres_cnt &&
			 part_ptr->tres_cnt[i])
			tres_factors[i] = value /
				(double)part_ptr->tres_cnt[i];
	}
}

static double _get_tres_prio_weighted(double *tres_factors)
{
	int i;
	double tmp_tres = 0.0;

	xassert(tres_factors);

	if (!weight_tres)
		return tmp_tres;

	for (i = 0; i < slurmctld_tres_cnt; i++) {
		tres_factors[i] *= weight_tres[i];
		tmp_tres += tres_factors[i];
	}

	return tmp_tres;
}

typedef struct {
	int counter;
	job_record_t *job_ptr;
	char *multi_prio_str;
	part_record_t *part_ptr;
	double part_tres;
	double priority_part_init;
} mult_prio_args_t;

static int _priority_each_qos(void *object, void *arg)
{
	slurmdb_qos_rec_t *qos_ptr = object;
	mult_prio_args_t *args = arg;
	job_record_t *job_ptr = args->job_ptr;
	double priority_qos = job_ptr->prio_factors->priority_qos;
	double priority_part = args->priority_part_init;
	uint64_t tmp_64;

	if (qos_ptr && qos_ptr->priority && weight_qos) {
		priority_qos =
			((flags & PRIORITY_FLAGS_NO_NORMAL_QOS) ?
			 qos_ptr->priority : qos_ptr->usage->norm_priority) *
			(double) weight_qos;
	}

	priority_part +=
		(job_ptr->prio_factors->priority_age +
		 job_ptr->prio_factors->priority_assoc +
		 job_ptr->prio_factors->priority_fs +
		 job_ptr->prio_factors->priority_js +
		 priority_qos +
		 args->part_tres +
		 (double)(((int64_t)job_ptr->prio_factors->priority_site)
			  - NICE_OFFSET) -
		 (double)(((int64_t)job_ptr->prio_factors->nice)
			     - NICE_OFFSET));

	/* Priority 0 is reserved for held jobs */
	if (priority_part < 1)
		priority_part = 1;

	tmp_64 = (uint64_t) priority_part;
	if (tmp_64 > 0xffffffff) {
		error("%pJ priority '%"PRIu64"' exceeds 32 bits. Reducing it to 4294967295 (2^32 - 1)",
		      job_ptr, tmp_64);
		tmp_64 = 0xffffffff;
		priority_part = (double) tmp_64;
	}
	if (((flags & PRIORITY_FLAGS_INCR_ONLY) == 0) ||
	    (job_ptr->prio_mult->priority_array[args->counter] <
	     (uint32_t) priority_part)) {
		job_ptr->prio_mult->priority_array[args->counter] =
			(uint32_t) priority_part;
	}
	if (slurm_conf.debug_flags & DEBUG_FLAG_PRIO) {
		if (args->multi_prio_str)
			xstrcat(args->multi_prio_str, ", ");
		if (qos_ptr)
			xstrfmtcat(args->multi_prio_str,
				   "%s/%s=%u", args->part_ptr->name,
				   qos_ptr->name,
				   job_ptr->prio_mult->
				   priority_array[args->counter]);
		else
			xstrfmtcat(args->multi_prio_str,
				   "%s=%u", args->part_ptr->name,
				   job_ptr->prio_mult->
				   priority_array[args->counter]);
	}

	args->counter++;
	return SLURM_SUCCESS;
}

static int _priority_each_partition(void *object, void *arg)
{
	mult_prio_args_t *args = arg;
	part_record_t *part_ptr = (part_record_t *)object;
	job_record_t *job_ptr = args->job_ptr;

	args->part_ptr = part_ptr;

	if (weight_tres) {
		double part_tres_factors[slurmctld_tres_cnt];
		memset(part_tres_factors, 0,
		       sizeof(double) * slurmctld_tres_cnt);
		_get_tres_factors(job_ptr, part_ptr, part_tres_factors);
		args->part_tres = _get_tres_prio_weighted(part_tres_factors);
	}

	args->priority_part_init =
		((flags & PRIORITY_FLAGS_NO_NORMAL_PART) ?
		 part_ptr->priority_job_factor :
		 part_ptr->norm_priority) * (double)weight_part;

	if (job_ptr->qos_list) {
		assoc_mgr_lock_t locks = {
			.qos = READ_LOCK,
		};
		assoc_mgr_lock(&locks);
		(void) list_for_each(job_ptr->qos_list,
				     _priority_each_qos, args);
		assoc_mgr_unlock(&locks);
	} else {
		(void) _priority_each_qos(NULL, args);
	}

	return SLURM_SUCCESS;
}

/* Returns the priority after applying the weight factors */
static uint32_t _get_priority_internal(time_t start_time,
				       job_record_t *job_ptr)
{
	double priority	= 0.0;
	priority_factors_t pre_factors;
	uint64_t tmp_64;
	double tmp_tres = 0.0;

	if (job_ptr->direct_set_prio && (job_ptr->priority > 0)) {
		if (job_ptr->prio_factors) {
			xfree(job_ptr->prio_factors->tres_weights);
			xfree(job_ptr->prio_factors->priority_tres);
			memset(job_ptr->prio_factors, 0,
			       sizeof(priority_factors_t));
		}
		return job_ptr->priority;
	}

	if (!job_ptr->details) {
		error("_get_priority_internal: job %u does not have a "
		      "details symbol set, can't set priority",
		      job_ptr->job_id);
		if (job_ptr->prio_factors) {
			xfree(job_ptr->prio_factors->tres_weights);
			xfree(job_ptr->prio_factors->priority_tres);
			memset(job_ptr->prio_factors, 0,
			       sizeof(priority_factors_t));
		}
		return 0;
	}

	set_priority_factors(start_time, job_ptr);

	if (slurm_conf.debug_flags & DEBUG_FLAG_PRIO) {
		memcpy(&pre_factors, job_ptr->prio_factors,
		       sizeof(priority_factors_t));
		if (job_ptr->prio_factors->priority_tres) {
			pre_factors.priority_tres = xcalloc(slurmctld_tres_cnt,
							    sizeof(double));
			memcpy(pre_factors.priority_tres,
			       job_ptr->prio_factors->priority_tres,
			       sizeof(double) * slurmctld_tres_cnt);
		}
	} else	/* clang needs this memset to avoid a warning */
		memset(&pre_factors, 0, sizeof(priority_factors_t));

	job_ptr->prio_factors->priority_age  *= (double)weight_age;
	job_ptr->prio_factors->priority_assoc *= (double)weight_assoc;
	job_ptr->prio_factors->priority_fs   *= (double)weight_fs;
	job_ptr->prio_factors->priority_js   *= (double)weight_js;
	job_ptr->prio_factors->priority_part *= (double)weight_part;
	job_ptr->prio_factors->priority_qos  *= (double)weight_qos;

	if (weight_tres && job_ptr->prio_factors->priority_tres) {
		double *tres_factors = NULL;
		tres_factors = job_ptr->prio_factors->priority_tres;
		tmp_tres = _get_tres_prio_weighted(tres_factors);
	}

	priority = job_ptr->prio_factors->priority_age
		+ job_ptr->prio_factors->priority_assoc
		+ job_ptr->prio_factors->priority_fs
		+ job_ptr->prio_factors->priority_js
		+ job_ptr->prio_factors->priority_part
		+ job_ptr->prio_factors->priority_qos
		+ tmp_tres
		+ (double)(((int64_t)job_ptr->prio_factors->priority_site)
			   - NICE_OFFSET)
		- (double)(((int64_t)job_ptr->prio_factors->nice)
			   - NICE_OFFSET);

	/* Priority 0 is reserved for held jobs */
	if (priority < 1)
		priority = 1;

	tmp_64 = (uint64_t) priority;
	if (tmp_64 > 0xffffffff) {
		error("%pJ priority '%"PRIu64"' exceeds 32 bits. Reducing it to 4294967295 (2^32 - 1)",
		      job_ptr, tmp_64);
		tmp_64 = 0xffffffff;
		priority = (double) tmp_64;
	}

	/*
	 * Free after transitioning from multi-part/qos job to single part job.
	 */
	if (!job_ptr->part_ptr_list && !job_ptr->qos_list &&
	    job_ptr->prio_mult) {
		xfree(job_ptr->prio_mult->priority_array);
		xfree(job_ptr->prio_mult->priority_array_names);
		xfree(job_ptr->prio_mult);
	}

	if (job_ptr->part_ptr_list || job_ptr->qos_list) {
		int i = 0;
		mult_prio_args_t arg = {
			.job_ptr = job_ptr
		};

		if (!job_ptr->prio_mult)
			job_ptr->prio_mult =
				xmalloc(sizeof(*job_ptr->prio_mult));

		if (job_ptr->prio_mult &&
		    (!job_ptr->prio_mult->priority_array ||
		     (job_ptr->prio_mult->last_update < last_part_update))) {

			xfree(job_ptr->prio_mult->priority_array);
			if (job_ptr->part_ptr_list) {
				i = list_count(job_ptr->part_ptr_list);
				/* part_ptr_list is already sorted */
				xfree(job_ptr->prio_mult->priority_array_names);
				job_ptr->prio_mult->priority_array_names =
					part_list_to_xstr(
						job_ptr->part_ptr_list);
			}

			if (job_ptr->qos_list) {
				int qos_count = list_count(job_ptr->qos_list);
				if (i)
					i *= qos_count;
				else
					i = qos_count;
			}

			job_ptr->prio_mult->priority_array =
				xcalloc(i, sizeof(uint32_t));

			job_ptr->prio_mult->last_update = time(NULL);
		}

		if (job_ptr->part_ptr_list)
			(void) list_for_each(job_ptr->part_ptr_list,
					     _priority_each_partition,
					     &arg);
		else
			(void) _priority_each_partition(
				job_ptr->part_ptr, &arg);

		log_flag(PRIO, "%pJ multi-partition/qos priorities: %s",
			 job_ptr, arg.multi_prio_str);
		xfree(arg.multi_prio_str);
	}

	if (slurm_conf.debug_flags & DEBUG_FLAG_PRIO) {
		int i;
		double *post_tres_factors =
			job_ptr->prio_factors->priority_tres;
		double *pre_tres_factors = pre_factors.priority_tres;
		assoc_mgr_lock_t locks = { NO_LOCK, NO_LOCK, NO_LOCK, NO_LOCK,
					   READ_LOCK, NO_LOCK, NO_LOCK };
		int64_t priority_site =
			(((int64_t)job_ptr->prio_factors->priority_site) -
			 NICE_OFFSET);

		info("Weighted Age priority is %f * %u = %.2f",
		     pre_factors.priority_age, weight_age,
		     job_ptr->prio_factors->priority_age);
		info("Weighted Assoc priority is %f * %u = %.2f",
		     pre_factors.priority_assoc, weight_assoc,
		     job_ptr->prio_factors->priority_assoc);
		info("Weighted Fairshare priority is %f * %u = %.2f",
		     pre_factors.priority_fs, weight_fs,
		     job_ptr->prio_factors->priority_fs);
		info("Weighted JobSize priority is %f * %u = %.2f",
		     pre_factors.priority_js, weight_js,
		     job_ptr->prio_factors->priority_js);
		info("Weighted Partition priority is %f * %u = %.2f",
		     pre_factors.priority_part, weight_part,
		     job_ptr->prio_factors->priority_part);
		info("Weighted QOS priority is %f * %u = %.2f",
		     pre_factors.priority_qos, weight_qos,
		     job_ptr->prio_factors->priority_qos);
		info("Site priority is %"PRId64, priority_site);

		if (weight_tres && pre_tres_factors && post_tres_factors) {
			assoc_mgr_lock(&locks);
			for(i = 0; i < slurmctld_tres_cnt; i++) {
				if (!post_tres_factors[i])
					continue;
				info("Weighted TRES:%s is %f * %.2f = %.2f",
				     assoc_mgr_tres_name_array[i],
				     pre_tres_factors[i], weight_tres[i],
				     post_tres_factors[i]);
			}
			assoc_mgr_unlock(&locks);
		}

		info("Job %u priority: %"PRId64" + %2.f + %.2f + %.2f + %.2f + %.2f + %.2f + %2.f - %"PRId64" = %.2f",
		     job_ptr->job_id,
		     priority_site,
		     job_ptr->prio_factors->priority_age,
		     job_ptr->prio_factors->priority_assoc,
		     job_ptr->prio_factors->priority_fs,
		     job_ptr->prio_factors->priority_js,
		     job_ptr->prio_factors->priority_part,
		     job_ptr->prio_factors->priority_qos,
		     tmp_tres,
		     (((int64_t)job_ptr->prio_factors->nice) - NICE_OFFSET),
		     priority);

		xfree(pre_factors.priority_tres);
	}
	return (uint32_t)priority;
}


/* based upon the last reset time, compute when the next reset should be */
static time_t _next_reset(uint16_t reset_period, time_t last_reset)
{
	struct tm last_tm;
	time_t tmp_time, now = time(NULL);

	if (localtime_r(&last_reset, &last_tm) == NULL)
		return (time_t) 0;

	last_tm.tm_sec   = 0;
	last_tm.tm_min   = 0;
	last_tm.tm_hour  = 0;
/*	last_tm.tm_wday = 0	ignored */
/*	last_tm.tm_yday = 0;	ignored */
	switch (reset_period) {
	case PRIORITY_RESET_DAILY:
		tmp_time = slurm_mktime(&last_tm);
		tmp_time += SECS_PER_DAY;
		while ((tmp_time + SECS_PER_DAY) < now)
			tmp_time += SECS_PER_DAY;
		return tmp_time;
	case PRIORITY_RESET_WEEKLY:
		tmp_time = slurm_mktime(&last_tm);
		tmp_time += (SECS_PER_DAY * (7 - last_tm.tm_wday));
		while ((tmp_time + SECS_PER_WEEK) < now)
			tmp_time += SECS_PER_WEEK;
		return tmp_time;
	case PRIORITY_RESET_MONTHLY:
		last_tm.tm_mday = 1;
		if (last_tm.tm_mon < 11)
			last_tm.tm_mon++;
		else {
			last_tm.tm_mon  = 0;
			last_tm.tm_year++;
		}
		break;
	case PRIORITY_RESET_QUARTERLY:
		last_tm.tm_mday = 1;
		if (last_tm.tm_mon < 3)
			last_tm.tm_mon = 3;
		else if (last_tm.tm_mon < 6)
			last_tm.tm_mon = 6;
		else if (last_tm.tm_mon < 9)
			last_tm.tm_mon = 9;
		else {
			last_tm.tm_mon  = 0;
			last_tm.tm_year++;
		}
		break;
	case PRIORITY_RESET_YEARLY:
		last_tm.tm_mday = 1;
		last_tm.tm_mon  = 0;
		last_tm.tm_year++;
		break;
	default:
		return (time_t) 0;
	}
	return slurm_mktime(&last_tm);
}

static void _handle_qos_tres_run_secs(long double *tres_run_decay,
				      uint64_t *tres_run_delta,
				      job_record_t *job_ptr,
				      slurmdb_qos_rec_t *qos)
{
	slurmdb_used_limits_t *used_limits_a = NULL;
	slurmdb_used_limits_t *used_limits_u = NULL;

	int i;

	if (!qos || !(accounting_enforce & ACCOUNTING_ENFORCE_LIMITS))
		return;

	if (!job_ptr->assoc_ptr)
		return;

	used_limits_a = acct_policy_get_acct_used_limits(
		&qos->usage->acct_limit_list, job_ptr->assoc_ptr->acct);
	used_limits_u = acct_policy_get_user_used_limits(
		&qos->usage->user_limit_list, job_ptr->user_id);

	for (i=0; i<slurmctld_tres_cnt; i++) {
		if (i == TRES_ARRAY_ENERGY)
			continue;
		if (tres_run_decay)
			qos->usage->usage_tres_raw[i] += tres_run_decay[i];

		if (tres_run_delta[i] >
		    qos->usage->grp_used_tres_run_secs[i]) {
			error("_handle_qos_tres_run_secs: job %u: "
			      "QOS %s TRES %s grp_used_tres_run_secs "
			      "underflow, tried to remove %"PRIu64" seconds "
			      "when only %"PRIu64" remained.",
			      job_ptr->job_id,
			      qos->name,
			      assoc_mgr_tres_name_array[i],
			      tres_run_delta[i],
			      qos->usage->grp_used_tres_run_secs[i]);
			qos->usage->grp_used_tres_run_secs[i] = 0;
		} else
			qos->usage->grp_used_tres_run_secs[i] -=
				tres_run_delta[i];

		log_flag(PRIO, "%s: job %u: Removed %"PRIu64" unused seconds from QOS %s TRES %s grp_used_tres_run_secs = %"PRIu64,
			 __func__, job_ptr->job_id, tres_run_delta[i], qos->name,
			 assoc_mgr_tres_name_array[i],
			 qos->usage->grp_used_tres_run_secs[i]);

		if (tres_run_delta[i] >
		    used_limits_a->tres_run_secs[i]) {
			error("_handle_qos_tres_run_secs: job %u: "
			      "QOS %s TRES %s account used limit tres_run_secs "
			      "underflow, tried to remove %"PRIu64" seconds "
			      "when only %"PRIu64" remained.",
			      job_ptr->job_id,
			      qos->name,
			      assoc_mgr_tres_name_array[i],
			      tres_run_delta[i],
			      used_limits_a->tres_run_secs[i]);
			used_limits_a->tres_run_secs[i] = 0;
		} else
			used_limits_a->tres_run_secs[i] -=
				tres_run_delta[i];

		log_flag(PRIO, "%s: job %u: Removed %"PRIu64" unused seconds from QOS %s TRES %s user used limit tres_run_secs = %"PRIu64,
			 __func__, job_ptr->job_id, tres_run_delta[i], qos->name,
			 assoc_mgr_tres_name_array[i],
			 used_limits_a->tres_run_secs[i]);

		if (tres_run_delta[i] >
		    used_limits_u->tres_run_secs[i]) {
			error("_handle_qos_tres_run_secs: job %u: QOS %s TRES %s user used limit tres_run_secs underflow, tried to remove %"PRIu64" seconds when only %"PRIu64" remained.",
			      job_ptr->job_id,
			      qos->name,
			      assoc_mgr_tres_name_array[i],
			      tres_run_delta[i],
			      used_limits_u->tres_run_secs[i]);
			used_limits_u->tres_run_secs[i] = 0;
		} else
			used_limits_u->tres_run_secs[i] -=
				tres_run_delta[i];

		log_flag(PRIO, "%s: job %u: Removed %"PRIu64" unused seconds from QOS %s TRES %s account used limit tres_run_secs = %"PRIu64,
			 __func__, job_ptr->job_id, tres_run_delta[i], qos->name,
			 assoc_mgr_tres_name_array[i],
			 used_limits_a->tres_run_secs[i]);
	}
}

static void _handle_assoc_tres_run_secs(long double *tres_run_decay,
					uint64_t *tres_run_delta,
					uint32_t job_id,
					slurmdb_assoc_rec_t *assoc)
{
	int i;

	if (!assoc || !(accounting_enforce & ACCOUNTING_ENFORCE_LIMITS))
		return;

	for (i=0; i<slurmctld_tres_cnt; i++) {
		if (i == TRES_ARRAY_ENERGY)
			continue;
		if (tres_run_decay)
			assoc->usage->usage_tres_raw[i] += tres_run_decay[i];

		if (tres_run_delta[i] >
		    assoc->usage->grp_used_tres_run_secs[i]) {
			error("_handle_assoc_tres_run_secs: job %u: "
			      "assoc %u TRES %s grp_used_tres_run_secs "
			      "underflow, tried to remove %"PRIu64" seconds "
			      "when only %"PRIu64" remained.",
			      job_id,
			      assoc->id,
			      assoc_mgr_tres_name_array[i],
			      tres_run_delta[i],
			      assoc->usage->grp_used_tres_run_secs[i]);
			assoc->usage->grp_used_tres_run_secs[i] = 0;
		} else
			assoc->usage->grp_used_tres_run_secs[i] -=
				tres_run_delta[i];

		log_flag(PRIO, "%s: job %u: Removed %"PRIu64" unused seconds from assoc %d TRES %s grp_used_tres_run_secs = %"PRIu64,
			 __func__, job_id, tres_run_delta[i], assoc->id,
			 assoc_mgr_tres_name_array[i],
			 assoc->usage->grp_used_tres_run_secs[i]);
	}
}

static void _handle_tres_run_secs(uint64_t *tres_run_delta,
				  job_record_t *job_ptr)
{

	slurmdb_assoc_rec_t *assoc = job_ptr->assoc_ptr;

	_handle_qos_tres_run_secs(NULL, tres_run_delta,
				  job_ptr, job_ptr->qos_ptr);

	/* Only update partition qos if not being used by job */
	if (job_ptr->part_ptr &&
	    (job_ptr->part_ptr->qos_ptr != job_ptr->qos_ptr))
		_handle_qos_tres_run_secs(NULL, tres_run_delta, job_ptr,
					  job_ptr->part_ptr->qos_ptr);

	while (assoc) {
		_handle_assoc_tres_run_secs(NULL, tres_run_delta,
					    job_ptr->job_id, assoc);
		assoc = assoc->usage->parent_assoc_ptr;
	}
}

/*
 * Remove previously used time from qos and assocs grp_used_tres_run_secs.
 *
 * When restarting slurmctld acct_policy_job_begin() is called for all
 * running jobs. There every jobs total requested trestime (tres_alloc *
 * time_limit) is added to grp_used_tres_run_secs of assocs and qos.
 *
 * This function will subtract all trestime that was used until the
 * decay thread last ran. This kludge is necessary as the decay thread
 * last_ran variable can't be accessed from acct_policy_job_begin().
 */
static void _init_grp_used_tres_run_secs(time_t last_ran)
{
	job_record_t *job_ptr = NULL;
	list_itr_t *itr;
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, WRITE_LOCK, NO_LOCK,
				   READ_LOCK, NO_LOCK, NO_LOCK };
	slurmctld_lock_t job_read_lock =
		{ NO_LOCK, READ_LOCK, NO_LOCK, NO_LOCK, NO_LOCK };
	uint64_t tres_run_delta[slurmctld_tres_cnt];
	int i;

	log_flag(PRIO, "Initializing grp_used_tres_run_secs");

	if (!(slurm_conf.accounting_storage_enforce &
	      ACCOUNTING_ENFORCE_LIMITS))
		return;
	if (!(job_list && list_count(job_list)))
		return;

	lock_slurmctld(job_read_lock);
	itr = list_iterator_create(job_list);

	assoc_mgr_lock(&locks);
	while ((job_ptr = list_next(itr))) {
		double usage_factor = 1.0;
		log_flag(PRIO, "job: %u", job_ptr->job_id);

		/* If end_time_exp is NO_VAL we have already ran the end for
		 * this job.  We don't want to do it again, so just exit.
		 */
		if (job_ptr->end_time_exp == (time_t)NO_VAL)
			continue;

		if (!IS_JOB_RUNNING(job_ptr))
			continue;

		if (job_ptr->start_time > last_ran)
			continue;

		/* apply usage factor */
		if (job_ptr->qos_ptr &&
		    (job_ptr->qos_ptr->usage_factor >= 0))
			usage_factor = job_ptr->qos_ptr->usage_factor;
		usage_factor *= (double)(last_ran - job_ptr->start_time);

		for (i=0; i<slurmctld_tres_cnt; i++) {
			if (job_ptr->tres_alloc_cnt[i] == NO_CONSUME_VAL64)
				continue;
			tres_run_delta[i] =
				job_ptr->tres_alloc_cnt[i] * usage_factor;
		}

		_handle_tres_run_secs(tres_run_delta, job_ptr);
	}
	assoc_mgr_unlock(&locks);
	list_iterator_destroy(itr);
	unlock_slurmctld(job_read_lock);
}

/* If the job is running then apply decay to the job.
 *
 * Return 0 if we don't need to process the job any further, 1 if
 * further processing is needed.
 */
static int _apply_new_usage(job_record_t *job_ptr, time_t start_period,
			    time_t end_period, bool adjust_for_end)
{
	slurmdb_qos_rec_t *qos;
	slurmdb_assoc_rec_t *assoc;
	double run_delta = 0.0, run_decay = 0.0, run_nodecay = 0.0;
	double billable_tres = 0.0;
	double real_decay = 0.0, real_nodecay = 0.0;
	uint64_t tres_run_delta[slurmctld_tres_cnt];
	long double tres_run_decay[slurmctld_tres_cnt];
	long double tres_run_nodecay[slurmctld_tres_cnt];
	uint64_t tres_time_delta = 0;
	int i;
	uint64_t job_time_limit_ends = 0;
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, WRITE_LOCK, NO_LOCK,
				   READ_LOCK, NO_LOCK, NO_LOCK };

	/* If end_time_exp is NO_VAL we have already ran the end for
	 * this job.  We don't want to do it again, so just exit.
	 */
	if (job_ptr->end_time_exp == (time_t)NO_VAL)
		return SLURM_SUCCESS;

	/* Even if job_ptr->qos_ptr->usage_factor is 0 we need to
	 * handle other non-usage variables here
	 * (grp_used_tres_run_secs), so don't return.
	 */

	if (job_ptr->start_time > start_period)
		start_period = job_ptr->start_time;

	/* Only change the end_time if we are at the end of the job.
	 * If the job is running over the time limit the end_time will
	 * be in the past.
	 */
	if (job_ptr->end_time && adjust_for_end
	    && (end_period > job_ptr->end_time))
		end_period = job_ptr->end_time;

	run_delta = difftime(end_period, start_period);

	/* Even if run_delta is 0 we need to
	 * handle other non-usage variables here
	 * (grp_used_tres_run_secs), so don't return.
	 */
	if (run_delta < 0)
		run_delta = 0;

	/* tres_run_delta will is used to
	 * decrease qos and assocs
	 * grp_used_tres_run_secs values. When
	 * a job is started only seconds until
	 * start_time+time_limit is added, so
	 * for jobs running over their
	 * timelimit we should only subtract
	 * the used time until the time limit. */
	job_time_limit_ends =
		(uint64_t)job_ptr->start_time +
		(uint64_t)job_ptr->time_limit * 60;

	if ((uint64_t)start_period >= job_time_limit_ends)
		tres_time_delta = 0;
	else if (IS_JOB_FINISHED(job_ptr) || IS_JOB_COMPLETING(job_ptr) ||
		 IS_JOB_RESIZING(job_ptr)) {
		/* If a job is being requeued sometimes the state will
		   be pending + completing so handle that the same as
		   finished so we don't leave time in the mix.
		*/
		tres_time_delta = (job_time_limit_ends -
				   (uint64_t)start_period);
	} else if (end_period > job_ptr->end_time_exp) {
		int end_exp = difftime(job_ptr->end_time_exp, start_period);

		if (end_exp > 0)
			tres_time_delta = (uint64_t)end_exp;
	} else
		tres_time_delta = run_delta;

	/* make sure we only run through this once at the end */
	if (adjust_for_end)
		job_ptr->end_time_exp = (time_t)NO_VAL;

	if (slurm_conf.debug_flags & DEBUG_FLAG_PRIO) {
		info("job %u ran for %g seconds with TRES counts of",
		     job_ptr->job_id, run_delta);
		if (job_ptr->tres_alloc_cnt) {
			for (i=0; i<slurmctld_tres_cnt; i++) {
				if (!job_ptr->tres_alloc_cnt[i] ||
				    (job_ptr->tres_alloc_cnt[i] ==
				     NO_CONSUME_VAL64))
					continue;
				info("TRES %s: %"PRIu64,
				     assoc_mgr_tres_name_array[i],
				     job_ptr->tres_alloc_cnt[i]);
			}
		} else
			info("No allocated TRES, state is %s",
			     job_state_string(job_ptr->job_state));
	}
	/* get the time in decayed fashion */
	run_decay = run_delta * pow(decay_factor, run_delta);
	/* clang needs these memset to avoid a warning */
	memset(tres_run_decay, 0, sizeof(tres_run_decay));
	memset(tres_run_nodecay, 0, sizeof(tres_run_nodecay));
	memset(tres_run_delta, 0, sizeof(tres_run_delta));
	assoc_mgr_lock(&locks);

	billable_tres = calc_job_billable_tres(job_ptr, start_period, true);
	real_decay    = run_decay * billable_tres;
	real_nodecay  = run_delta * billable_tres;
	run_nodecay   = run_delta;

	qos = job_ptr->qos_ptr;
	if (qos && (qos->usage_factor >= 0)) {
		real_decay *= qos->usage_factor;
		run_decay  *= qos->usage_factor;
		real_nodecay *= qos->usage_factor;
		run_nodecay  *= qos->usage_factor;

		tres_time_delta *= qos->usage_factor;
	}
	if (job_ptr->tres_alloc_cnt) {
		for (i=0; i<slurmctld_tres_cnt; i++) {
			if (!job_ptr->tres_alloc_cnt[i] ||
			    (job_ptr->tres_alloc_cnt[i] == NO_CONSUME_VAL64))
				continue;
			tres_run_delta[i] = tres_time_delta *
				job_ptr->tres_alloc_cnt[i];
			tres_run_decay[i] = (long double)run_decay *
				(long double)job_ptr->tres_alloc_cnt[i];
			tres_run_nodecay[i] = (long double)run_nodecay *
				(long double)job_ptr->tres_alloc_cnt[i];
		}
	}

	assoc = job_ptr->assoc_ptr;

	/* now apply the usage factor for this qos */
	if (qos) {
		if (qos->flags & QOS_FLAG_NO_DECAY) {
			qos->usage->grp_used_wall += run_nodecay;
			qos->usage->usage_raw += (long double)real_nodecay;

			_handle_qos_tres_run_secs(tres_run_nodecay,
						  tres_run_delta,
						  job_ptr, qos);
		} else {
			qos->usage->grp_used_wall += run_decay;
			qos->usage->usage_raw += (long double)real_decay;

			_handle_qos_tres_run_secs(tres_run_decay,
						  tres_run_delta,
						  job_ptr, qos);
		}
	}

	/* sanity check, there should always be a part_ptr here, but only do
	 * the qos if it isn't the same qos as the job is using */
	if (job_ptr->part_ptr && (job_ptr->part_ptr->qos_ptr != qos))
		qos = job_ptr->part_ptr->qos_ptr;
	else
		qos = NULL;

	/* now apply the usage factor for this qos */
	if (qos) {
		/* usage factor only matter's on the job qos */
		/* if (qos->usage_factor >= 0) { */
		/* 	real_decay *= qos->usage_factor; */
		/* 	run_decay *= qos->usage_factor; */
		/* } */
		if (qos->flags & QOS_FLAG_NO_DECAY) {
			qos->usage->grp_used_wall += run_nodecay;
			qos->usage->usage_raw += (long double)real_nodecay;

			_handle_qos_tres_run_secs(tres_run_nodecay,
						  tres_run_delta,
						  job_ptr, qos);
		} else {
			qos->usage->grp_used_wall += run_decay;
			qos->usage->usage_raw += (long double)real_decay;

			_handle_qos_tres_run_secs(tres_run_decay,
						  tres_run_delta,
						  job_ptr, qos);
		}
	}


	/* We want to do this all the way up
	 * to and including root.  This way we
	 * can keep track of how much usage
	 * has occurred on the entire system
	 * and use that to normalize against. */
	while (assoc) {
		assoc->usage->grp_used_wall += run_decay;
		assoc->usage->usage_raw += (long double)real_decay;
		log_flag(PRIO, "Adding %f new usage to assoc %u (%s/%s/%s) raw usage is now %Lf. Group wall added %f making it %f.",
			 real_decay, assoc->id, assoc->acct, assoc->user,
			 assoc->partition, assoc->usage->usage_raw, run_decay,
			 assoc->usage->grp_used_wall);
		_handle_assoc_tres_run_secs(tres_run_decay, tres_run_delta,
					    job_ptr->job_id, assoc);

		assoc = assoc->usage->parent_assoc_ptr;
	}
	assoc_mgr_unlock(&locks);
	return 1;
}


static int _decay_apply_new_usage_and_weighted_factors(job_record_t *job_ptr,
						       time_t *start_time_ptr)
{
	/* Always return SUCCESS so that list_for_each will
	 * continue processing list of jobs. */

	if (!decay_apply_new_usage(job_ptr, start_time_ptr))
		return SLURM_SUCCESS;

	decay_apply_weighted_factors(job_ptr, start_time_ptr);

	return SLURM_SUCCESS;
}


static void *_decay_thread(void *no_data)
{
	time_t start_time = time(NULL);
	time_t next_reset = 0;
	double decay_hl = (double) slurm_conf.priority_decay_hl;
	uint16_t reset_period = slurm_conf.priority_reset_period;

	time_t now;
	double run_delta = 0.0, real_decay = 0.0;
	struct timeval tvnow;
	struct timespec abs;

	/* Write lock on jobs, read lock on nodes and partitions */
	slurmctld_lock_t job_write_lock =
		{ NO_LOCK, WRITE_LOCK, READ_LOCK, READ_LOCK, NO_LOCK };
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, NO_LOCK, NO_LOCK,
				   NO_LOCK, NO_LOCK, NO_LOCK };

#if HAVE_SYS_PRCTL_H
	if (prctl(PR_SET_NAME, "decay", NULL, NULL, NULL) < 0) {
		error("%s: cannot set my name to %s %m", __func__, "decay");
	}
#endif

	/* setup timer */
	gettimeofday(&tvnow, NULL);
	abs.tv_sec = tvnow.tv_sec;
	abs.tv_nsec = tvnow.tv_usec * 1000;

	if (g_last_reset == 0)
		g_last_reset = start_time;

	_init_grp_used_tres_run_secs(g_last_ran);

	while (!plugin_shutdown) {
		now = start_time;

		slurm_mutex_lock(&decay_lock);
		running_decay = 1;

		/* If reconfig is called handle all that happens
		   outside of the loop here */
		if (reconfig) {
			/* if decay_hl is 0 or less that means no
			   decay is to be had.  This also means we
			   flush the used time at a certain time
			   set by PriorityUsageResetPeriod in the slurm.conf
			*/
			reset_period = slurm_conf.priority_reset_period;
			next_reset = 0;
			decay_hl = (double) slurm_conf.priority_decay_hl;
			if (decay_hl > 0)
				decay_factor = 1 - (0.693 / decay_hl);
			else
				decay_factor = 1;

			reconfig = 0;
		}

		/* this needs to be done right away so as to
		 * incorporate it into the decay loop.
		 */
		switch(reset_period) {
		case PRIORITY_RESET_NONE:
			break;
		case PRIORITY_RESET_NOW:	/* do once */
			_reset_usage();
			reset_period = PRIORITY_RESET_NONE;
			g_last_reset = now;
			break;
		case PRIORITY_RESET_DAILY:
		case PRIORITY_RESET_WEEKLY:
		case PRIORITY_RESET_MONTHLY:
		case PRIORITY_RESET_QUARTERLY:
		case PRIORITY_RESET_YEARLY:
			if (next_reset == 0) {
				next_reset = _next_reset(reset_period,
							 g_last_reset);
			}
			if (now >= next_reset) {
				_reset_usage();
				g_last_reset = next_reset;
				next_reset = _next_reset(reset_period,
							 g_last_reset);
			}
		}

		/* Calculate all the normalized usage unless this is Fair Tree;
		 * it handles these calculations during its tree traversal */
		if (!(flags & PRIORITY_FLAGS_FAIR_TREE)) {
			assoc_mgr_lock(&locks);
			_set_children_usage_efctv(
				assoc_mgr_root_assoc->usage->children_list);
			assoc_mgr_unlock(&locks);
		}

		if (!g_last_ran)
			goto get_usage;
		else
			run_delta = difftime(start_time, g_last_ran);

		if (run_delta <= 0)
			goto get_usage;
		real_decay = pow(decay_factor, (double)run_delta);

		if (real_decay < DBL_MIN)
			real_decay = DBL_MIN;

		log_flag(PRIO, "Decay factor over %g seconds goes from %.15f -> %.15f",
			 run_delta, decay_factor, real_decay);

		/* first apply decay to used time */
		if (_apply_decay(real_decay) != SLURM_SUCCESS) {
			error("priority/multifactor: problem applying decay");
			running_decay = 0;
			slurm_mutex_unlock(&decay_lock);
			break;
		}

		lock_slurmctld(job_write_lock);

		/*
		 * Give the site_factor plugin a chance to update the
		 * site_factor value if desired.
		 */
		site_factor_g_update();

		if (!(flags & PRIORITY_FLAGS_FAIR_TREE)) {
			list_for_each(
				job_list,
				(ListForF) _decay_apply_new_usage_and_weighted_factors,
				&start_time
				);
		}

		unlock_slurmctld(job_write_lock);

	get_usage:
		if (flags & PRIORITY_FLAGS_FAIR_TREE)
			fair_tree_decay(job_list, start_time);

		g_last_ran = start_time;

		_write_last_decay_ran(g_last_ran, g_last_reset);

		running_decay = 0;

		if (!plugin_shutdown) {
			/* Sleep until the next time. */
			abs.tv_sec += slurm_conf.priority_calc_period;
			slurm_cond_timedwait(&decay_cond, &decay_lock, &abs);
			start_time = time(NULL);
			/* repeat ;) */
		}
		slurm_mutex_unlock(&decay_lock);

	}

	return NULL;
}

static int _create_prio_list_qos(void *x, void *arg)
{
	slurmdb_qos_rec_t *qos_ptr = x;
	create_prio_list_t *prio_list = arg;
	part_record_t *part_ptr = prio_list->part_ptr;
	job_record_t *job_ptr = prio_list->job_ptr;

	priority_factors_object_t *obj =
		xmalloc(sizeof(priority_factors_object_t));

	if (!prio_list->ret_list)
		prio_list->ret_list =
			list_create(_destroy_priority_factors_obj_light);
	list_append(prio_list->ret_list, obj);

	/*
	 * Don't xstrdup dup anything here, the list is freed
	 * with _destroy_priority_factors_obj_light.
	 */
	obj->account = job_ptr->account;
	obj->job_id = job_ptr->job_id;
	obj->partition = part_ptr ? part_ptr->name : job_ptr->part_ptr->name;
	obj->qos = qos_ptr ? qos_ptr->name :
		job_ptr->qos_ptr ? job_ptr->qos_ptr->name : NULL;
	obj->user_id = job_ptr->user_id;

	if (job_ptr->direct_set_prio) {
		obj->direct_prio = job_ptr->priority;
	} else {
		obj->prio_factors = xmalloc(sizeof(priority_factors_t));
		slurm_copy_priority_factors(obj->prio_factors,
					    job_ptr->prio_factors);

		/* This portion is only needed for multi-partition jobs */
		if (part_ptr) {
			obj->prio_factors->priority_part =
				((flags & PRIORITY_FLAGS_NO_NORMAL_PART) ?
				 part_ptr->priority_job_factor :
				 part_ptr->norm_priority) *
				(double)weight_part;
			if (obj->prio_factors->priority_tres) {
				_get_tres_factors(
					job_ptr, part_ptr,
					obj->prio_factors->priority_tres);
				_get_tres_prio_weighted(
					obj->prio_factors->priority_tres);
			}
		}

		if (qos_ptr && qos_ptr->priority && weight_qos) {
			obj->prio_factors->priority_qos =
				((flags & PRIORITY_FLAGS_NO_NORMAL_QOS) ?
				 qos_ptr->priority :
				 qos_ptr->usage->norm_priority) *
				(double) weight_qos;
		}

	}

	return SLURM_SUCCESS;
}

static int _create_prio_list_part(void *x, void *arg)
{
	create_prio_list_t *prio_list = arg;
	job_record_t *job_ptr = prio_list->job_ptr;
	prio_list->part_ptr = x;

	if (job_ptr->qos_list)
		(void) list_for_each(job_ptr->qos_list,
				     _create_prio_list_qos,
				     prio_list);
	else
		(void) _create_prio_list_qos(NULL, prio_list);

	return SLURM_SUCCESS;
}

static void _internal_setup(void)
{
	damp_factor = (long double) slurm_conf.fs_dampening_factor;
	max_age = slurm_conf.priority_max_age;
	weight_age = slurm_conf.priority_weight_age;
	weight_assoc = slurm_conf.priority_weight_assoc;
	weight_fs = slurm_conf.priority_weight_fs;
	weight_js = slurm_conf.priority_weight_js;
	weight_part = slurm_conf.priority_weight_part;
	weight_qos = slurm_conf.priority_weight_qos;
	xfree(weight_tres);
	weight_tres = slurm_get_tres_weight_array(
		slurm_conf.priority_weight_tres, slurmctld_tres_cnt, true);
	flags = slurm_conf.priority_flags;

	log_flag(PRIO, "priority: Damp Factor is %u", damp_factor);
	log_flag(PRIO, "priority: AccountingStorageEnforce is %u",
		 slurm_conf.accounting_storage_enforce);
	log_flag(PRIO, "priority: Max Age is %u", max_age);
	log_flag(PRIO, "priority: Weight Age is %u", weight_age);
	log_flag(PRIO, "priority: Weight Assoc is %u", weight_assoc);
	log_flag(PRIO, "priority: Weight Fairshare is %u", weight_fs);
	log_flag(PRIO, "priority: Weight JobSize is %u", weight_js);
	log_flag(PRIO, "priority: Weight Part is %u", weight_part);
	log_flag(PRIO, "priority: Weight QOS is %u", weight_qos);
	log_flag(PRIO, "priority: Flags is %u", flags);
}


/* Recursively call assoc_mgr_normalize_assoc_shares from assoc_mgr.c on
 * children of an assoc
 */
static void _set_norm_shares(list_t *children_list)
{
	list_itr_t *itr = NULL;
	slurmdb_assoc_rec_t *assoc = NULL;

	if (!children_list || list_is_empty(children_list))
		return;

	itr = list_iterator_create(children_list);
	while ((assoc = list_next(itr))) {
		assoc_mgr_normalize_assoc_shares(assoc);
		if (!assoc->user)
			_set_norm_shares(assoc->usage->children_list);
	}

	list_iterator_destroy(itr);
}

/* We need to set up some global variables that are needed outside of the
* decay_thread (i.e. decay_factor, g_last_ran). These are needed if a job was
* completing and the slurmctld was reset. If they aren't setup before
* continuing we could get more time added than should be on a restart.
*/
static void _init_decay_vars()
{
	double decay_hl = (double) slurm_conf.priority_decay_hl;
	/*
	* DECAY_FACTOR DESCRIPTION:
	*
	* The decay thread applies an exponential decay over the past
	* consumptions using a rolling approach.
	* Every calc period p in seconds, the already computed usage is
	* computed again applying the decay factor of that slice :
	* decay_factor_slice.
	*
	* To ease the computation, the notion of decay_factor
	* is introduced and corresponds to the decay factor
	* required for a slice of 1 second. Thus, for any given
	* slice of time of n seconds, decay_factor_slice will be
	* defined as : df_slice = pow(df,n)
	*
	* For a slice corresponding to the defined half life 'decay_hl' and
	* a usage x, we will therefore have :
	*    >>  x * pow(decay_factor,decay_hl) = 1/2 x  <<
	*
	* This expression helps to define the value of decay_factor that
	* is necessary to apply the previously described logic.
	*
	* The expression is equivalent to :
	*    >> decay_hl * ln(decay_factor) = ln(1/2)
	*    >> ln(decay_factor) = ln(1/2) / decay_hl
	*    >> decay_factor = e( ln(1/2) / decay_hl )
	*
	* Applying the power series e(x) = sum(x^n/n!) for n from 0 to infinity
	*    >> decay_factor = 1 + ln(1/2)/decay_hl
	*    >> decay_factor = 1 - ( 0.693 / decay_hl)
	*
	* This explains the following declaration.
	*/
	if (decay_hl > 0)
		decay_factor = 1 - (0.693 / decay_hl);
	_read_last_decay_ran(&g_last_ran, &g_last_reset);
}

static void _depth_oblivious_set_usage_efctv(slurmdb_assoc_rec_t *assoc)
{
	long double ratio_p, ratio_l, k, f, ratio_s;
	slurmdb_assoc_rec_t *parent_assoc = NULL;
	list_itr_t *sib_itr = NULL;
	slurmdb_assoc_rec_t *sibling = NULL;
	char *child;
	char *child_str;

	if (assoc->user) {
		child = "user";
		child_str = assoc->user;
	} else {
		child = "account";
		child_str = assoc->acct;
	}

	/* We want priority_fs = pow(2.0, -R); where
	   R = ratio_p * ratio_l^k
	*/

	/* ratio_p is R for our parent */

	/* ratio_l is our usage ratio r divided by ratio_s,
	 * the usage ratio of our siblings (including
	 * ourselves). In the standard case where everything
	 * is consumed at the leaf accounts ratio_s=ratio_p
	 */

	/* k is a factor which tends towards 0 when ratio_p
	   diverges from 1 and ratio_l would bring back R
	   towards 1
	*/

	/* Effective usage is now computed to be R*shares_norm
	   so that the general formula of
	   priority_fs = pow(2.0, -(usage_efctv / shares_norm))
	   gives what we want: priority_fs = pow(2.0, -R);
	*/

	f = 5.0; /* FIXME: This could be a tunable parameter
		    (higher f means more impact when parent consumption
		    is inadequate) */
	parent_assoc =  assoc->usage->fs_assoc_ptr;

	if (assoc->usage->shares_norm &&
	    parent_assoc->usage->shares_norm &&
	    parent_assoc->usage->usage_efctv &&
	    assoc->usage->usage_norm) {
		ratio_p = (parent_assoc->usage->usage_efctv /
			   parent_assoc->usage->shares_norm);

		ratio_s = 0;
		sib_itr = list_iterator_create(
			parent_assoc->usage->children_list);
		while ((sibling = list_next(sib_itr))) {
			if(sibling->shares_raw != SLURMDB_FS_USE_PARENT)
				ratio_s += sibling->usage->usage_norm;
		}
		list_iterator_destroy(sib_itr);
		ratio_s /= parent_assoc->usage->shares_norm;

		ratio_l = (assoc->usage->usage_norm /
			   assoc->usage->shares_norm) / ratio_s;
#if defined(__FreeBSD__)
		if (!ratio_p || !ratio_l
		    || log(ratio_p) * log(ratio_l) >= 0) {
			k = 1;
		} else {
			k = 1 / (1 + pow(f * log(ratio_p), 2));
		}

		assoc->usage->usage_efctv =
			ratio_p * pow(ratio_l, k) *
			assoc->usage->shares_norm;
#else
		if (!ratio_p || !ratio_l
		    || logl(ratio_p) * logl(ratio_l) >= 0) {
			k = 1;
		} else {
			k = 1 / (1 + powl(f * logl(ratio_p), 2));
		}

		assoc->usage->usage_efctv =
			ratio_p * pow(ratio_l, k) *
			assoc->usage->shares_norm;
#endif

		log_flag(PRIO, "Effective usage for %s %s off %s(%s) (%Lf * %Lf ^ %Lf) * %f  = %Lf",
			 child, child_str, assoc->usage->parent_assoc_ptr->acct,
			 assoc->usage->fs_assoc_ptr->acct, ratio_p, ratio_l, k,
			 assoc->usage->shares_norm, assoc->usage->usage_efctv);
	} else {
		assoc->usage->usage_efctv = assoc->usage->usage_norm;
		log_flag(PRIO, "Effective usage for %s %s off %s(%s) %Lf",
			 child, child_str, assoc->usage->parent_assoc_ptr->acct,
			 assoc->usage->fs_assoc_ptr->acct,
			 assoc->usage->usage_efctv);
	}
}

static void _set_usage_efctv(slurmdb_assoc_rec_t *assoc)
{
	/* Variable names taken from HTML documentation */
	long double ua_child = assoc->usage->usage_norm;
	long double ue_parent =
		assoc->usage->fs_assoc_ptr->usage->usage_efctv;
	uint32_t s_child = assoc->shares_raw;
	uint32_t s_all_siblings = assoc->usage->level_shares;

	/* If no user in the account has shares, avoid division by zero by
	 * setting usage_efctv to the parent's usage_efctv */
	if (!s_all_siblings)
		assoc->usage->usage_efctv = ue_parent;
	else
		assoc->usage->usage_efctv = ua_child +
			(ue_parent - ua_child) *
			(s_child / (long double) s_all_siblings);
}

extern int init(void)
{
	/* Write lock on jobs, read lock on nodes and partitions */

	/* This means we aren't running from the controller so skip setup. */
	if (cluster_cpus == NO_VAL) {
		damp_factor = (long double) slurm_conf.fs_dampening_factor;
		return SLURM_SUCCESS;
	}

	if (site_factor_g_init() != SLURM_SUCCESS)
		fatal("Failed to initialize site_factor plugin.");

	_internal_setup();

	/* Check to see if we are running a supported accounting plugin */
	if (!slurm_with_slurmdbd()) {
		if (weight_age)
			error("PriorityWeightAge can only be used with SlurmDBD, ignoring");
		if (weight_fs)
			error("PriorityWeightFairshare can only be used with SlurmDBD, ignoring");
		calc_fairshare = 0;
		weight_age = 0;
		weight_fs = 0;
	} else if (assoc_mgr_root_assoc) {
		_init_decay_vars();
		assoc_mgr_root_assoc->usage->usage_efctv = 1.0;
	} else {
		if (weight_fs) {
			fatal("It appears you don't have any association "
			      "data from your database.  "
			      "The priority/multifactor plugin requires "
			      "this information to run correctly.  Please "
			      "check your database connection and try again.");
		}
		calc_fairshare = 0;
	}

	debug("%s loaded", plugin_name);
	return SLURM_SUCCESS;
}

extern void fini(void)
{
	plugin_shutdown = time(NULL);

	/* Daemon termination handled here */
	if (running_decay)
		debug("Waiting for priority decay thread to finish.");

	slurm_mutex_lock(&decay_lock);

	/* signal the decay thread to end */
	if (decay_handler_thread)
		slurm_cond_signal(&decay_cond);

	xfree(weight_tres);

	slurm_mutex_unlock(&decay_lock);

	/* Now join outside the lock */
	slurm_thread_join(decay_handler_thread);

	site_factor_g_fini();
}

void priority_p_thread_start(void)
{
	slurm_thread_create(&decay_handler_thread, _decay_thread, NULL);

	return;
}

extern uint32_t priority_p_set(uint32_t last_prio, job_record_t *job_ptr)
{
	uint32_t priority;

	/*
	 * Run this first so any change to site_factor will be
	 * included in the summation done inside _get_priority_internal().
	 */
	site_factor_g_set(job_ptr);

	priority = _get_priority_internal(time(NULL), job_ptr);

	debug2("initial priority for job %u is %u", job_ptr->job_id, priority);

	return priority;
}

extern void priority_p_reconfig(bool assoc_clear)
{
	assoc_mgr_lock_t locks = { WRITE_LOCK, NO_LOCK, NO_LOCK, NO_LOCK,
				   NO_LOCK, NO_LOCK, NO_LOCK };

	reconfig = 1;
	_internal_setup();

	/* Since Fair Tree uses a different shares calculation method, we
	 * must reassign shares at reconfigure if the algorithm was switched to
	 * or from Fair Tree */
	if ((flags & PRIORITY_FLAGS_FAIR_TREE) !=
	    (slurm_conf.priority_flags & PRIORITY_FLAGS_FAIR_TREE)) {
		assoc_mgr_lock(&locks);
		_set_norm_shares(assoc_mgr_root_assoc->usage->children_list);
		assoc_mgr_unlock(&locks);
	}

	flags = slurm_conf.priority_flags;

	/* Since the used_cpu_run_secs has been reset by the reconfig,
	 * we need to remove the time that has past since the last
	 * poll.  We can't just do the correct calculation in the
	 * first place because it will mess up everything in the poll
	 * since it is based off the g_last_ran time.
	 */
	if (assoc_clear)
		_init_grp_used_tres_run_secs(g_last_ran);

	debug2("%s reconfigured", plugin_name);

	return;
}


extern void set_assoc_usage_norm(slurmdb_assoc_rec_t *assoc)
{
	/* If root usage is 0, there is no usage anywhere. */
	if (!assoc_mgr_root_assoc->usage->usage_raw) {
		assoc->usage->usage_norm = 0L;
		return;
	}

	assoc->usage->usage_norm = assoc->usage->usage_raw
		/ assoc_mgr_root_assoc->usage->usage_raw;


	/* This is needed in case someone changes the half-life on the
	 * fly and now we have used more time than is available under
	 * the new config */
	if (assoc->usage->usage_norm > 1L)
		assoc->usage->usage_norm = 1L;
}


extern void priority_p_set_assoc_usage(slurmdb_assoc_rec_t *assoc)
{
	xassert(assoc_mgr_root_assoc);
	xassert(assoc);
	xassert(assoc->usage);
	xassert(assoc->usage->fs_assoc_ptr);

	set_assoc_usage_norm(assoc);
	_set_assoc_usage_efctv(assoc);

	if (slurm_conf.debug_flags & DEBUG_FLAG_PRIO)
		_priority_p_set_assoc_usage_debug(assoc);
}


extern double priority_p_calc_fs_factor(long double usage_efctv,
					long double shares_norm)
{
	double priority_fs = 0.0;

	if (fuzzy_equal(usage_efctv, NO_VAL))
		return priority_fs;

	if (shares_norm <= 0)
		return priority_fs;

	priority_fs = pow(2.0, -((usage_efctv/shares_norm) / damp_factor));

	return priority_fs;
}

extern list_t *priority_p_get_priority_factors_list(uid_t uid)
{
	list_itr_t *itr;
	job_record_t *job_ptr = NULL;
	time_t start_time = time(NULL);
	create_prio_list_t prio_list = { 0 };

	xassert(verify_lock(JOB_LOCK, READ_LOCK));
	xassert(verify_lock(NODE_LOCK, READ_LOCK));
	xassert(verify_lock(PART_LOCK, READ_LOCK));

	if (job_list && list_count(job_list)) {
		time_t use_time;

		itr = list_iterator_create(job_list);
		while ((job_ptr = list_next(itr))) {
			if (!(flags & PRIORITY_FLAGS_CALCULATE_RUNNING) &&
			    !IS_JOB_PENDING(job_ptr))
				continue;

			/* Job is not active on this cluster. */
			if (IS_JOB_REVOKED(job_ptr))
				continue;

			/*
			 * This means the job is not eligible yet
			 */
			if (flags & PRIORITY_FLAGS_ACCRUE_ALWAYS)
				use_time = job_ptr->details->submit_time;
			else
				use_time = job_ptr->details->begin_time;

			if (!use_time || (use_time > start_time))
				continue;

			/*
			 * 0 means the job is held
			 */
			if (job_ptr->priority == 0)
				continue;

			if ((slurm_conf.private_data & PRIVATE_DATA_JOBS) &&
			    (job_ptr->user_id != uid) &&
			    !validate_operator(uid) &&
			    (((slurm_mcs_get_privatedata() == 0) &&
			      !assoc_mgr_is_user_acct_coord(acct_db_conn, uid,
			                                    job_ptr->account,
							    false)) ||
			     ((slurm_mcs_get_privatedata() == 1) &&
			      (mcs_g_check_mcs_label(uid, job_ptr->mcs_label,
						     false) != 0))))
				continue;

			/*
			 * Job is not in any partition, so there is nothing to
			 * return. This can happen if the Partition was deleted,
			 * CALCULATE_RUNNING is enabled, and this job is still
			 * waiting out MinJobAge before being removed from the
			 * system.
			 */
			if (!job_ptr->part_ptr && !job_ptr->part_ptr_list)
				continue;

			prio_list.job_ptr = job_ptr;

			/* Job in one partition */
			if (!job_ptr->part_ptr_list) {
				_create_prio_list_part(NULL, &prio_list);
			} else {
				list_for_each(job_ptr->part_ptr_list,
					      _create_prio_list_part,
					      &prio_list);
			}
		}
		list_iterator_destroy(itr);
		if (!list_count(prio_list.ret_list))
			FREE_NULL_LIST(prio_list.ret_list);
	}

	return prio_list.ret_list;
}

/* at least slurmctld_lock_t job_write_lock = { NO_LOCK, WRITE_LOCK,
 * READ_LOCK, READ_LOCK, NO_LOCK }; should be locked before calling this */
extern void priority_p_job_end(job_record_t *job_ptr)
{
	log_flag(PRIO, "%s: called for job %u", __func__, job_ptr->job_id);

	_apply_new_usage(job_ptr, g_last_ran, time(NULL), 1);
}

extern bool decay_apply_new_usage(job_record_t *job_ptr,
				  time_t *start_time_ptr)
{

	/* Don't need to handle finished jobs. */
	if (IS_JOB_FINISHED(job_ptr) || IS_JOB_COMPLETING(job_ptr))
		return false;

	/* apply new usage */
	if (((flags & PRIORITY_FLAGS_CALCULATE_RUNNING) ||
	     !IS_JOB_PENDING(job_ptr)) &&
	    !IS_JOB_POWER_UP_NODE(job_ptr) &&
	    job_ptr->start_time && job_ptr->assoc_ptr) {
		if (!_apply_new_usage(job_ptr, g_last_ran, *start_time_ptr, 0))
			return false;
	}
	return true;
}


extern int decay_apply_weighted_factors(job_record_t *job_ptr,
					time_t *start_time_ptr)
{
	uint32_t new_prio;

	/* Always return SUCCESS so that list_for_each will
	 * continue processing list of jobs. */

	/*
	 * Priority 0 is reserved for held jobs. Also skip priority
	 * re_calculation for non-pending jobs.
	 */
	if ((job_ptr->priority == 0) ||
	    IS_JOB_POWER_UP_NODE(job_ptr) ||
	    (!IS_JOB_PENDING(job_ptr) &&
	     !(flags & PRIORITY_FLAGS_CALCULATE_RUNNING)))
		return SLURM_SUCCESS;

	new_prio = _get_priority_internal(*start_time_ptr, job_ptr);
	if (((flags & PRIORITY_FLAGS_INCR_ONLY) == 0) ||
	    (job_ptr->priority < new_prio)) {
		job_ptr->priority = new_prio;
		last_job_update = time(NULL);
	}

	debug2("priority for job %u is now %u",
	       job_ptr->job_id, job_ptr->priority);

	return SLURM_SUCCESS;
}

extern uint32_t priority_p_recover(uint32_t prio_boost)
{
	time_t start_time;
	slurmctld_lock_t job_write_lock = {
		.job = WRITE_LOCK,
		.node = READ_LOCK,
		.part = READ_LOCK,
	};

	if (slurm_with_slurmdbd())
		return 0;

	start_time = time(NULL);
	/* Initialize job priority factors for valid sprio output */
	lock_slurmctld(job_write_lock);
	list_for_each(job_list,
		      (ListForF) _decay_apply_new_usage_and_weighted_factors,
		      &start_time);
	unlock_slurmctld(job_write_lock);

	return 0;
}

extern void set_priority_factors(time_t start_time, job_record_t *job_ptr)
{
	assoc_mgr_lock_t locks = { .assoc = READ_LOCK, .qos = READ_LOCK };

	xassert(job_ptr);

	if (!job_ptr->prio_factors) {
		job_ptr->prio_factors =
			xmalloc(sizeof(priority_factors_t));
	} else {
		xfree(job_ptr->prio_factors->tres_weights);
		xfree(job_ptr->prio_factors->priority_tres);
		memset(job_ptr->prio_factors, 0, sizeof(priority_factors_t));
	}

	if (weight_age && job_ptr->details->accrue_time) {
		uint32_t diff = 0;

		/*
		 * Only really add an age priority if the
		 * job_ptr->details->accrue_time is past the start_time.
		 */
		if (start_time > job_ptr->details->accrue_time)
			diff = start_time - job_ptr->details->accrue_time;

		if (diff < max_age)
			job_ptr->prio_factors->priority_age =
				(double)diff / (double)max_age;
		else
			job_ptr->prio_factors->priority_age = 1.0;
	}

	if (job_ptr->assoc_ptr && weight_fs) {
		job_ptr->prio_factors->priority_fs =
			_get_fairshare_priority(job_ptr);
	}

	/* FIXME: this should work off the product of TRESBillingWeights */
	if (weight_js && active_node_record_count && cluster_cpus) {
		uint32_t cpu_cnt = 0, min_nodes = 1;
		int node_count = active_node_record_count;

		/* On the initial run of this we don't have total_cpus
		   so go off the requesting.  After the first shot
		   total_cpus should be filled in.
		*/
		if (job_ptr->total_cpus)
			cpu_cnt = job_ptr->total_cpus;
		else if (job_ptr->details
			 && (job_ptr->details->max_cpus != NO_VAL))
			cpu_cnt = job_ptr->details->max_cpus;
		else if (job_ptr->details && job_ptr->details->min_cpus)
			cpu_cnt = job_ptr->details->min_cpus;
		if (job_ptr->details)
			min_nodes = job_ptr->details->min_nodes;

		if (flags & PRIORITY_FLAGS_SIZE_RELATIVE) {
			uint32_t time_limit = 1;
			/* Job size in CPUs (based upon average CPUs/Node */
			job_ptr->prio_factors->priority_js =
				(double)min_nodes *
				(double)cluster_cpus /
				(double)node_count;
			if (cpu_cnt > job_ptr->prio_factors->priority_js) {
				job_ptr->prio_factors->priority_js =
					(double)cpu_cnt;
			}
			/* Divide by job time limit */
			if (job_ptr->time_limit != NO_VAL)
				time_limit = job_ptr->time_limit;
			else if (job_ptr->part_ptr)
				time_limit = job_ptr->part_ptr->max_time;
			job_ptr->prio_factors->priority_js /= time_limit;
			/* Normalize to max value of 1.0 */
			job_ptr->prio_factors->priority_js /= cluster_cpus;
			if (slurm_conf.priority_favor_small) {
				job_ptr->prio_factors->priority_js =
					(double) 1.0 -
					job_ptr->prio_factors->priority_js;
			}
		} else if (slurm_conf.priority_favor_small) {
			if (node_count > min_nodes)
				job_ptr->prio_factors->priority_js =
					(double) (node_count - min_nodes) /
					(double) node_count;
			else
				job_ptr->prio_factors->priority_js = 0;
			if (cpu_cnt) {
				job_ptr->prio_factors->priority_js +=
					(double)(cluster_cpus - cpu_cnt)
					/ (double)cluster_cpus;
				job_ptr->prio_factors->priority_js /= 2;
			}
		} else {	/* favor large */
			job_ptr->prio_factors->priority_js =
				(double) min_nodes / (double) node_count;
			if (cpu_cnt) {
				job_ptr->prio_factors->priority_js +=
					(double)cpu_cnt / (double)cluster_cpus;
				job_ptr->prio_factors->priority_js /= 2;
			}
		}
		if (job_ptr->prio_factors->priority_js < .0)
			job_ptr->prio_factors->priority_js = 0.0;
		else if (job_ptr->prio_factors->priority_js > 1.0)
			job_ptr->prio_factors->priority_js = 1.0;
	}

	if (job_ptr->part_ptr && job_ptr->part_ptr->priority_job_factor &&
	    weight_part) {
		job_ptr->prio_factors->priority_part =
			(flags & PRIORITY_FLAGS_NO_NORMAL_PART) ?
			job_ptr->part_ptr->priority_job_factor :
			job_ptr->part_ptr->norm_priority;
	}

	job_ptr->prio_factors->priority_site = job_ptr->site_factor;

	assoc_mgr_lock(&locks);
	if (job_ptr->assoc_ptr && weight_assoc)
		job_ptr->prio_factors->priority_assoc =
			(flags & PRIORITY_FLAGS_NO_NORMAL_ASSOC) ?
			job_ptr->assoc_ptr->priority :
			job_ptr->assoc_ptr->usage->priority_norm;

	if (job_ptr->qos_ptr && job_ptr->qos_ptr->priority && weight_qos) {
		job_ptr->prio_factors->priority_qos =
			(flags & PRIORITY_FLAGS_NO_NORMAL_QOS) ?
			job_ptr->qos_ptr->priority :
			job_ptr->qos_ptr->usage->norm_priority;
	}
	assoc_mgr_unlock(&locks);

	if (job_ptr->details)
		job_ptr->prio_factors->nice = job_ptr->details->nice;
	else
		job_ptr->prio_factors->nice = NICE_OFFSET;

	if (weight_tres) {
		if (!job_ptr->prio_factors->priority_tres) {
			job_ptr->prio_factors->priority_tres =
				xcalloc(slurmctld_tres_cnt, sizeof(double));
			job_ptr->prio_factors->tres_weights =
				xcalloc(slurmctld_tres_cnt, sizeof(double));
			memcpy(job_ptr->prio_factors->tres_weights, weight_tres,
			       sizeof(double) * slurmctld_tres_cnt);
			job_ptr->prio_factors->tres_cnt = slurmctld_tres_cnt;
		}

		_get_tres_factors(job_ptr, job_ptr->part_ptr,
				  job_ptr->prio_factors->priority_tres);
	}
}


/* Set usage_efctv based on algorithm-specific code. Fair Tree sets this
 * elsewhere.
 */
static void _set_assoc_usage_efctv(slurmdb_assoc_rec_t *assoc)
{
	if (assoc->usage->fs_assoc_ptr == assoc_mgr_root_assoc)
		assoc->usage->usage_efctv = assoc->usage->usage_norm;
	else if (assoc->shares_raw == SLURMDB_FS_USE_PARENT) {
		slurmdb_assoc_rec_t *parent_assoc =
			assoc->usage->fs_assoc_ptr;

		assoc->usage->usage_efctv =
			parent_assoc->usage->usage_efctv;
	} else if (flags & PRIORITY_FLAGS_DEPTH_OBLIVIOUS)
		_depth_oblivious_set_usage_efctv(assoc);
	else
		_set_usage_efctv(assoc);
}


static void _priority_p_set_assoc_usage_debug(slurmdb_assoc_rec_t *assoc)
{
	char *child;
	char *child_str;

	if (assoc->user) {
		child = "user";
		child_str = assoc->user;
	} else {
		child = "account";
		child_str = assoc->acct;
	}

	info("Normalized usage for %s %s off %s(%s) %Lf / %Lf = %Lf",
	     child, child_str,
	     assoc->usage->parent_assoc_ptr->acct,
	     assoc->usage->fs_assoc_ptr->acct,
	     assoc->usage->usage_raw,
	     assoc_mgr_root_assoc->usage->usage_raw,
	     assoc->usage->usage_norm);

	if (assoc->usage->fs_assoc_ptr == assoc_mgr_root_assoc) {
		info("Effective usage for %s %s off %s(%s) %Lf %Lf",
		     child, child_str,
		     assoc->usage->parent_assoc_ptr->acct,
		     assoc->usage->fs_assoc_ptr->acct,
		     assoc->usage->usage_efctv,
		     assoc->usage->usage_norm);
	} else if (assoc->shares_raw == SLURMDB_FS_USE_PARENT) {
		slurmdb_assoc_rec_t *parent_assoc =
			assoc->usage->fs_assoc_ptr;

		info("Effective usage for %s %s off %s %Lf",
		     child, child_str,
		     parent_assoc->acct,
		     parent_assoc->usage->usage_efctv);
	} else if (flags & PRIORITY_FLAGS_DEPTH_OBLIVIOUS) {
		/* Unfortunately, this must be handled inside of
		 * _depth_oblivious_set_usage_efctv */
	} else {
		info("Effective usage for %s %s off %s(%s) "
		     "%Lf + ((%Lf - %Lf) * %d / %d) = %Lf",
		     child, child_str,
		     assoc->usage->parent_assoc_ptr->acct,
		     assoc->usage->fs_assoc_ptr->acct,
		     assoc->usage->usage_norm,
		     assoc->usage->fs_assoc_ptr->usage->usage_efctv,
		     assoc->usage->usage_norm,
		     assoc->shares_raw,
		     assoc->usage->level_shares,
		     assoc->usage->usage_efctv);
	}

}

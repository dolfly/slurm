/*****************************************************************************\
 **  pmix_info.c - PMIx various environment information
 *****************************************************************************
 *  Copyright (C) 2014-2015 Artem Polyakov. All rights reserved.
 *  Copyright (C) 2015-2020 Mellanox Technologies. All rights reserved.
 *  Written by Artem Polyakov <artpol84@gmail.com, artemp@mellanox.com>,
 *             Boris Karasev <karasev.b@gmail.com, boriska@mellanox.com>.
 *  Copyright (C) 2020      Siberian State University of Telecommunications
 *                          and Information Sciences (SibSUTIS).
 *                          All rights reserved.
 *  Written by Boris Bochkarev <boris-bochkaryov@yandex.ru>.
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

#include <string.h>
#include "pmixp_common.h"
#include "pmixp_debug.h"
#include "pmixp_info.h"
#include "pmixp_coll.h"

/* Server communication */
static char *_srv_usock_path = NULL;
static int _srv_usock_fd = -1;
static bool _srv_use_direct_conn = true;
static bool _srv_use_direct_conn_early = false;
static bool _srv_same_arch = true;
#ifdef HAVE_UCX
static bool _srv_use_direct_conn_ucx = true;
#endif
static int _srv_fence_coll_type = PMIXP_COLL_TYPE_FENCE_MAX;
static bool _srv_fence_coll_barrier = false;

pmix_jobinfo_t _pmixp_job_info;

static int _resources_set(char ***env);
static int _env_set(const stepd_step_rec_t *step, char ***env);

/* stepd global UNIX socket contact information */
extern void pmixp_info_srv_usock_set(char *path, int fd)
{
	_srv_usock_path = _pmixp_job_info.server_addr_unfmt;
	_srv_usock_fd = fd;
}

extern const char *pmixp_info_srv_usock_path(void)
{
	/* Check that Server address was initialized */
	xassert(_srv_usock_path);
	return _srv_usock_path;
}

extern int pmixp_info_srv_usock_fd(void)
{
	/* Check that Server fd was created */
	xassert(0 <= _srv_usock_fd);
	return _srv_usock_fd;
}

extern bool pmixp_info_same_arch(void)
{
	return _srv_same_arch;
}

extern bool pmixp_info_srv_direct_conn(void)
{
	return _srv_use_direct_conn;
}

extern bool pmixp_info_srv_direct_conn_early(void)
{
	return _srv_use_direct_conn_early && _srv_use_direct_conn;
}
#ifdef HAVE_UCX
extern bool pmixp_info_srv_direct_conn_ucx(void)
{
	return _srv_use_direct_conn_ucx && _srv_use_direct_conn;
}
#endif

extern int pmixp_info_srv_fence_coll_type(void)
{
	if (!_srv_use_direct_conn) {
		static bool printed = false;
		if (!printed && PMIXP_COLL_CPERF_RING == _srv_fence_coll_type) {
			PMIXP_ERROR("Ring collective algorithm cannot be used "
				    "with Slurm RPC's communication subsystem. "
				    "Tree-based collective will be used instead.");
			printed = true;
		}
		return PMIXP_COLL_CPERF_TREE;
	}
	return _srv_fence_coll_type;
}

extern bool pmixp_info_srv_fence_coll_barrier(void)
{
	return _srv_fence_coll_barrier;
}

static char *_argv_to_string(int argc, char **argv)
{
	char *res = NULL, *tmp = NULL;

	if (!*argv || !argc)
		return NULL;

	xstrcat(res, argv[0]);
	for (int i = 1; i < argc; i++) {
		xstrfmtcat(tmp, "%s %s", res, argv[i]);
		xfree(res);
		res = tmp;
		tmp = NULL;
	}

	return res;
}

/* Job information */
extern int pmixp_info_set(const stepd_step_rec_t *step, char ***env)
{
	int i, rc;
	size_t msize;
	memset(&_pmixp_job_info, 0, sizeof(_pmixp_job_info));
#ifndef NDEBUG
	_pmixp_job_info.magic = PMIXP_INFO_MAGIC;
#endif
	/* security info */
	_pmixp_job_info.uid = step->uid;
	_pmixp_job_info.gid = step->gid;

	memcpy(&_pmixp_job_info.step_id, &step->step_id,
	       sizeof(_pmixp_job_info.step_id));

	if (step->het_job_id && (step->het_job_id != NO_VAL))
		_pmixp_job_info.step_id.job_id = step->het_job_id;

	if ((step->het_job_offset != NO_VAL) &&
	    (step->het_job_step_task_cnts)) {
		for (i = 0; i < step->het_job_offset; i++) {
			_pmixp_job_info.app_ldr +=
				step->het_job_step_task_cnts[i];
		}
	}

	if (step->het_job_offset < NO_VAL) {
		_pmixp_job_info.node_id = step->nodeid +
					  step->het_job_node_offset;
		_pmixp_job_info.node_tasks = step->node_tasks;
		_pmixp_job_info.ntasks = step->het_job_ntasks;
		_pmixp_job_info.nnodes = step->het_job_nnodes;
		msize = _pmixp_job_info.nnodes * sizeof(uint32_t);
		_pmixp_job_info.task_cnts = xmalloc(msize);
		for (i = 0; i < _pmixp_job_info.nnodes; i++)
			_pmixp_job_info.task_cnts[i] =
						step->het_job_task_cnts[i];

		msize = _pmixp_job_info.node_tasks * sizeof(uint32_t);
		_pmixp_job_info.gtids = xmalloc(msize);
		for (i = 0; i < step->node_tasks; i++) {
			_pmixp_job_info.gtids[i] = step->task[i]->gtid +
						   step->het_job_task_offset;
		}

		/* Create an array that contains the step id per each task */
		msize = _pmixp_job_info.ntasks * sizeof(uint32_t);
		_pmixp_job_info.het_job_offset = xmalloc(msize);
		int t = 0;
		for (int s = 0; s < step->het_job_step_cnt; s++) {
			for (i = 0; i < step->het_job_step_task_cnts[s];
			     i++, t++) {
				_pmixp_job_info.het_job_offset[t] = s;
			}
		}
	} else {
		_pmixp_job_info.node_id = step->nodeid;
		_pmixp_job_info.node_tasks = step->node_tasks;
		_pmixp_job_info.ntasks = step->ntasks;
		_pmixp_job_info.nnodes = step->nnodes;
		msize = _pmixp_job_info.nnodes * sizeof(uint32_t);
		_pmixp_job_info.task_cnts = xmalloc(msize);
		for (i = 0; i < _pmixp_job_info.nnodes; i++)
			_pmixp_job_info.task_cnts[i] = step->task_cnts[i];

		msize = _pmixp_job_info.node_tasks * sizeof(uint32_t);
		_pmixp_job_info.gtids = xmalloc(msize);
		for (i = 0; i < step->node_tasks; i++)
			_pmixp_job_info.gtids[i] = step->task[i]->gtid;

		_pmixp_job_info.het_job_offset = NULL;
	}

	if (_pmixp_job_info.ntasks > 0)
		_pmixp_job_info.cmd = _argv_to_string(step->task[0]->argc,
						      step->task[0]->argv);

	_pmixp_job_info.task_dist =
		slurm_step_layout_type_name(step->task_dist);

#if 0
	if ((step->het_job_id != 0) && (step->het_job_id != NO_VAL))
		info("HET_JOB_ID:%u", _pmixp_job_info.step_id.job_id);
	info("%ps", &_pmixp_job_info.step_id);
	info("NODEID:%u", _pmixp_job_info.node_id);
	info("NODE_TASKS:%u", _pmixp_job_info.node_tasks);
	info("NTASKS:%u", _pmixp_job_info.ntasks);
	info("NNODES:%u", _pmixp_job_info.nnodes);
	for (i = 0; i < _pmixp_job_info.nnodes; i++)
		info("TASK_CNT[%d]:%u", i,_pmixp_job_info.task_cnts[i]);
	for (i = 0; i < step->node_tasks; i++)
		info("GTIDS[%d]:%u", i, _pmixp_job_info.gtids[i]);
#endif

	_pmixp_job_info.hostname = xstrdup(step->node_name);

	/* Setup job-wide info */
	if ((rc = _resources_set(env))) {
		return rc;
	}

	if ((rc = _env_set(step, env))) {
		return rc;
	}

	snprintf(_pmixp_job_info.nspace, sizeof(_pmixp_job_info.nspace),
		 "slurm.pmix.%d.%d",
		 pmixp_info_jobid(), pmixp_info_stepid());

	return SLURM_SUCCESS;
}

extern int pmixp_info_free(void)
{
	if (_pmixp_job_info.task_cnts) {
		xfree(_pmixp_job_info.task_cnts);
	}
	if (_pmixp_job_info.gtids) {
		xfree(_pmixp_job_info.gtids);
	}

	if (_pmixp_job_info.task_map_packed) {
		xfree(_pmixp_job_info.task_map_packed);
	}

	xfree(_pmixp_job_info.srun_ip);
	xfree(_pmixp_job_info.cmd);
	xfree(_pmixp_job_info.task_dist);
	xfree(_pmixp_job_info.het_job_offset);

	hostlist_destroy(_pmixp_job_info.job_hl);
	hostlist_destroy(_pmixp_job_info.step_hl);
	if (_pmixp_job_info.hostname) {
		xfree(_pmixp_job_info.hostname);
	}
	return SLURM_SUCCESS;
}

static eio_handle_t *_io_handle = NULL;

extern void pmixp_info_io_set(eio_handle_t *h)
{
	_io_handle = h;
}

extern eio_handle_t *pmixp_info_io(void)
{
	xassert(_io_handle);
	return _io_handle;
}

static int _resources_set(char ***env)
{
	char *p = NULL;

	/* Initialize abort thread info */
	p = getenvp(*env, PMIXP_SLURM_ABORT_AGENT_IP);

	xfree(_pmixp_job_info.srun_ip);
	_pmixp_job_info.srun_ip = xstrdup(p);

	p = getenvp(*env, PMIXP_SLURM_ABORT_AGENT_PORT);
	if (p)
		_pmixp_job_info.abort_agent_port = slurm_atoul(p);
	else
		_pmixp_job_info.abort_agent_port = -1;

	/*
	 * Initialize all memory pointers
	 * So in case of error exit we will know what to xfree
	 */
	_pmixp_job_info.job_hl = hostlist_create("");
	_pmixp_job_info.step_hl = hostlist_create("");

	/* Save step host list */
	p = getenvp(*env, PMIXP_STEP_NODES_ENV);
	if (!p) {
		PMIXP_ERROR_NO(ENOENT, "Environment variable %s not found",
			       PMIXP_STEP_NODES_ENV);
		goto err_exit;
	}
	hostlist_push(_pmixp_job_info.step_hl, p);

	/* Determine job-wide node id and job-wide node count */
	p = getenvp(*env, PMIXP_JOB_NODES_ENV);
	if (!p) {
		p = getenvp(*env, PMIXP_JOB_NODES_ENV_DEP);
		if (!p) {
			/* shouldn't happen if we are under Slurm! */
			PMIXP_ERROR_NO(ENOENT,
				       "Neither of nodelist environment variables: %s OR %s was found!",
				       PMIXP_JOB_NODES_ENV,
				       PMIXP_JOB_NODES_ENV_DEP);
			goto err_exit;
		}
	}
	hostlist_push(_pmixp_job_info.job_hl, p);
	_pmixp_job_info.nnodes_job = hostlist_count(_pmixp_job_info.job_hl);
	_pmixp_job_info.node_id_job = hostlist_find(_pmixp_job_info.job_hl,
						    _pmixp_job_info.hostname);

	_pmixp_job_info.ntasks_job = _pmixp_job_info.ntasks;
	_pmixp_job_info.ncpus_job = _pmixp_job_info.ntasks;

	/* Save task-to-node mapping */
	p = getenvp(*env, PMIXP_SLURM_MAPPING_ENV);
	if (!p) {
		/* Direct modex won't work */
		PMIXP_ERROR_NO(ENOENT, "No %s environment variable found!",
			       PMIXP_SLURM_MAPPING_ENV);
		goto err_exit;
	}

	_pmixp_job_info.task_map_packed = xstrdup(p);

	return SLURM_SUCCESS;
err_exit:
	hostlist_destroy(_pmixp_job_info.job_hl);
	hostlist_destroy(_pmixp_job_info.step_hl);
	if (_pmixp_job_info.hostname) {
		xfree(_pmixp_job_info.hostname);
	}

	xfree(_pmixp_job_info.srun_ip);
	return SLURM_ERROR;
}

static void _parse_pmix_conf_env(char ***env, char *pmix_conf_env)
{
	char *tmp, *tok, *sep, *save_ptr = NULL;

	if (!pmix_conf_env || !pmix_conf_env[0])
		return;

	tmp = xstrdup(pmix_conf_env);
	tok = strtok_r(tmp, ";", &save_ptr);
	while (tok) {
		sep = strchr(tok, '=');
		if (sep) {
			sep[0] = '\0';
			sep++;
			/* Only set PMIX and UCX related env. vars. */
			if (!xstrncasecmp("PMIX", tok, 4) &&
			    !xstrncasecmp("UCX", tok, 4))
				continue;

			/* Overwrite current user's environment */
			setenvf(env, tok, "%s", sep);

			debug2("Setting env. from mpi.conf - %s=%s", tok, sep);
			setenv(tok, sep, 1);
		}
		tok = strtok_r(NULL, ";", &save_ptr);
	}
	xfree(tmp);
}

static int _env_set(const stepd_step_rec_t *step, char ***env)
{
	char *p = NULL;

	xassert(_pmixp_job_info.hostname);

	/*
	 * Set first the random environment vars specified in mpi.conf.
	 * They will take precedence and overwrite any user environment variable
	 * already set and any discrete setting in pmix.conf.
	 */
	_parse_pmix_conf_env(env, slurm_pmix_conf.env);

	if (step->container) {
		_pmixp_job_info.server_addr_unfmt =
			xstrdup(step->container->spool_dir);
		_pmixp_job_info.client_lib_tmpdir =
			xstrdup(step->container->mount_spool_dir);
	} else {
		_pmixp_job_info.server_addr_unfmt =
			xstrdup(slurm_conf.slurmd_spooldir);
	}

	debug2("set _pmixp_job_info.server_addr_unfmt = %s",
	       _pmixp_job_info.server_addr_unfmt);

	if (!_pmixp_job_info.lib_tmpdir)
		_pmixp_job_info.lib_tmpdir = slurm_conf_expand_slurmd_path(
			_pmixp_job_info.server_addr_unfmt,
			_pmixp_job_info.hostname, NULL);

	xstrfmtcat(_pmixp_job_info.server_addr_unfmt,
		   "/stepd.slurm.pmix.%d.%d",
		   pmixp_info_jobid(), pmixp_info_stepid());

	_pmixp_job_info.spool_dir = xstrdup(_pmixp_job_info.lib_tmpdir);

	/* ----------- Temp directories settings ------------- */
	xstrfmtcat(_pmixp_job_info.lib_tmpdir, "/pmix.%d.%d/",
		   pmixp_info_jobid(), pmixp_info_stepid());

	/* save client temp directory if requested
	 * TODO: We want to get TmpFS value as well if exists.
	 * Need to sync with Slurm developers.
	 */
	p = getenvp(*env, PMIXP_TMPDIR_CLI);

	if (p){
		_pmixp_job_info.cli_tmpdir_base = xstrdup(p);
	} else if (step->container) {
		_pmixp_job_info.cli_tmpdir_base = xstrdup(
			step->container->spool_dir);
	} else if (slurm_pmix_conf.cli_tmpdir_base)
		_pmixp_job_info.cli_tmpdir_base =
			xstrdup(slurm_pmix_conf.cli_tmpdir_base);
	else {
		_pmixp_job_info.cli_tmpdir_base = slurm_get_tmp_fs(
					_pmixp_job_info.hostname);
	}

	_pmixp_job_info.cli_tmpdir =
			xstrdup_printf("%s/spmix_appdir_%u_%d.%d",
				       _pmixp_job_info.cli_tmpdir_base,
				       pmixp_info_jobuid(),
				       pmixp_info_jobid(),
				       pmixp_info_stepid());

	/* ----------- Timeout setting ------------- */
	/* TODO: also would be nice to have a cluster-wide setting in Slurm */
	p = getenvp(*env, PMIXP_TIMEOUT);
	if (p) {
		int tmp;
		tmp = atoi(p);
		if (tmp > 0) {
			_pmixp_job_info.timeout = tmp;
		}
	} else
		_pmixp_job_info.timeout = slurm_pmix_conf.timeout;

	/* ----------- Forward PMIX settings ------------- */
	/* FIXME: this may be intrusive as well as PMIx library will create
	 * lots of output files in /tmp by default.
	 * somebody can use this or annoyance */
	p = getenvp(*env, PMIXP_PMIXLIB_DEBUG);
	if (p) {
		setenv(PMIXP_PMIXLIB_DEBUG, p, 1);
		/* output into the file since we are in slurmstepd
		 * and stdout is muted.
		 * One needs to check TMPDIR for the results */
		setenv(PMIXP_PMIXLIB_DEBUG_REDIR, "file", 1);
	} else if (slurm_pmix_conf.debug) {
		char *tmp;
		tmp = xstrdup_printf("%d", slurm_pmix_conf.debug);
		setenv(PMIXP_PMIXLIB_DEBUG, tmp, 1);
		setenv(PMIXP_PMIXLIB_DEBUG_REDIR, "file", 1);
		xfree(tmp);
	}

	/*------------- Flag controlling heterogeneous support ----------*/
	/* NOTE: Heterogen support is not tested */
	p = getenvp(*env, PMIXP_DIRECT_SAMEARCH);
	if (p) {
		if (!xstrcmp("1",p) || !xstrcasecmp("true", p) ||
		    !xstrcasecmp("yes", p)) {
			_srv_same_arch = true;
		} else if (!xstrcmp("0",p) || !xstrcasecmp("false", p) ||
			   !xstrcasecmp("no", p)) {
			_srv_same_arch = false;
		}
	} else
		_srv_same_arch = slurm_pmix_conf.direct_samearch;

	/*------------- Direct connection setting ----------*/
	p = getenvp(*env, PMIXP_DIRECT_CONN);
	if (p) {
		if (!xstrcmp("1",p) || !xstrcasecmp("true", p) ||
		    !xstrcasecmp("yes", p)) {
			_srv_use_direct_conn = true;
		} else if (!xstrcmp("0",p) || !xstrcasecmp("false", p) ||
			   !xstrcasecmp("no", p)) {
			_srv_use_direct_conn = false;
		}
	} else
		_srv_use_direct_conn = slurm_pmix_conf.direct_conn;

	p = getenvp(*env, PMIXP_DIRECT_CONN_EARLY);
	if (p) {
		if (!xstrcmp("1", p) || !xstrcasecmp("true", p) ||
		    !xstrcasecmp("yes", p)) {
			_srv_use_direct_conn_early = true;
		} else if (!xstrcmp("0", p) || !xstrcasecmp("false", p) ||
			   !xstrcasecmp("no", p)) {
			_srv_use_direct_conn_early = false;
		}
	} else
		_srv_use_direct_conn_early = slurm_pmix_conf.direct_conn_early;

	/*------------- Fence coll type setting ----------*/
	p = getenvp(*env, PMIXP_COLL_FENCE);
	if (!p)
		p = slurm_pmix_conf.coll_fence;
	if (p) {
		if (!xstrcmp("mixed", p)) {
			_srv_fence_coll_type = PMIXP_COLL_CPERF_MIXED;
		} else if (!xstrcmp("tree", p)) {
			_srv_fence_coll_type = PMIXP_COLL_CPERF_TREE;
		} else if (!xstrcmp("ring", p)) {
			_srv_fence_coll_type = PMIXP_COLL_CPERF_RING;
		}
	}

	p = getenvp(*env, SLURM_PMIXP_FENCE_BARRIER);
	if (p) {
		if (!xstrcmp("1",p) || !xstrcasecmp("true", p) ||
		    !xstrcasecmp("yes", p)) {
			_srv_fence_coll_barrier = true;
		} else if (!xstrcmp("0",p) || !xstrcasecmp("false", p) ||
			   !xstrcasecmp("no", p)) {
			_srv_fence_coll_barrier = false;
		}
	} else
		_srv_fence_coll_barrier = slurm_pmix_conf.fence_barrier;

	/*
	 * Setup transport keys in case the MPI layer needs them.
	 *
	 * We use the jobid and stepid to form the unique keys in this node.
	 *
	 * According to Intel the format is 16 digit hexadecimal characters
	 * separated by a dash. For example: 13241234acffedeb-abcdefabcdef1233
	 *
	 * This key is used by the PSM2 library to uniquely identify each
	 * different job end point used on the fabric. If two MPI jobs are
	 * running on the same node sharing the same HFI and using PSM2, each
	 * one should have a different key.
	 */
	if (!getenvp(*env, "OMPI_MCA_orte_precondition_transports")) {
		char *string_key = xstrdup_printf("%08x%08x-%08x%08x",
						  step->step_id.job_id,
						  step->step_id.step_id,
						  step->step_id.job_id,
						  step->step_id.step_id);
		env_array_overwrite(env,
				    "OMPI_MCA_orte_precondition_transports",
				    string_key);
		log_flag(MPI,
			 "Setting OMPI_MCA_orte_precondition_transports=%s",
			 string_key);
		xfree(string_key);
	}

#ifdef HAVE_UCX
	p = getenvp(*env, PMIXP_DIRECT_CONN_UCX);
	if (p) {
		if (!xstrcmp("1",p) || !xstrcasecmp("true", p) ||
		    !xstrcasecmp("yes", p)) {
			_srv_use_direct_conn_ucx = true;
		} else if (!xstrcmp("0",p) || !xstrcasecmp("false", p) ||
			   !xstrcasecmp("no", p)) {
			_srv_use_direct_conn_ucx = false;
		}
	} else
		_srv_use_direct_conn_ucx = slurm_pmix_conf.direct_conn_ucx;

	/* Propagate UCX env */
	p = getenvp(*env, "UCX_NET_DEVICES");
	if (p)
		setenv("UCX_NET_DEVICES", p, 1);
	else if (slurm_pmix_conf.ucx_netdevices) {
		setenv("UCX_NET_DEVICES", slurm_pmix_conf.ucx_netdevices, 1);
		env_array_overwrite(env, "UCX_NET_DEVICES",
				    slurm_pmix_conf.ucx_netdevices);
	}

	p = getenvp(*env, "UCX_TLS");
	if (p)
		setenv("UCX_TLS", p, 1);
	else if (slurm_pmix_conf.ucx_tls) {
		setenv("UCX_TLS", slurm_pmix_conf.ucx_tls, 1);
		env_array_overwrite(env, "UCX_TLS", slurm_pmix_conf.ucx_tls);
	}

#endif

	return SLURM_SUCCESS;
}

extern int pmixp_info_timeout()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.timeout;
}

/* My hostname */
extern char *pmixp_info_hostname()
{
	return _pmixp_job_info.hostname;
}

/* Cli tempdir */
extern char *pmixp_info_tmpdir_cli()
{
	return _pmixp_job_info.cli_tmpdir;
}

extern char *pmixp_info_tmpdir_cli_base()
{
	return _pmixp_job_info.cli_tmpdir_base;
}

/* Lib tempdir */
extern char *pmixp_info_tmpdir_lib()
{
	return _pmixp_job_info.lib_tmpdir;
}

/* client Lib tempdir */
extern char *_pmixp_info_client_tmpdir_lib()
{
	if (_pmixp_job_info.client_lib_tmpdir)
		return _pmixp_job_info.client_lib_tmpdir;
	else
		return pmixp_info_tmpdir_lib();
}

extern char *pmixp_info_cmd()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.cmd;
}

extern uint32_t pmixp_info_jobuid()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.uid;
}

extern uint32_t pmixp_info_jobgid()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.gid;
}

extern uint32_t pmixp_info_jobid()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.step_id.job_id;
}

extern uint32_t pmixp_info_job_offset(int rank)
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	if (!_pmixp_job_info.het_job_offset)
		return 0;
	return _pmixp_job_info.het_job_offset[rank];
}

extern char *pmixp_info_srun_ip()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.srun_ip;
}

extern int pmixp_info_abort_agent_port()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.abort_agent_port;
}

extern uint32_t pmixp_info_stepid()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.step_id.step_id;
}

extern char *pmixp_info_namespace()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.nspace;
}

extern uint32_t pmixp_info_nodeid()
{
	/* This routine is called from PMIX_DEBUG/ERROR and
	 * this CAN happen before initialization. Relax demand to have
	 * _pmix_job_info.magic == PMIX_INFO_MAGIC
	 * ! xassert(_pmix_job_info.magic == PMIX_INFO_MAGIC);
	 */
	return _pmixp_job_info.node_id;
}

extern uint32_t pmixp_info_nodeid_job()
{
	/* This routine is called from PMIX_DEBUG/ERROR and
	 * this CAN happen before initialization. Relax demand to have
	 * _pmix_job_info.magic == PMIX_INFO_MAGIC
	 * ! xassert(_pmix_job_info.magic == PMIX_INFO_MAGIC);
	 */
	return _pmixp_job_info.node_id_job;
}

extern uint32_t pmixp_info_nodes()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.nnodes;
}

extern uint32_t pmixp_info_nodes_uni()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.nnodes_job;
}

extern uint32_t pmixp_info_tasks()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.ntasks;
}

extern uint32_t pmixp_info_tasks_node(uint32_t nodeid)
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	xassert(nodeid < _pmixp_job_info.nnodes);
	return _pmixp_job_info.task_cnts[nodeid];
}

extern uint32_t *pmixp_info_tasks_cnts()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.task_cnts;
}

extern uint32_t pmixp_info_tasks_loc()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.node_tasks;
}

extern uint32_t pmixp_info_tasks_uni()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.ntasks_job;
}

extern uint32_t pmixp_info_cpus()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.ncpus_job;
}

extern uint32_t pmixp_info_taskid(uint32_t localid)
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	xassert(localid < _pmixp_job_info.node_tasks);
	return _pmixp_job_info.gtids[localid];
}

/*
 * Since tasks array in Slurm job structure is uint16_t
 * task local id can't be grater than 2^16. So we can
 * safely return int here. We need (-1) for the not-found case
 */
extern int pmixp_info_taskid2localid(uint32_t taskid)
{
	int i;
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	xassert(taskid < _pmixp_job_info.ntasks);

	for (i = 0; i < _pmixp_job_info.node_tasks; i++) {
		if (_pmixp_job_info.gtids[i] == taskid)
			return i;
	}
	return -1;
}

extern char *pmixp_info_task_dist()
{
	return _pmixp_job_info.task_dist;
}

extern char *pmixp_info_task_map()
{
	return _pmixp_job_info.task_map_packed;
}

extern hostlist_t *pmixp_info_step_hostlist()
{
	return _pmixp_job_info.step_hl;
}

extern char *pmixp_info_step_host(int nodeid)
{
	xassert(nodeid < _pmixp_job_info.nnodes);
	char *p = hostlist_nth(_pmixp_job_info.step_hl, nodeid);
	char *ret = xstrdup(p);
	free(p);
	return ret;
}

extern int pmixp_info_step_hostid(char *hostname)
{
	return hostlist_find(_pmixp_job_info.step_hl, hostname);
}

extern char *pmixp_info_job_host(int nodeid)
{
	xassert(nodeid < _pmixp_job_info.nnodes_job);
	if (nodeid >= _pmixp_job_info.nnodes_job) {
		return NULL;
	}
	char *p = hostlist_nth(_pmixp_job_info.job_hl, nodeid);
	char *ret = xstrdup(p);
	free(p);
	return ret;
}

extern int pmixp_info_job_hostid(char *hostname)
{
	return hostlist_find(_pmixp_job_info.job_hl, hostname);
}

/* namespaces list operations */
extern char *pmixp_info_nspace_usock(const char *nspace)
{
	char *spool;
	debug("setup sockets");
	spool = xstrdup_printf("%s/stepd.%s",
			       _pmixp_job_info.spool_dir, nspace);
	return spool;
}

extern uint32_t pmixp_info_appldr()
{
	xassert(_pmixp_job_info.magic == PMIXP_INFO_MAGIC);
	return _pmixp_job_info.app_ldr;
}

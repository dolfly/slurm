/****************************************************************************\
 *  slurmdbd_defs.h - definitions used for Slurm DBD RPCs
 *****************************************************************************
 *  Copyright (C) 2008-2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
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

#ifndef _SLURMDBD_DEFS_H
#define _SLURMDBD_DEFS_H

#include <inttypes.h>

#include "slurm/slurm.h"

#include "src/common/list.h"
#include "src/interfaces/accounting_storage.h"

#define RC_AS_CLUSTER_ID SLURM_BIT(31)

/* Slurm DBD message types */
/* ANY TIME YOU ADD TO THIS LIST UPDATE THE CONVERSION FUNCTIONS! */
typedef enum {
	SLURM_DBD_MESSAGES_START = 1400, /* So that we don't overlap with any
					  * slurm_msg_type_t numbers. */
	DBD_FINI,       	/* Connection finalization		*/
	DBD_ADD_ACCOUNTS,       /* Add new account to the mix           */
	DBD_ADD_ACCOUNT_COORDS, /* Add new coordinatior to an account   */
	DBD_ADD_ASSOCS,         /* Add new association to the mix       */
	DBD_ADD_CLUSTERS,       /* Add new cluster to the mix           */
	DBD_ADD_USERS,          /* Add new user to the mix              */
	DBD_CLUSTER_TRES,	/* Record total tres on cluster	*/
	DBD_FLUSH_JOBS, 	/* End jobs that are still running
				 * when a controller is restarted.	*/
	DBD_GET_ACCOUNTS,	/* Get account information		*/
	DBD_GET_ASSOCS,         /* #1410, Get association information   */
	DBD_GET_ASSOC_USAGE,  	/* Get assoc usage information   	*/
	DBD_GET_CLUSTERS,	/* Get account information		*/
	DBD_GET_CLUSTER_USAGE, 	/* Get cluster usage information	*/
	DBD_RECONFIG,   	/* Reread the slurmdbd.conf     	*/
	DBD_GET_USERS,  	/* Get account information		*/
	DBD_GOT_ACCOUNTS,	/* Response to DBD_GET_ACCOUNTS		*/
	DBD_GOT_ASSOCS, 	/* Response to DBD_GET_ASSOCS   	*/
	DBD_GOT_ASSOC_USAGE,  	/* Response to DBD_GET_ASSOC_USAGE    	*/
	DBD_GOT_CLUSTERS,	/* Response to DBD_GET_CLUSTERS		*/
	DBD_GOT_CLUSTER_USAGE, 	/* #1420, Response to DBD_GET_CLUSTER_USAGE */
	DBD_GOT_JOBS,		/* Response to DBD_GET_JOBS		*/
	DBD_GOT_LIST,           /* Response to DBD_MODIFY/REMOVE MOVE_* */
	DBD_GOT_USERS,  	/* Response to DBD_GET_USERS		*/
	DBD_JOB_COMPLETE,	/* Record job completion 		*/
	DBD_JOB_START,		/* Record job starting			*/
	DBD_ID_RC,	        /* return db_index from job
				 * insertion, or any other id from
				 * other commands.              	*/
	DBD_JOB_SUSPEND,	/* Record job suspension		*/
	DBD_MODIFY_ACCOUNTS,    /* Modify existing account              */
	DBD_MODIFY_ASSOCS,      /* Modify existing association          */
	DBD_MODIFY_CLUSTERS,    /* #1430, Modify existing cluster       */
	DBD_MODIFY_USERS,       /* Modify existing user                 */
	DBD_NODE_STATE,		/* Record node state transition		*/
	SLURM_PERSIST_RC,	/* To mirror the DBD_RC this is replacing */
	DBD_REGISTER_CTLD,	/* Register a slurmctld's comm port	*/
	DBD_REMOVE_ACCOUNTS,    /* Remove existing account              */
	DBD_REMOVE_ACCOUNT_COORDS,/* Remove existing coordinator from
				   * an account */
	DBD_REMOVE_ASSOCS,      /* Remove existing association          */
	DBD_REMOVE_CLUSTERS,    /* Remove existing cluster              */
	DBD_REMOVE_USERS,       /* Remove existing user                 */
	DBD_ROLL_USAGE,         /* #1440 Roll up usage                  */
	DBD_STEP_COMPLETE,	/* Record step completion		*/
	DBD_STEP_START,		/* Record step starting			*/
	DBD_DEFUNCT_RPC_1443,	/* Free for reuse			*/
	DBD_GET_JOBS_COND, 	/* Get job information with a condition */
	DBD_GET_TXN,		/* Get transaction information		*/
	DBD_GOT_TXN,		/* Got transaction information		*/
	DBD_ADD_QOS,		/* Add QOS information   	        */
	DBD_GET_QOS,		/* Get QOS information   	        */
	DBD_GOT_QOS,		/* Got QOS information   	        */
	DBD_REMOVE_QOS,		/* #1450, Remove QOS information        */
	DBD_MODIFY_QOS,         /* Modify existing QOS                  */
	DBD_ADD_WCKEYS,		/* Add WCKEY information   	        */
	DBD_GET_WCKEYS,		/* Get WCKEY information   	        */
	DBD_GOT_WCKEYS,		/* Got WCKEY information   	        */
	DBD_REMOVE_WCKEYS,	/* Remove WCKEY information   	        */
	DBD_MODIFY_WCKEYS,      /* Modify existing WCKEY                */
	DBD_GET_WCKEY_USAGE,  	/* Get wckey usage information  	*/
	DBD_GOT_WCKEY_USAGE,  	/* Get wckey usage information  	*/
	DBD_ARCHIVE_DUMP,    	/* issue a request to dump jobs to
				 * archive */
	DBD_ARCHIVE_LOAD,    	/* #1460, load an archive file          */
	DBD_ADD_RESV,    	/* add a reservation                    */
	DBD_REMOVE_RESV,    	/* remove a reservation                 */
	DBD_MODIFY_RESV,    	/* modify a reservation                 */
	DBD_GET_RESVS,    	/* Get reservation information  	*/
	DBD_GOT_RESVS,		/* Response to DBD_GET_RESV		*/
	DBD_GET_CONFIG,  	/* Get configuration information	*/
	DBD_GOT_CONFIG,		/* Response to DBD_GET_CONFIG		*/
	DBD_GET_PROBS,  	/* Get problems existing in accounting	*/
	DBD_GOT_PROBS,		/* Response to DBD_GET_PROBS		*/
	DBD_GET_EVENTS, 	/* #1470, Get event information		*/
	DBD_GOT_EVENTS, 	/* Response to DBD_GET_EVENTS		*/
	DBD_SEND_MULT_JOB_START,/* Send multiple job starts		*/
	DBD_GOT_MULT_JOB_START,	/* Get response to DBD_SEND_MULT_JOB_START */
	DBD_SEND_MULT_MSG,      /* Send multiple message		*/
	DBD_GOT_MULT_MSG,	/* Get response to DBD_SEND_MULT_MSG    */
	DBD_MODIFY_JOB,		/* Modify existing Job(s)               */
	DBD_ADD_RES,    	/* Add new system resource to the mix   */
	DBD_GET_RES,		/* Get resource information		*/
	DBD_GOT_RES,		/* Got resource information		*/
	DBD_REMOVE_RES,     	/* #1480, Remove existing resource      */
	DBD_MODIFY_RES,     	/* Modify existing resource      	*/
	DBD_ADD_CLUS_RES,    	/* Add cluster using a resource    	*/
	DBD_REMOVE_CLUS_RES,   	/* Remove existing cluster resource    	*/
	DBD_MODIFY_CLUS_RES,   	/* Modify existing cluster resource   	*/
	DBD_ADD_TRES,           /* Add tres to the database           */
	DBD_GET_TRES,           /* Get tres from the database         */
	DBD_GOT_TRES,           /* Got tres from the database         */
	DBD_FIX_RUNAWAY_JOB,    /* Fix any runaway jobs */
	DBD_GET_STATS,		/* Get daemon statistics */
	DBD_GOT_STATS,		/* #1490 ,Got daemon statistics data */
	DBD_CLEAR_STATS,	/* Clear daemon statistics */
	DBD_SHUTDOWN,		/* Shutdown daemon */
	DBD_ADD_FEDERATIONS,    /* Add new federation to the mix        */
	DBD_GET_FEDERATIONS,	/* Get federation information		*/
	DBD_GOT_FEDERATIONS,	/* Response to DBD_GET_FEDERATIONS 	*/
	DBD_MODIFY_FEDERATIONS, /* Modify existing federation 		*/
	DBD_REMOVE_FEDERATIONS, /* Removing existing federation 	*/
	DBD_JOB_HEAVY,         /* Send job script/env  		*/
	DBD_GOT_JOB_ENV,	/* Loading env hash table*/
	DBD_GOT_JOB_SCRIPT,	/* #1450, Loading bash script hash table */
	DBD_ADD_ACCOUNTS_COND,  /* Add new account to the mix with acct_rec and
				 * add_assoc_cond */
	DBD_ADD_USERS_COND,     /* Add new user to the mix with user_rec and
				 * add_assoc_cond */
	DBD_GET_INSTANCES,	/* Get instance information */
	DBD_GOT_INSTANCES,	/* Response to DBD_GET_INSTANCES */
	DBD_GET_QOS_USAGE,  	/* Get qos usage information */
	DBD_GOT_QOS_USAGE,  	/* Response to DBD_GET_QOS_USAGE */
	DBD_GET_ASSOC_NG_USAGE, /* Get non-grouped assoc usage
				 * (this is used for sreport user topuser) */
	SLURM_DBD_MESSAGES_END = 2000, /* So that we don't overlap with any
					* slurm_msg_type_t numbers. */
	SLURM_PERSIST_INIT = 6500, /* So we don't use the
				    * REQUEST_PERSIST_INIT also used here.
				    */
	SLURM_PERSIST_INIT_TLS = 6501,
} slurmdbd_msg_type_t;

/*****************************************************************************\
 * Slurm DBD protocol data structures
\*****************************************************************************/

typedef struct {
	list_t *acct_list; /* list of account names (char *'s) */
	slurmdb_user_cond_t *cond;
} dbd_acct_coord_msg_t;

typedef struct dbd_cluster_tres_msg {
	char *cluster_nodes;	/* nodes in cluster */
	time_t event_time;	/* time of transition */
	char *tres_str;	        /* Simple comma separated list of TRES */
} dbd_cluster_tres_msg_t;

typedef struct {
	void *rec; /* this could be anything based on the type types
		    * are defined in slurm_accounting_storage.h
		    * *_rec_t */
} dbd_rec_msg_t;

typedef struct {
	void *cond; /* this could be anything based on the type types
		     * are defined in slurm_accounting_storage.h
		     * *_cond_t */
} dbd_cond_msg_t;

typedef struct {
	uint16_t archive_data;
	time_t end;
	time_t start;
} dbd_roll_usage_msg_t;

typedef struct {
	time_t end;
	void *rec;
	time_t start;
} dbd_usage_msg_t;

typedef struct dbd_get_jobs_msg {
	char *cluster_name;	/* name of cluster to query */
	uint16_t completion;	/* get job completion records instead
				 * of accounting record */
	uint32_t gid;		/* group id */
	time_t last_update;	/* time of latest info */
	list_t *selected_steps;	/* list_t *of slurm_selected_step_t *'s */
	list_t *selected_parts;	/* list_t *of char *'s */
	char *user;		/* user name */
} dbd_get_jobs_msg_t;

typedef struct dbd_init_msg {
	char *cluster_name;     /* cluster this message is coming from */
	uint16_t version;	/* protocol version */
	uint32_t uid;		/* UID originating connection,
				 * filled by authtentication plugin*/
} dbd_init_msg_t;

typedef struct dbd_fini_msg {
	uint16_t close_conn;  /* to close connection 1, 0 will keep
				 connection open */
	uint16_t commit;      /* to rollback(0) or commit(1) changes */
} dbd_fini_msg_t;

typedef struct dbd_job_comp_msg {
	char *	 admin_comment;	/* job admin comment field */
	uint32_t assoc_id;	/* accounting association id needed to
				 * find job record in db */
	char *	 comment;	/* job comment field */
	uint64_t db_index;	/* index into the db for this job */
	uint32_t derived_ec;	/* derived job exit code or signal */
	time_t   end_time;	/* job termintation time */
	char *failed_node;	/* Name of node that failed which caused
				 * this job to be killed.
				 * NULL in all other situations */
	uint32_t exit_code;	/* job exit code or signal */
	char *extra;		/* job extra field */
	uint32_t job_id;	/* job ID */
	uint32_t job_state;	/* job state */
	char *   nodes;		/* hosts allocated to the job */
	uint32_t req_uid;	/* requester user ID */
	time_t   start_time;	/* job start time */
	time_t   submit_time;	/* job submit time needed to find job
				 * record in db */
	char *	 system_comment;/* job system comment field */
	char    *tres_alloc_str;/* Simple comma separated list of TRES */
} dbd_job_comp_msg_t;

typedef struct dbd_job_start_msg {
	char *   account;       /* Account name for those not running
				 * with associations */
	uint32_t alloc_nodes;   /* how many nodes used in job */
	uint32_t array_job_id;	/* job_id of a job array or 0 if N/A */
	uint32_t array_max_tasks;/* max number of tasks able to run at once */
	uint32_t array_task_id;	/* task_id of a job array of NO_VAL
				 * if N/A */
	char *   array_task_str;/* hex string of unstarted tasks */
	uint32_t array_task_pending;/* number of tasks still pending */
	uint32_t assoc_id;	/* accounting association id */
	char *   constraints;   /* features/constraints requested by job */
	char *container;        /* OCI Container bundle path */
	uint32_t db_flags;      /* flags about job */
	uint64_t db_index;	/* index into the db for this job */
	time_t   eligible_time;	/* time job becomes eligible to run */
	char *env_hash;         /* hash value of env */
	uint32_t gid;	        /* group ID */
	uint32_t het_job_id;	/* ID of hetjob leader or 0 */
	uint32_t het_job_offset; /* Hetjob component ID, zero-origin */
	uint32_t job_id;	/* job ID */
	uint32_t job_state;	/* job state */
	char *licenses; 	/* job licenses */
	char *   mcs_label;	/* job mcs_label */
	char *   name;		/* job name */
	char *   nodes;		/* hosts allocated to the job */
	char *   node_inx;      /* ranged bitmap string of hosts
				 * allocated to the job */
	char *   partition;	/* partition job is running on */
	uint32_t priority;	/* job priority */
	uint32_t qos_id;        /* qos job is running with */
	char *qos_req;          /* qos(s) requested for the job */
	uint32_t req_cpus;	/* count of req processors */
	uint64_t req_mem;       /* requested minimum memory */
	uint16_t restart_cnt;   /* How many times the job has been restarted */
	uint32_t resv_id;	/* reservation id */
	char *resv_req;		/* original requested reservations */
	char *script_hash;      /* hash value of script */
	uint16_t segment_size;	/* requested segment size */
	time_t   start_time;	/* job start time */
	uint32_t state_reason_prev; /* Last reason of blocking before job
				     * started */
	char *std_err;          /* The stderr file path of the job */
	char *std_in;           /* The stdin file path of the job */
	char *std_out;          /* The stdout file path of the job */
	char *submit_line;      /* The command issued with all it's options in a
				 * string */
	time_t   submit_time;	/* job submit time */
	uint32_t timelimit;	/* job timelimit */
	uint32_t uid;	        /* user ID if associations are being used */
	char*    gres_used;     /* String depicting the GRES actually used by
				 * type for the entire job on all nodes. */
	char    *tres_alloc_str;/* Simple comma separated list of TRES */
	char    *tres_req_str;  /* Simple comma separated list of TRES */
	char *   wckey;		/* wckey name */
	char    *work_dir;      /* work dir of job */
} dbd_job_start_msg_t;

/* returns a uint32_t along with a return code */
typedef struct dbd_id_rc_msg {
	uint32_t job_id;
	uint64_t db_index;
	uint64_t flags;
	uint32_t return_code;
} dbd_id_rc_msg_t;

typedef struct {
	char *env;              /* job environment in text form */
	char *env_hash;         /* hash value of env */
	char *script;           /* job_script in text form */
	char *script_hash;      /* hash value of script */
	buf_t *script_buf;	/* job script as mmap buf */
} dbd_job_heavy_msg_t;

typedef struct dbd_job_suspend_msg {
	uint32_t assoc_id;	/* accounting association id needed
				 * to find job record in db */
	uint64_t db_index;	/* index into the db for this job */
	uint32_t job_id;	/* job ID needed to find job record
				 * in db */
	uint32_t job_state;	/* job state */
	time_t   submit_time;	/* job submit time needed to find job record
				 * in db */
	time_t   suspend_time;	/* job suspend or resume time */
} dbd_job_suspend_msg_t;

typedef struct {
	list_t *my_list;	/* this list could be of any type as long as it
				 * is handled correctly on both ends */
	uint32_t return_code;   /* If there was an error and a list of
				 * them this is the type of error it
				 * was */
} dbd_list_msg_t;

typedef struct {
	void *cond;
	void *rec;
} dbd_modify_msg_t;

#define DBD_NODE_STATE_DOWN  1
#define DBD_NODE_STATE_UP    2
#define DBD_NODE_STATE_UPDATE 3

typedef struct dbd_node_state_msg {
	time_t event_time;	/* time of transition */
	char *extra;		/* arbitrary string */
	char *hostlist;		/* name of hosts */
	char *instance_id;	/* cloud instance id */
	char *instance_type;	/* cloud instance type */
	uint16_t new_state;	/* new state of host, see DBD_NODE_STATE_* */
	char *reason;		/* explanation for the node's state */
	uint32_t reason_uid;   	/* User that set the reason, ignore if
				 * no reason is set. */
	uint32_t state;         /* current state of node.  Used to get
				   flags on the state (i.e. maintenance) */
	char *tres_str;	        /* Simple comma separated list of TRES */
} dbd_node_state_msg_t;

typedef struct dbd_register_ctld_msg {
	uint16_t dimensions;    /* dimensions of system */
	uint32_t flags;         /* flags for cluster */
	uint16_t port;		/* slurmctld's comm port */
	uint32_t cluster_id; /* cluster id of cluster making request */
} dbd_register_ctld_msg_t;

typedef struct dbd_step_comp_msg {
	uint32_t assoc_id;	/* accounting association id */
	uint64_t db_index;	/* index into the db for this job */
	time_t   end_time;	/* job termintation time */
	uint32_t exit_code;	/* job exit code or signal */
	jobacctinfo_t *jobacct; /* status info */
	time_t   job_submit_time;/* job submit time needed to find job record
				  * in db */
	char    *job_tres_alloc_str;/* Simple comma separated list of TRES for
				     * the job (primarily for the energy of the
				     * completing job.  This is only filled in
				     * on the last step in the job. */
	uint32_t req_uid;	/* requester user ID */
	time_t   start_time;	/* step start time */
	uint16_t state;         /* current state of node.  Used to get
				   flags on the state (i.e. maintenance) */
	slurm_step_id_t step_id;
	uint32_t total_tasks;	/* count of tasks for step */
} dbd_step_comp_msg_t;

typedef struct dbd_step_start_msg {
	uint32_t assoc_id;	/* accounting association id */
	uint64_t db_index;	/* index into the db for this job */
	char *container;	/* OCI Container Bundle path */
	char *   name;		/* step name */
	char *   nodes;		/* hosts allocated to the step */
	char *   node_inx;	/* bitmap index of hosts allocated to
				 * the step */
	uint32_t node_cnt;      /* how many nodes used in step */
	time_t   start_time;	/* step start time */
	time_t   job_submit_time;/* job submit time needed to find job record
				  * in db */
	uint32_t req_cpufreq_min; /* requested minimum CPU frequency  */
	uint32_t req_cpufreq_max; /* requested maximum CPU frequency  */
	uint32_t req_cpufreq_gov; /* requested CPU frequency governor */
	slurm_step_id_t step_id;
	char *cwd;              /* Current work dir of the step */
	char *std_err;          /* The stderr file path of the step */
	char *std_in;           /* The stdin file path of the step */
	char *std_out;          /* The stdout file path of the step */
	char *submit_line;      /* The command issued with all it's options in a
				 * string */
	uint32_t task_dist;     /* layout method of step */
	uint32_t time_limit; /* time limit of step */
	uint32_t total_tasks;	/* count of tasks for step */
	char *tres_alloc_str;   /* Simple comma separated list of TRES */
} dbd_step_start_msg_t;

typedef struct {
	void *data;
	uint16_t msg_type;
} dbd_relay_msg_t;

/*****************************************************************************\
 * Slurm DBD message processing functions
\*****************************************************************************/

extern slurmdbd_msg_type_t str_2_slurmdbd_msg_type(char *msg_type);
extern char *slurmdbd_msg_type_2_str(slurmdbd_msg_type_t msg_type,
				     int get_enum);

/*****************************************************************************\
 * Free various SlurmDBD message structures
\*****************************************************************************/
extern void slurmdbd_free_buffer(void *x);

extern void slurmdbd_free_acct_coord_msg(dbd_acct_coord_msg_t *msg);
extern void slurmdbd_free_cluster_tres_msg(dbd_cluster_tres_msg_t *msg);
extern void slurmdbd_free_msg(persist_msg_t *msg);
extern void slurmdbd_free_rec_msg(dbd_rec_msg_t *msg, slurmdbd_msg_type_t type);
extern void slurmdbd_free_cond_msg(dbd_cond_msg_t *msg,
				   slurmdbd_msg_type_t type);
extern void slurmdbd_free_fini_msg(dbd_fini_msg_t *msg);
extern void slurmdbd_free_job_complete_msg(dbd_job_comp_msg_t *msg);
extern void slurmdbd_free_job_start_msg(void *in);
extern void slurmdbd_free_job_heavy_msg(void *in);
extern void slurmdbd_free_id_rc_msg(void *in);
extern void slurmdbd_free_job_suspend_msg(dbd_job_suspend_msg_t *msg);
extern void slurmdbd_free_list_msg(dbd_list_msg_t *msg);
extern void slurmdbd_free_modify_msg(dbd_modify_msg_t *msg,
				     slurmdbd_msg_type_t type);
extern void slurmdbd_free_node_state_msg(dbd_node_state_msg_t *msg);
extern void slurmdbd_free_register_ctld_msg(dbd_register_ctld_msg_t *msg);
extern void slurmdbd_free_roll_usage_msg(dbd_roll_usage_msg_t *msg);
extern void slurmdbd_free_step_complete_msg(dbd_step_comp_msg_t *msg);
extern void slurmdbd_free_step_start_msg(dbd_step_start_msg_t *msg);
extern void slurmdbd_free_usage_msg(dbd_usage_msg_t *msg,
				    slurmdbd_msg_type_t type);

#endif	/* !_SLURMDBD_DEFS_H */

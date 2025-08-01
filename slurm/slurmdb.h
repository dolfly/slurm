/*****************************************************************************\
 *  slurmdb.h - Interface codes and functions for slurm
 ******************************************************************************
 *  Copyright (C) 2010 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Danny Auble da@llnl.gov, et. al.
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
#ifndef _SLURMDB_H
#define _SLURMDB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <slurm/slurm.h>

typedef enum {
	SLURMDB_ADMIN_NOTSET,
	SLURMDB_ADMIN_NONE,
	SLURMDB_ADMIN_OPERATOR,
	SLURMDB_ADMIN_SUPER_USER
} slurmdb_admin_level_t;

typedef enum {
	SLURMDB_CLASS_NONE, /* no class given */
	SLURMDB_CLASS_CAPABILITY, /* capability cluster */
	SLURMDB_CLASS_CAPACITY, /* capacity cluster */
	SLURMDB_CLASS_CAPAPACITY, /* a cluster that is both capability
				   * and capacity */
} slurmdb_classification_type_t;

typedef enum {
	SLURMDB_EVENT_ALL,
	SLURMDB_EVENT_CLUSTER,
	SLURMDB_EVENT_NODE
} slurmdb_event_type_t;

typedef enum {
	SLURMDB_PROBLEM_NOT_SET,
	SLURMDB_PROBLEM_ACCT_NO_ASSOC,
	SLURMDB_PROBLEM_ACCT_NO_USERS,
	SLURMDB_PROBLEM_USER_NO_ASSOC,
	SLURMDB_PROBLEM_USER_NO_UID,
} slurmdb_problem_type_t;

typedef enum {
	SLURMDB_REPORT_SORT_TIME,
	SLURMDB_REPORT_SORT_NAME
} slurmdb_report_sort_t;

typedef enum {
	SLURMDB_REPORT_TIME_SECS,
	SLURMDB_REPORT_TIME_MINS,
	SLURMDB_REPORT_TIME_HOURS,
	SLURMDB_REPORT_TIME_PERCENT,
	SLURMDB_REPORT_TIME_SECS_PER,
	SLURMDB_REPORT_TIME_MINS_PER,
	SLURMDB_REPORT_TIME_HOURS_PER,
} slurmdb_report_time_format_t;

typedef enum {
	SLURMDB_RESOURCE_NOTSET,
	SLURMDB_RESOURCE_LICENSE
} slurmdb_resource_type_t;

typedef enum {
	SLURMDB_UPDATE_NOTSET,
	SLURMDB_ADD_USER,
	SLURMDB_ADD_ASSOC,
	SLURMDB_ADD_COORD,
	SLURMDB_MODIFY_USER,
	SLURMDB_MODIFY_ASSOC,
	SLURMDB_REMOVE_USER,
	SLURMDB_REMOVE_ASSOC,
	SLURMDB_REMOVE_COORD,
	SLURMDB_ADD_QOS,
	SLURMDB_REMOVE_QOS,
	SLURMDB_MODIFY_QOS,
	SLURMDB_ADD_WCKEY,
	SLURMDB_REMOVE_WCKEY,
	SLURMDB_MODIFY_WCKEY,
	SLURMDB_ADD_CLUSTER,
	SLURMDB_REMOVE_CLUSTER,
	SLURMDB_REMOVE_ASSOC_USAGE,
	SLURMDB_ADD_RES,
	SLURMDB_REMOVE_RES,
	SLURMDB_MODIFY_RES,
	SLURMDB_UPDATE_QOS_USAGE,
	SLURMDB_ADD_TRES,
	SLURMDB_UPDATE_FEDS,
} slurmdb_update_type_t;

/* Define QOS flags */
typedef enum {
	QOS_FLAG_NONE = 0,
	QOS_FLAG_PART_MIN_NODE = SLURM_BIT(0),
	QOS_FLAG_PART_MAX_NODE = SLURM_BIT(1),
	QOS_FLAG_PART_TIME_LIMIT = SLURM_BIT(2),
	QOS_FLAG_ENFORCE_USAGE_THRES = SLURM_BIT(3),
	QOS_FLAG_NO_RESERVE = SLURM_BIT(4),
	QOS_FLAG_REQ_RESV = SLURM_BIT(5),
	QOS_FLAG_DENY_LIMIT = SLURM_BIT(6),
	QOS_FLAG_OVER_PART_QOS = SLURM_BIT(7),
	QOS_FLAG_NO_DECAY = SLURM_BIT(8),
	QOS_FLAG_USAGE_FACTOR_SAFE = SLURM_BIT(9),
	QOS_FLAG_RELATIVE = SLURM_BIT(10),
	QOS_FLAG_RELATIVE_SET = SLURM_BIT(11), // Not stored in the database
	QOS_FLAG_PART_QOS = SLURM_BIT(12), // Not stored in the database
	QOS_FLAG_DELETED = SLURM_BIT(13), // Not stored in the database

	/* 28+ are operators and will not be on a normal QOS rec. */
	QOS_FLAG_BASE = 0x0fffffff,
	QOS_FLAG_NOTSET = SLURM_BIT(28),
	QOS_FLAG_ADD = SLURM_BIT(29),
	QOS_FLAG_REMOVE = SLURM_BIT(30),
} slurmdb_qos_flags_t;

/* Define QOS Cond flags */
#define	QOS_COND_FLAG_WITH_DELETED SLURM_BIT(0)

/* Define Server Resource flags */
#define	SLURMDB_RES_FLAG_BASE        0x0fffffff /* apply to get real flags */
#define	SLURMDB_RES_FLAG_NOTSET      0x10000000
#define	SLURMDB_RES_FLAG_ADD         0x20000000
#define	SLURMDB_RES_FLAG_REMOVE      0x40000000

#define	SLURMDB_RES_FLAG_ABSOLUTE    SLURM_BIT(0)

/* Define Federation flags */
#define	FEDERATION_FLAG_BASE           0x0fffffff
#define	FEDERATION_FLAG_NOTSET         0x10000000
#define	FEDERATION_FLAG_ADD            0x20000000
#define	FEDERATION_FLAG_REMOVE         0x40000000

/* SLURM CLUSTER FEDERATION STATES */
enum cluster_fed_states {
	CLUSTER_FED_STATE_NA,
	CLUSTER_FED_STATE_ACTIVE,
	CLUSTER_FED_STATE_INACTIVE
};
#define CLUSTER_FED_STATE_BASE       0x000f
#define CLUSTER_FED_STATE_FLAGS      0xfff0
#define CLUSTER_FED_STATE_DRAIN      0x0010 /* drain cluster by not accepting
					       any new jobs and waiting for all
					       federated jobs to complete.*/
#define CLUSTER_FED_STATE_REMOVE     0x0020 /* remove cluster from federation
					       once cluster is drained of
					       federated jobs */

/* flags and types of resources */
/* when we come up with some */

/*
 * Translation of db_flags in job_record_t and flag
 * slurmdb_job_[rec|cond]_t
 */
#define SLURMDB_JOB_FLAG_NONE     0x00000000 /* No flags */
#define SLURMDB_JOB_CLEAR_SCHED   0x0000000f /* clear scheduling bits (0-3) */
#define SLURMDB_JOB_FLAG_NOTSET   SLURM_BIT(0) /* Schedule bits not set */
#define SLURMDB_JOB_FLAG_SUBMIT   SLURM_BIT(1) /* Job was started on submit */
#define SLURMDB_JOB_FLAG_SCHED    SLURM_BIT(2) /* Job was started from main
						* scheduler */
#define SLURMDB_JOB_FLAG_BACKFILL SLURM_BIT(3) /* Job was started from
						* backfill */
#define SLURMDB_JOB_FLAG_START_R  SLURM_BIT(4) /* Job start rpc was received */

/*
 * Slurm job condition flags
 * slurmdb_job_cond_t
 */
#define JOBCOND_FLAG_DUP              SLURM_BIT(0) /* Report duplicate job
						    * entries */
#define JOBCOND_FLAG_NO_STEP          SLURM_BIT(1) /* Don't report job step
						    * info */
#define JOBCOND_FLAG_NO_TRUNC         SLURM_BIT(2) /* Report info. without
						    * truncating the time to the
						    * usage_start and
						    * usage_end */
#define JOBCOND_FLAG_RUNAWAY          SLURM_BIT(3) /* Report runaway jobs onl */
#define JOBCOND_FLAG_WHOLE_HETJOB     SLURM_BIT(4) /* Report info about all
						    * hetjob components */
#define JOBCOND_FLAG_NO_WHOLE_HETJOB  SLURM_BIT(5) /* Only report info about
						    * requested hetjob
						    * components */
#define JOBCOND_FLAG_NO_WAIT          SLURM_BIT(6) /* Tell dbd plugin not to
						    * wait around for result. */
#define JOBCOND_FLAG_NO_DEFAULT_USAGE SLURM_BIT(7) /* Use usage_time as the
						    * submit_time of the job.
						    */
#define JOBCOND_FLAG_SCRIPT           SLURM_BIT(8) /* Get batch script only */
#define JOBCOND_FLAG_ENV              SLURM_BIT(9) /* Get job's env only */

/* Archive / Purge time flags */
#define SLURMDB_PURGE_BASE    0x0000ffff   /* Apply to get the number
					    * of units */
#define SLURMDB_PURGE_FLAGS   0xffff0000   /* apply to get the flags */
#define SLURMDB_PURGE_HOURS   0x00010000   /* Purge units are in hours */
#define SLURMDB_PURGE_DAYS    0x00020000   /* Purge units are in days */
#define SLURMDB_PURGE_MONTHS  0x00040000   /* Purge units are in months,
					    * the default */
#define SLURMDB_PURGE_ARCHIVE 0x00080000   /* Archive before purge */

/* Parent account should be used when calculating FairShare */
#define SLURMDB_FS_USE_PARENT 0x7FFFFFFF

#define SLURMDB_CLASSIFIED_FLAG 0x0100
#define SLURMDB_CLASS_BASE      0x00ff

/* Cluster flags */
typedef enum {
	CLUSTER_FLAG_NONE = 0,
	CLUSTER_FLAG_REGISTER = SLURM_BIT(0), /* Cluster is registering now */
	CLUSTER_FLAG_DELETED = SLURM_BIT(1), /* Cluster is deleted */
	/* SLURM_BIT(2) empty */
	/* SLURM_BIT(3) empty */
	/* SLURM_BIT(4) empty */
	/* SLURM_BIT(5) empty */
	/* SLURM_BIT(6) empty */
	CLUSTER_FLAG_MULTSD = SLURM_BIT(7), /* Cluster is multiple slurmd */
	/* SLURM_BIT(8) empty */
	/* SLURM_BIT(9) empty */
	/* SLURM_BIT(10) empty */
	CLUSTER_FLAG_FED = SLURM_BIT(11), /* This cluster is in a federation. */
	CLUSTER_FLAG_EXT = SLURM_BIT(12), /* This cluster is external */
}  slurmdb_cluster_flags_t;

/* Assoc flags */
typedef enum {
	ASSOC_FLAG_NONE = 0,
	ASSOC_FLAG_DELETED = SLURM_BIT(0),
	ASSOC_FLAG_NO_UPDATE = SLURM_BIT(1),
	ASSOC_FLAG_EXACT = SLURM_BIT(2), /* If looking for a partition based
					  * association don't return SUCCESS for
					  * a non-partition based association
					  * when calling
					  * assoc_mgr_fill_in_assoc() */
	ASSOC_FLAG_USER_COORD_NO = SLURM_BIT(3),

	/* Anything above this (0-15) will not be stored in the database. */
	ASSOC_FLAG_BASE = 0x0000ffff,

	ASSOC_FLAG_USER_COORD = SLURM_BIT(16),
	ASSOC_FLAG_BLOCK_ADD = SLURM_BIT(17),
} slurmdb_assoc_flags_t;

/* Assoc Cond flags */
#define	ASSOC_COND_FLAG_WITH_DELETED SLURM_BIT(0)
#define	ASSOC_COND_FLAG_WITH_USAGE SLURM_BIT(1)
#define	ASSOC_COND_FLAG_ONLY_DEFS SLURM_BIT(2)
#define	ASSOC_COND_FLAG_RAW_QOS SLURM_BIT(3)
#define	ASSOC_COND_FLAG_SUB_ACCTS SLURM_BIT(4)
#define	ASSOC_COND_FLAG_WOPI SLURM_BIT(5)
#define	ASSOC_COND_FLAG_WOPL SLURM_BIT(6)
#define	ASSOC_COND_FLAG_QOS_USAGE SLURM_BIT(7)
#define ASSOC_COND_FLAG_WITH_NG_USAGE SLURM_BIT(8)

/* Event condition flags */
#define SLURMDB_EVENT_COND_OPEN SLURM_BIT(0) /* Return only open events */

/* Flags for slurmdbd_conn->db_conn */
#define DB_CONN_FLAG_CLUSTER_DEL SLURM_BIT(0)
#define DB_CONN_FLAG_ROLLBACK SLURM_BIT(1)
#define DB_CONN_FLAG_FEDUPDATE SLURM_BIT(2)

/********************************************/

/* Association conditions used for queries of the database */

/* slurmdb_tres_rec_t is used in other structures below so this needs
 * to be declared before hand.
 */
typedef struct {
	uint64_t alloc_secs; /* total amount of secs allocated if used in an
				accounting_list */
	uint32_t rec_count;  /* number of records alloc_secs is, DON'T PACK */
	uint64_t count; /* Count of TRES on a given cluster, 0 if
			 * listed generically. */
	uint32_t id;    /* Database ID for the TRES */
	char *name;     /* Name of TRES if type is generic like GRES
			 * or License. Make include optional GRES type
			 * (e.g. "gpu" or "gpu:tesla") */
	char *type;     /* Type of TRES (CPU, MEM, etc) */
} slurmdb_tres_rec_t;

/* slurmdb_assoc_cond_t is used in other structures below so
 * this needs to be declared first.
 */
typedef struct {
	list_t *acct_list;		/* list of char * */
	list_t *cluster_list;		/* list of char * */

	list_t *def_qos_id_list;	/* list of char * */

	uint32_t flags;			/* Query flags */
	list_t *format_list;		/* list of char * */
	list_t *id_list;		/* list of char */

	list_t *parent_acct_list;	/* name of parent account */
	list_t *partition_list;		/* list of char * */

	list_t *qos_list;		/* list of char * */

	time_t usage_end;
	time_t usage_start;

	list_t *user_list;		/* list of char * */
} slurmdb_assoc_cond_t;

/* slurmdb_job_cond_t is used by slurmdb_archive_cond_t so it needs to
 * be defined before hand.
 */
typedef struct {
	list_t *acct_list;		/* list of char * */
	list_t *associd_list;		/* list of char */
	list_t *cluster_list;		/* list of char * */
	list_t *constraint_list; 	/* list of char * */
	uint32_t cpus_max;      	/* number of cpus high range */
	uint32_t cpus_min;      	/* number of cpus low range */
	uint32_t db_flags;      	/* flags sent from the slurmctld on the job */
	int32_t exitcode;       	/* exit code of job */
	uint32_t flags;         	/* Reporting flags*/
	list_t *format_list; 		/* list of char * */
	list_t *groupid_list;		/* list of char * */
	list_t *jobname_list;		/* list of char * */
	uint32_t nodes_max;		/* number of nodes high range */
	uint32_t nodes_min;		/* number of nodes low range */
	list_t *partition_list;		/* list of char * */
	list_t *qos_list;		/* list of char * */
	list_t *reason_list;		/* list of char * */
	list_t *resv_list;		/* list of char * */
	list_t *resvid_list;		/* list of char * */
	list_t *state_list;		/* list of char * */
	list_t *step_list;		/* list of slurm_selected_step_t */
	uint32_t timelimit_max;		/* max timelimit */
	uint32_t timelimit_min;		/* min timelimit */
	time_t usage_end;
	time_t usage_start;
	char *used_nodes;		/* a ranged node string where jobs ran */
	list_t *userid_list;		/* list of char * */
	list_t *wckey_list;		/* list of char * */
} slurmdb_job_cond_t;

/* slurmdb_stats_t needs to be defined before slurmdb_job_rec_t and
 * slurmdb_step_rec_t.
 */
typedef struct {
	double act_cpufreq;	/* contains actual average cpu frequency */
	uint64_t consumed_energy; /* contains energy consumption in joules */
	char *tres_usage_in_ave; /* average amount of usage in data */
	char *tres_usage_in_max; /* contains max amount of usage in data */
	char *tres_usage_in_max_nodeid; /* contains node number max was on */
	char *tres_usage_in_max_taskid; /* contains task number max was on */
	char *tres_usage_in_min; /* contains min amount of usage in data */
	char *tres_usage_in_min_nodeid; /* contains node number min was on */
	char *tres_usage_in_min_taskid; /* contains task number min was on */
	char *tres_usage_in_tot; /* total amount of usage in data */
	char *tres_usage_out_ave; /* average amount of usage out data */
	char *tres_usage_out_max; /* contains amount of max usage out data */
	char *tres_usage_out_max_nodeid; /* contains node number max was on */
	char *tres_usage_out_max_taskid; /* contains task number max was on */
	char *tres_usage_out_min; /* contains amount of min usage out data */
	char *tres_usage_out_min_nodeid; /* contains node number min was on */
	char *tres_usage_out_min_taskid; /* contains task number min was on */
	char *tres_usage_out_tot; /* total amount of usage out data */
} slurmdb_stats_t;

/************** alphabetical order of structures **************/

typedef enum {
	SLURMDB_ACCT_FLAG_NONE = 0,
	SLURMDB_ACCT_FLAG_DELETED = SLURM_BIT(0),
	SLURMDB_ACCT_FLAG_WASSOC = SLURM_BIT(1),
	SLURMDB_ACCT_FLAG_WCOORD = SLURM_BIT(2),
	SLURMDB_ACCT_FLAG_USER_COORD_NO = SLURM_BIT(3),

	/* Anything above this (0-15) will not be stored in the database. */
	SLURMDB_ACCT_FLAG_BASE = 0x0000ffff,

	SLURMDB_ACCT_FLAG_USER_COORD = SLURM_BIT(16),
} slurmdb_acct_flags_t;

typedef struct {
	slurmdb_assoc_cond_t *assoc_cond;/* use acct_list here for
						  names */
	list_t *description_list; /* list of char * */
	slurmdb_acct_flags_t flags;  /* SLURMDB_ACCT_FLAG_* */
	list_t *organization_list; /* list of char * */
} slurmdb_account_cond_t;

typedef struct {
	list_t *assoc_list; /* list of slurmdb_assoc_rec_t *'s */
	list_t *coordinators; /* list of slurmdb_coord_rec_t *'s */
	char *description;
	slurmdb_acct_flags_t flags; /* SLURMDB_ACCT_FLAG_* */
	char *name;
	char *organization;
} slurmdb_account_rec_t;

typedef struct {
	uint64_t alloc_secs; /* number of cpu seconds allocated */
	uint32_t id;	/* association/wckey ID		*/
	uint32_t id_alt; /* association/wckey ID */
	time_t period_start; /* when this record was started */
	slurmdb_tres_rec_t tres_rec;
} slurmdb_accounting_rec_t;

typedef struct {
	char *archive_dir;     /* location to place archive file */
	char *archive_script;  /* script to run instead of default
				  actions */
	slurmdb_job_cond_t *job_cond; /* conditions for the jobs to archive */
	uint32_t purge_event; /* purge events older than this in
			       * months by default set the
			       * SLURMDB_PURGE_ARCHIVE bit for
			       * archiving */
	uint32_t purge_job; /* purge jobs older than this in months
			     * by default set the
			     * SLURMDB_PURGE_ARCHIVE bit for
			     * archiving */
	uint32_t purge_resv; /* purge reservations older than this in months
			      * by default set the
			      * SLURMDB_PURGE_ARCHIVE bit for
			      * archiving */
	uint32_t purge_step; /* purge steps older than this in months
			      * by default set the
			      * SLURMDB_PURGE_ARCHIVE bit for
			      * archiving */
	uint32_t purge_suspend; /* purge suspend data older than this
				 * in months by default set the
				 * SLURMDB_PURGE_ARCHIVE bit for
				 * archiving */
	uint32_t purge_txn; /* purge transaction data older than this
			     * in months by default set the
			     * SLURMDB_PURGE_ARCHIVE bit for
			     * archiving */
	uint32_t purge_usage; /* purge usage data older than this
			       * in months by default set the
			       * SLURMDB_PURGE_ARCHIVE bit for
			       * archiving */
} slurmdb_archive_cond_t;

typedef struct {
	char *archive_file;  /* archive file containing data that was
				once flushed from the database */
	char *insert;     /* an sql statement to be ran containing the
			     insert of jobs since past */
} slurmdb_archive_rec_t;

typedef struct {
	uint64_t count;  /* Count of tres on a given cluster, 0 if
			    listed generically. */
	list_t *format_list;	/* list of char * */
	list_t *id_list;	/* Database ID */
	list_t *name_list;	/* Name of tres if type is generic like GRES
				   or License. */
	list_t *type_list;	/* Type of tres (CPU, MEM, etc) */
	uint16_t with_deleted;
} slurmdb_tres_cond_t;

/* slurmdb_tres_rec_t is defined above alphabetical */

/* slurmdb_assoc_cond_t is defined above alphabetical */

/* This has slurmdb_assoc_rec_t's in it so we define the struct afterwards. */
typedef struct slurmdb_assoc_usage slurmdb_assoc_usage_t;
typedef struct slurmdb_bf_usage slurmdb_bf_usage_t;
typedef struct slurmdb_user_rec slurmdb_user_rec_t;

typedef struct slurmdb_assoc_rec {
	list_t *accounting_list; /* list of slurmdb_accounting_rec_t *'s */
	char *acct;		   /* account/project associated to
				    * assoc */
	struct slurmdb_assoc_rec *assoc_next; /* next assoc with
						       * same hash index
						       * based off the
						       * account/user
						       * DOESN'T GET PACKED */
	struct slurmdb_assoc_rec *assoc_next_id; /* next assoc with
							* same hash index
							* DOESN'T GET PACKED */
	slurmdb_bf_usage_t *bf_usage; /* data for backfill scheduler,
				       * (DON'T PACK) */
	char *cluster;		   /* cluster associated to association */

	char *comment;		   /* comment for the association */

	uint32_t def_qos_id;       /* Which QOS id is this
				    * associations default */
	slurmdb_assoc_flags_t flags; /* various flags see ASSOC_FLAG_* */
	uint32_t grp_jobs;	   /* max number of jobs the
				    * underlying group of associations can run
				    * at one time */
	uint32_t grp_jobs_accrue;  /* max number of jobs the
				    * underlying group of associations can have
				    * accruing priority at one time */
	uint32_t grp_submit_jobs;  /* max number of jobs the
				    * underlying group of
				    * associations can submit at
				    * one time */
	char *grp_tres;            /* max number of cpus the
				    * underlying group of
				    * associations can allocate at one time */
	uint64_t *grp_tres_ctld;   /* grp_tres broken out in an array
				    * based off the ordering of the total
				    * number of TRES in the system
				    * (DON'T PACK) */
	char *grp_tres_mins;       /* max number of cpu minutes the
				    * underlying group of
				    * associations can run for */
	uint64_t *grp_tres_mins_ctld; /* grp_tres_mins broken out in an array
				       * based off the ordering of the total
				       * number of TRES in the system
				       * (DON'T PACK) */
	char *grp_tres_run_mins;   /* max number of cpu minutes the
				    * underlying group of
				    * associations can
				    * having running at one time */
	uint64_t *grp_tres_run_mins_ctld; /* grp_tres_run_mins
					   * broken out in an array
					   * based off the ordering
					   * of the total number of TRES in
					   * the system
					   * (DON'T PACK) */
	uint32_t grp_wall;         /* total time in hours the
				    * underlying group of
				    * associations can run for */

	uint32_t id;		   /* id identifying a combination of
				    * user-account(-partition) */

	uint16_t is_def;           /* Is this the users default assoc/acct */

	slurmdb_assoc_usage_t *leaf_usage; /* Points to usage for user assocs.
					    * Holds usage of deleted users in
					    * parent assocs (DON'T PACK) */
	char *lineage;		   /* Complete path up the hierarchy to the root
				    * association */
	uint32_t max_jobs;	   /* max number of jobs this
				    * association can run at one time */
	uint32_t max_jobs_accrue;  /* max number of jobs this association can
				    * have accruing priority time.
				    */
	uint32_t max_submit_jobs;  /* max number of jobs that can be
				      submitted by association */
	char *max_tres_mins_pj;    /* max number of cpu minutes this
				    * association can have per job */
	uint64_t *max_tres_mins_ctld; /* max_tres_mins broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	char *max_tres_run_mins;   /* max number of cpu minutes this
				    * association can
				    * having running at one time */
	uint64_t *max_tres_run_mins_ctld; /* max_tres_run_mins
					   * broken out in an array
					   * based off the ordering
					   * of the total number of TRES in
					   * the system
					   * (DON'T PACK) */
	char *max_tres_pj;         /* max number of cpus this
				    * association can allocate per job */
	uint64_t *max_tres_ctld;   /* max_tres broken out in an array
				    * based off the ordering of the
				    * total number of TRES in the system
				    * (DON'T PACK) */
	char *max_tres_pn;         /* max number of TRES this
				    * association can allocate per node */
	uint64_t *max_tres_pn_ctld;   /* max_tres_pn broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	uint32_t max_wall_pj;      /* longest time this
				    * association can run a job */

	uint32_t min_prio_thresh;  /* Don't reserve resources for pending jobs
				    * unless they have a priority equal to or
				    * higher than this. */
	char *parent_acct;	   /* name of parent account */
	uint32_t parent_id;	   /* id of parent account */
	char *partition;	   /* optional partition in a cluster
				    * associated to association */
	uint32_t priority;	   /* association priority */
	list_t *qos_list;          /* list of char * */

	uint32_t shares_raw;	   /* number of shares allocated to
				    * association */

	uint32_t uid;		   /* user ID */
	slurmdb_assoc_usage_t *usage;
	char *user;		   /* user associated to assoc */
	slurmdb_user_rec_t *user_rec; /* Cache of user record
				       * soft ref - mem not managed here
				       * (DON'T PACK)
				       */
} slurmdb_assoc_rec_t;

typedef struct {
	list_t *acct_list;	/* list of char * */
	slurmdb_assoc_rec_t assoc; /* filled with limits for associations
				      to be added. */
	list_t *cluster_list; /* list of char * */

	char *default_acct; /* default account name (DON'T PACK) */

	list_t *partition_list; /* list of char * */
	list_t *user_list;	/* list of char * */
	list_t *wckey_list;	/* list of char * */
} slurmdb_add_assoc_cond_t;

struct slurmdb_assoc_usage {
	uint32_t accrue_cnt;    /* Count of how many jobs I have accuring prio
				 * (DON'T PACK for state file) */
	list_t *children_list;	/* list of children associations
				 * (DON'T PACK) */
	bitstr_t *grp_node_bitmap;	/* Bitmap of allocated nodes
					 * (DON'T PACK) */
	uint16_t *grp_node_job_cnt;	/* Count of jobs allocated on each node
					 * (DON'T PACK) */
	uint64_t *grp_used_tres; /* array of active tres counts
				  * (DON'T PACK for state file) */
	uint64_t *grp_used_tres_run_secs; /* array of running tres secs
					   * (DON'T PACK for state file) */

	double grp_used_wall;   /* group count of time used in running jobs */
	double fs_factor;	/* Fairshare factor. Not used by all algorithms
				 * (DON'T PACK for state file) */
	uint32_t level_shares;  /* number of shares on this level of
				 * the tree (DON'T PACK for state file) */

	slurmdb_assoc_rec_t *parent_assoc_ptr; /* ptr to direct
						* parent assoc
						* set in slurmctld
						* (DON'T PACK) */

	double priority_norm;   /* normalized priority (DON'T PACK for
				 * state file) */

	slurmdb_assoc_rec_t *fs_assoc_ptr;    /* ptr to fairshare parent
					       * assoc if fairshare
					       * == SLURMDB_FS_USE_PARENT
					       * set in slurmctld
					       * (DON'T PACK) */

	double shares_norm;     /* normalized shares
				 * (DON'T PACK for state file) */

	uint32_t tres_cnt; /* size of the tres arrays,
			    * (DON'T PACK for state file) */
	long double usage_efctv;/* effective, normalized usage
				 * (DON'T PACK for state file) */
	long double usage_norm;	/* normalized usage
				 * (DON'T PACK for state file) */
	long double usage_raw;	/* measure of TRESBillableUnits usage */

	long double *usage_tres_raw; /* measure of each TRES usage */
	uint32_t used_jobs;	/* count of active jobs
				 * (DON'T PACK for state file) */
	uint32_t used_submit_jobs; /* count of jobs pending or running
				    * (DON'T PACK for state file) */

	/* Currently FAIR_TREE systems are defining data on
	 * this struct but instead we could keep a void pointer to system
	 * specific data. This would allow subsystems to define whatever data
	 * they need without having to modify this struct; it would also save
	 * space.
	 */
	long double level_fs;	/* (FAIR_TREE) Result of fairshare equation
				 * compared to the association's siblings
				 * (DON'T PACK for state file) */

	bitstr_t *valid_qos;    /* qos available for this association
				 * derived from the qos_list.
				 * (DON'T PACK for state file) */
};

struct slurmdb_bf_usage {
	uint64_t count;
	time_t last_sched;
};

typedef struct {
	uint16_t classification; /* how this machine is classified */
	list_t *cluster_list; /* list of char * */
	list_t *federation_list; /* list of char */
	slurmdb_cluster_flags_t flags;
	list_t *format_list; 	/* list of char * */
	list_t *rpc_version_list; /* list of char * */
	time_t usage_end;
	time_t usage_start;
	uint16_t with_deleted;
	uint16_t with_usage;
} slurmdb_cluster_cond_t;

typedef struct {
	list_t *feature_list; /* list of cluster features */
	uint32_t id; /* id of cluster in federation */
	char *name; /* Federation name */
	void *recv;  /* slurm_persist_conn_t we recv information about this
		      * sibling on. (We get this information) */
	void *send; /* slurm_persist_conn_t we send information to this
		     * cluster on. (We set this information) */
	uint32_t state; /* state of cluster in federation */
	bool sync_recvd; /* true sync jobs from sib has been processed. */
	bool sync_sent;  /* true after sib sent sync jobs to sibling */
} slurmdb_cluster_fed_t;

struct slurmdb_cluster_rec {
	list_t *accounting_list; /* list of slurmdb_cluster_accounting_rec_t *'s */
	uint16_t classification; /* how this machine is classified */
	time_t comm_fail_time;	/* avoid constant error messages. For
			         * convenience only. DOESN'T GET PACKED */
	slurm_addr_t control_addr; /* For convenience only.
				    * DOESN'T GET PACKED */
	char *control_host;
	uint32_t control_port;
	uint16_t dimensions; /* number of dimensions this cluster is */
	int *dim_size; /* For convenience only.
			* Size of each dimension For now only on
			* a bluegene cluster.  DOESN'T GET
			* PACKED, is set up in slurmdb_get_info_cluster */
	uint16_t id; /* unique id */
	slurmdb_cluster_fed_t fed; /* Federation information */
	slurmdb_cluster_flags_t flags;
	pthread_mutex_t lock; /* For convenience only. DOESN"T GET PACKED */
	char *name;
	char *nodes;
	slurmdb_assoc_rec_t *root_assoc; /* root assoc for
						* cluster */
	uint16_t rpc_version; /* rpc version this cluster is running */
	list_t *send_rpc;     /* For convenience only. DOESN'T GET PACKED */
	char  	*tres_str;    /* comma separated list of TRES */
};

#ifndef __slurmdb_cluster_rec_t_defined
#  define __slurmdb_cluster_rec_t_defined
typedef struct slurmdb_cluster_rec slurmdb_cluster_rec_t;
#endif

typedef struct {
	uint64_t alloc_secs; /* number of cpu seconds allocated */
	uint64_t down_secs; /* number of cpu seconds down */
	uint64_t idle_secs; /* number of cpu seconds idle */
	uint64_t over_secs; /* number of cpu seconds overcommitted */
	uint64_t pdown_secs; /* number of cpu seconds planned down */
	time_t period_start; /* when this record was started */
	uint64_t plan_secs; /* number of cpu seconds planned */
	slurmdb_tres_rec_t tres_rec;
} slurmdb_cluster_accounting_rec_t;

typedef struct {
	char *cluster; /* name of cluster */
	uint32_t allowed; /* percentage/count of total resources
			   * allowed for this cluster */
} slurmdb_clus_res_rec_t;

#define COORD_SET_INDIRECT 0
#define COORD_SET_DIRECT 1
#define COORD_SET_BY_ACCT 2

typedef struct {
	char *name;
	uint16_t direct;
} slurmdb_coord_rec_t;

typedef struct {
	list_t *cluster_list;	/* list of char * */
	uint32_t cond_flags;    /* condition flags */
	uint32_t cpus_max;      /* number of cpus high range */
	uint32_t cpus_min;      /* number of cpus low range */
	uint16_t event_type;    /* type of events (slurmdb_event_type_t),
				 * default is all */
	list_t *format_list; 	/* list of char * */
	char *node_list;        /* node list string */
	time_t period_end;      /* period end of events */
	time_t period_start;    /* period start of events */
	list_t *reason_list;    /* list of char * */
	list_t *reason_uid_list;/* list of char * */
	list_t *state_list;     /* list of char * */
} slurmdb_event_cond_t;

typedef struct {
	char *cluster;          /* Name of associated cluster */
	char *cluster_nodes;    /* node list in cluster during time
				 * period (only set in a cluster event) */
	uint16_t event_type;    /* type of event (slurmdb_event_type_t) */
	char *node_name;        /* Name of node (only set in a node event) */
	time_t period_end;      /* End of period */
	time_t period_start;    /* Start of period */
	char *reason;           /* reason node is in state during time
				   period (only set in a node event) */
	uint32_t reason_uid;    /* uid of that who set the reason */
	uint32_t state;         /* State of node during time
				   period (only set in a node event) */
	char *tres_str;         /* TRES touched by this event */
} slurmdb_event_rec_t;

typedef struct {
	list_t *cluster_list; 		/* list of char * */
	list_t *federation_list; 	/* list of char * */
	list_t *format_list; 		/* list of char * */
	uint16_t with_deleted;
} slurmdb_federation_cond_t;

typedef struct {
	char     *name;		/* Name of federation */
	uint32_t  flags; 	/* flags to control scheduling on controller */
	list_t *cluster_list;	/* List of slurmdb_cluster_rec_t *'s */
} slurmdb_federation_rec_t;

typedef struct {
	list_t *cluster_list; /* list of char * */
	list_t *extra_list; /* list of char * */
	list_t *format_list; /* list of char * */
	list_t *instance_id_list; /* list of char * */
	list_t *instance_type_list; /* list of char * */
	char *node_list; /* node list string */
	time_t time_end; /* time end of instances */
	time_t time_start; /* time start of instances */
} slurmdb_instance_cond_t;

typedef struct {
	char *cluster; /* name of associated cluster */
	char *extra; /* name of instance_id */
	char *instance_id; /* name of instance_id */
	char *instance_type; /* name of instance_id */
	char *node_name; /* name of node */
	time_t time_end; /* time end of instance */
	time_t time_start; /* time start of instance */
} slurmdb_instance_rec_t;

/* slurmdb_job_cond_t is defined above alphabetical */

typedef struct {
	char    *account;
	char	*admin_comment;
	uint32_t alloc_nodes;
	uint32_t array_job_id;	/* job_id of a job array or 0 if N/A */
	uint32_t array_max_tasks; /* How many tasks of the array can be
				     running at one time.
				  */
	uint32_t array_task_id;	/* task_id of a job array of NO_VAL
				 * if N/A */
	char    *array_task_str; /* If pending these are the array
				    tasks this record represents.
				 */
	uint32_t associd;
	char	*blockid;
	char    *cluster;
	char    *constraints;
	char *container; /* OCI Container Bundle path */
	uint64_t db_index; /* index in the table */
	uint32_t derived_ec;
	char	*derived_es; /* aka "comment" */
	uint32_t elapsed;
	time_t eligible;
	time_t end;
	char *env;
	uint32_t exitcode;
	char *extra; /* Extra - arbitrary string */
	char *failed_node;
	uint32_t flags;
	void *first_step_ptr;
	uint32_t gid;
	uint32_t het_job_id;
	uint32_t het_job_offset;
	uint32_t jobid;
	char	*jobname;
	char *lineage;		   /* Complete path up the hierarchy to the root
				    * association */
	char *licenses;
	char 	*mcs_label;
	char	*nodes;
	char	*partition;
	uint32_t priority;
	uint32_t qosid;
	char *qos_req;
	uint32_t req_cpus;
	uint64_t req_mem;
	uint32_t requid;
	uint16_t restart_cnt;
	uint32_t resvid;
	char *resv_name;
	char *resv_req; /* original requested reservations */
	char *script;
	uint16_t segment_size;
	uint32_t show_full;
	time_t start;
	uint32_t state;
	uint32_t state_reason_prev;
	list_t *steps; /* list of slurmdb_step_rec_t *'s */
	char *std_err;
	char *std_in;
	char *std_out;
	time_t submit;
	char *submit_line;
	uint32_t suspended;
	char	*system_comment;
	uint64_t sys_cpu_sec;
	uint64_t sys_cpu_usec;
	uint32_t timelimit;
	uint64_t tot_cpu_sec;
	uint64_t tot_cpu_usec;
	char *tres_alloc_str;
	char *tres_req_str;
	uint32_t uid;
	char 	*used_gres;
	char    *user;
	uint64_t user_cpu_sec;
	uint64_t user_cpu_usec;
	char    *wckey;
	uint32_t wckeyid;
	char    *work_dir;
} slurmdb_job_rec_t;

typedef struct {
	uint32_t accrue_cnt;    /* Count of how many jobs I have accuring prio
				 * (DON'T PACK for state file) */
	list_t *acct_limit_list; /* slurmdb_used_limits_t's (DON'T PACK
			          * for state file) */
	list_t *job_list; /* list of job pointers to submitted/running
			     jobs (DON'T PACK) */
	bitstr_t *grp_node_bitmap;	/* Bitmap of allocated nodes
					 * (DON'T PACK) */
	uint16_t *grp_node_job_cnt;	/* Count of jobs allocated on each node
					 * (DON'T PACK) */
	uint32_t grp_used_jobs;	/* count of active jobs (DON'T PACK
				 * for state file) */
	uint32_t grp_used_submit_jobs; /* count of jobs pending or running
					* (DON'T PACK for state file) */
	uint64_t *grp_used_tres; /* count of tres in use in this qos
				 * (DON'T PACK for state file) */
	uint64_t *grp_used_tres_run_secs; /* count of running tres secs
					 * (DON'T PACK for state file) */
	double grp_used_wall;   /* group count of time (minutes) used in
				 * running jobs */
	double norm_priority;/* normalized priority (DON'T PACK for
			      * state file) */
	uint32_t tres_cnt; /* size of the tres arrays,
			    * (DON'T PACK for state file) */
	long double usage_raw;	/* measure of resource usage */

	long double *usage_tres_raw; /* measure of each TRES usage */
	list_t *user_limit_list; /* slurmdb_used_limits_t's (DON'T PACK
				  * for state file) */
} slurmdb_qos_usage_t;

typedef struct {
	time_t blocked_until; /* internal use only, DON'T PACK  */
	char *description;
	uint32_t id;
	slurmdb_qos_flags_t flags; /* flags for various things to enforce or
				      override other limits */
	uint32_t grace_time; /* preemption grace time */
	uint32_t grp_jobs_accrue; /* max number of jobs this qos can
				   * have accruing priority time
				   */
	uint32_t grp_jobs;	/* max number of jobs this qos can run
				 * at one time */
	uint32_t grp_submit_jobs; /* max number of jobs this qos can submit at
				   * one time */
	char *grp_tres;            /* max number of tres this qos can
				    * allocate at one time */
	uint64_t *grp_tres_ctld;   /* grp_tres broken out in an array
				    * based off the ordering of the total
				    * number of TRES in the system
				    * (DON'T PACK) */
	char *grp_tres_mins;       /* max number of tres minutes this
				    * qos can run for */
	uint64_t *grp_tres_mins_ctld; /* grp_tres_mins broken out in an array
				       * based off the ordering of the total
				       * number of TRES in the system
				       * (DON'T PACK) */
	char *grp_tres_run_mins;   /* max number of tres minutes this
				    * qos can have running at one time */
	uint64_t *grp_tres_run_mins_ctld; /* grp_tres_run_mins
					   * broken out in an array
					   * based off the ordering
					   * of the total number of TRES in
					   * the system
					   * (DON'T PACK) */
	uint32_t grp_wall; /* total time in hours this qos can run for */

	double limit_factor; /* factor to apply to tres_cnt for associations
			      * using this qos */
	uint32_t max_jobs_pa;	/* max number of jobs an account can
				 * run with this qos at one time */
	uint32_t max_jobs_pu;	/* max number of jobs a user can
				 * run with this qos at one time */
	uint32_t max_jobs_accrue_pa; /* max number of jobs an account can
				      * have accruing priority time
				      */
	uint32_t max_jobs_accrue_pu; /* max number of jobs a user can
				      * have accruing priority time
				      */
	uint32_t max_submit_jobs_pa; /* max number of jobs an account can
					submit with this qos at once */
	uint32_t max_submit_jobs_pu; /* max number of jobs a user can
					submit with this qos at once */
	char *max_tres_mins_pj;    /* max number of tres minutes this
				    * qos can have per job */
	uint64_t *max_tres_mins_pj_ctld; /* max_tres_mins broken out in an array
					  * based off the ordering of the
					  * total number of TRES in the system
					  * (DON'T PACK) */
	char *max_tres_pa;         /* max number of tres this
				    * QOS can allocate per account */
	uint64_t *max_tres_pa_ctld;   /* max_tres_pa broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	char *max_tres_pj;         /* max number of tres this
				    * qos can allocate per job */
	uint64_t *max_tres_pj_ctld;   /* max_tres_pj broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	char *max_tres_pn;         /* max number of tres this
				    * qos can allocate per job */
	uint64_t *max_tres_pn_ctld;   /* max_tres_pj broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	char *max_tres_pu;         /* max number of tres this
				    * QOS can allocate per user */
	uint64_t *max_tres_pu_ctld;   /* max_tres broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */
	char *max_tres_run_mins_pa;   /* max number of tres minutes this
				       * qos can having running at one
				       * time per account, currently
				       * this doesn't do anything.
				       */
	uint64_t *max_tres_run_mins_pa_ctld; /* max_tres_run_mins_pa
					      * broken out in an array
					      * based off the ordering
					      * of the total number of TRES in
					      * the system, currently
					      * this doesn't do anything.
					      * (DON'T PACK) */
	char *max_tres_run_mins_pu;   /* max number of tres minutes this
				       * qos can having running at one
				       * time, currently this doesn't
				       * do anything.
				       */
	uint64_t *max_tres_run_mins_pu_ctld; /* max_tres_run_mins_pu
					      * broken out in an array
					      * based off the ordering
					      * of the total number of TRES in
					      * the system, currently
					      * this doesn't do anything.
					      * (DON'T PACK) */
	uint32_t max_wall_pj; /* longest time this
			       * qos can run a job */
	uint32_t min_prio_thresh;  /* Don't reserve resources for pending jobs
				    * unless they have a priority equal to or
				    * higher than this. */
	char *min_tres_pj; /* min number of tres a job can
			    * allocate with this qos */
	uint64_t *min_tres_pj_ctld;   /* min_tres_pj broken out in an array
				       * based off the ordering of the
				       * total number of TRES in the system
				       * (DON'T PACK) */

	char *name;
	bitstr_t *preempt_bitstr; /* other qos' this qos can preempt */
	list_t *preempt_list;  /* list of char *'s only used to add or
				* change the other qos' this can preempt,
				* when doing a get use the preempt_bitstr */
	uint16_t preempt_mode;	/* See PREEMPT_MODE_* in slurm/slurm.h */
	uint32_t preempt_exempt_time; /* Job run time before becoming
					 eligible for preemption */
	uint32_t priority;  /* ranged int needs to be a unint for
			     * heterogeneous systems */
	uint64_t *relative_tres_cnt; /* Only here for convenience DON'T PACK */
	slurmdb_qos_usage_t *usage; /* For internal use only, DON'T PACK */
	double usage_factor; /* factor to apply to usage in this qos */
	double usage_thres; /* percent of effective usage of an
			       association when breached will deny
			       pending and new jobs */
} slurmdb_qos_rec_t;

typedef struct {
	list_t *description_list; /* list of char * */
	uint16_t flags; /* See QOS_COND_FLAG_* */
	list_t *id_list; /* list of char * */
	list_t *format_list;/* list of char * */
	list_t *name_list; /* list of char * */
	uint16_t preempt_mode;	/* See PREEMPT_MODE_* in slurm/slurm.h */
} slurmdb_qos_cond_t;

typedef struct {
	list_t *cluster_list; /* cluster reservations. list of char * */
	uint64_t flags; /* flags for reservation. */
	list_t *format_list; /* list of char * */
	list_t *id_list; /* ids of reservations. list of char * */
	list_t *name_list; /* name of reservations. list of char * */
	char *nodes; /* list of nodes in reservation */
	time_t time_end; /* end time of reservation */
	time_t time_start; /* start time of reservation */
	uint16_t with_usage; /* send usage for reservation */
} slurmdb_reservation_cond_t;

typedef struct {
	char *assocs; /* comma separated list of associations */
	char *cluster; /* cluster reservation is for */
	char *comment; /* arbitrary comment assigned to reservation */
	uint64_t flags; /* flags for reservation. */
	uint32_t id;   /* id of reservation. */
	char *name; /* name of reservation */
	char *nodes; /* list of nodes in reservation */
	char *node_inx; /* node index of nodes in reservation */
	time_t time_end; /* end time of reservation */
	time_t time_force; /* The actual time the reservation started */
	time_t time_start; /* start time of reservation */
	time_t time_start_prev; /* If start time was changed this is
				 * the previous start time.  Needed
				 * for accounting */
	char *tres_str;
	double unused_wall; /* amount of seconds this reservation wasn't used */
	list_t *tres_list; /* list of slurmdb_tres_rec_t, only set when
			    * job usage is requested. */
} slurmdb_reservation_rec_t;

typedef struct {
	char *container;
	char *cwd;
	uint32_t elapsed;
	time_t end;
	int32_t exitcode;
	slurmdb_job_rec_t *job_ptr;
	uint32_t nnodes;
	char *nodes;
	uint32_t ntasks;
	char *pid_str;
	uint32_t req_cpufreq_min;
	uint32_t req_cpufreq_max;
	uint32_t req_cpufreq_gov;
	uint32_t requid;
	time_t start;
	uint32_t state;
	slurmdb_stats_t stats;
	slurm_step_id_t step_id;	/* job's step number */
	char *stepname;
	char *std_err;
	char *std_in;
	char *std_out;
	char *submit_line;
	uint32_t suspended;
	uint64_t sys_cpu_sec;
	uint32_t sys_cpu_usec;
	uint32_t task_dist;
	uint32_t timelimit;
	uint64_t tot_cpu_sec;
	uint32_t tot_cpu_usec;
	char *tres_alloc_str;
	uint64_t user_cpu_sec;
	uint32_t user_cpu_usec;
} slurmdb_step_rec_t;

/* slurmdb_stats_t defined above alphabetical */

typedef struct {
	list_t *allowed_list; /* list of char * */
	list_t *cluster_list; /* list of char * */
	list_t *description_list; /* list of char * */
	uint32_t flags;
	list_t *format_list;/* list of char * */
	list_t *id_list; /* list of char * */
	list_t *manager_list; /* list of char * */
	list_t *name_list; /* list of char * */
	list_t *server_list; /* list of char * */
	list_t *type_list; /* list of char * */
	uint16_t with_deleted;
	uint16_t with_clusters;
} slurmdb_res_cond_t;

typedef struct {
	uint32_t allocated; /* count allocated to the clus_res_list */
	uint32_t last_consumed; /* number from the server saying how many it
				 * currently has consumed */
	list_t *clus_res_list; /* list of slurmdb_clus_res_rec_t *'s */
	slurmdb_clus_res_rec_t *clus_res_rec; /* if only one cluster
						 being represented */
	uint32_t count; /* count of resources managed on the server */
	char *description;
	uint32_t flags; /* resource attribute flags */
	uint32_t id;
	time_t last_update;
	char *manager;  /* resource manager name */
	char *name;
	char *server;  /* resource server name */
	uint32_t type; /* resource type */
} slurmdb_res_rec_t;

typedef struct {
	list_t *acct_list; /* list of char * */
	list_t *action_list; /* list of char * */
	list_t *actor_list; /* list of char * */
	list_t *cluster_list; /* list of char * */
	list_t *format_list;/* list of char * */
	list_t *id_list; /* list of char * */
	list_t *info_list; /* list of char * */
	list_t *name_list; /* list of char * */
	time_t time_end;
	time_t time_start;
	list_t *user_list; /* list of char * */
	uint16_t with_assoc_info;
} slurmdb_txn_cond_t;

typedef struct {
	char *accts;
	uint16_t action;
	char *actor_name;
	char *clusters;
	uint32_t id;
	char *set_info;
	time_t timestamp;
	char *users;
	char *where_query;
} slurmdb_txn_rec_t;

/* Right now this is used in the slurmdb_qos_rec_t structure.  In the
 * user_limit_list and acct_limit_list. */
typedef struct {
	uint32_t accrue_cnt; /* count of jobs accruing prio */
	char *acct; /* If limits for an account this is the accounts name */
	uint32_t jobs;	/* count of active jobs */
	uint32_t submit_jobs; /* count of jobs pending or running */
	uint64_t *tres; /* array of TRES allocated */
	uint64_t *tres_run_secs; /* array of how many TRES secs are
				  * allocated currently */
	bitstr_t *node_bitmap;	/* Bitmap of allocated nodes */
	uint16_t *node_job_cnt;	/* Count of jobs allocated on each node */
	uint32_t uid; /* If limits for a user this is the users uid */
} slurmdb_used_limits_t;

typedef struct {
	uint16_t admin_level; /* really slurmdb_admin_level_t but for
				 packing purposes needs to be uint16_t */
	slurmdb_assoc_cond_t *assoc_cond; /* use user_list here for names */
	list_t *def_acct_list; /* list of char * (We can't really use
				* the assoc_cond->acct_list for this
				* because then it is impossible for us
				* to tell which accounts are defaults
				* and which ones aren't, especially when
				* dealing with other versions.)*/
	list_t *def_wckey_list; /* list of char * */
	uint16_t with_assocs;
	uint16_t with_coords;
	uint16_t with_deleted;
	uint16_t with_wckeys;
	uint16_t without_defaults;
} slurmdb_user_cond_t;

enum {
	SLURMDB_USER_FLAG_NONE		= 0,
	SLURMDB_USER_FLAG_DELETED	= SLURM_BIT(0),
};

struct slurmdb_user_rec {
	uint16_t admin_level; /* really slurmdb_admin_level_t but for
				 packing purposes needs to be uint16_t */
	list_t *assoc_list; /* list of slurmdb_assoc_rec_t *'s */
	slurmdb_bf_usage_t *bf_usage; /* data for backfill scheduler,
				       * (DON'T PACK) */
	list_t *coord_accts; /* list of slurmdb_coord_rec_t *'s */
	char *default_acct;
	char *default_wckey;
	uint32_t flags;		/* SLURMDB_USER_FLAG_* */
	char *name;
	char *old_name;
	uint32_t uid;
	list_t *wckey_list; /* list of slurmdb_wckey_rec_t *'s */
};

typedef struct {
	list_t *objects; /* depending on type */
	uint16_t type; /* really slurmdb_update_type_t but for
			* packing purposes needs to be a
			* uint16_t */
} slurmdb_update_object_t;

typedef struct {
	list_t *cluster_list;	/* list of char * */
	list_t *format_list;	/* list of char * */
	list_t *id_list;	/* list of char * */

	list_t *name_list;	/* list of char * */

	uint16_t only_defs;     /* only give me the defaults */

	time_t usage_end;
	time_t usage_start;

	list_t *user_list;	/* list of char * */

	uint16_t with_usage;    /* fill in usage */
	uint16_t with_deleted;  /* return deleted associations */
} slurmdb_wckey_cond_t;

enum {
	SLURMDB_WCKEY_FLAG_NONE          = 0,
	SLURMDB_WCKEY_FLAG_DELETED       = SLURM_BIT(0),
};

typedef struct {
	list_t *accounting_list; /* list of slurmdb_accounting_rec_t *'s */
	char *cluster;		/* cluster associated */
	uint32_t flags;		/* SLURMDB_WCKEY_FLAG_* */
	uint32_t id;		/* id identifying a combination of
				 * user-wckey-cluster */
	uint16_t is_def;        /* Is this the users default wckey */

	char *name;		/* wckey name */
	uint32_t uid;		/* user ID */

	char *user;		/* user associated */
} slurmdb_wckey_rec_t;

typedef struct {
	char *name;
	char *print_name;
	char *spaces;
	uint16_t user; /* set to 1 if it is a user i.e. if name[0] is
			* '|' */
} slurmdb_print_tree_t;

typedef struct {
	slurmdb_assoc_rec_t *assoc;
	char *key;
	char *sort_name;
	list_t *children;
} slurmdb_hierarchical_rec_t;

/************** report specific structures **************/

typedef struct {
	char *acct;
	char *cluster;
	uint32_t id;
	uint32_t id_alt;
	char *parent_acct;
	list_t *tres_list; /* list of slurmdb_tres_rec_t *'s */
	char *user;
} slurmdb_report_assoc_rec_t;

typedef struct {
	char *acct;
	list_t *acct_list; /* list of char *'s */
	list_t *assoc_list; /* list of slurmdb_report_assoc_rec_t's */
	char *name;
	char *partition; /* Optional partition associated with the user */
	list_t *tres_list; /* list of slurmdb_tres_rec_t *'s */
	uid_t uid;
} slurmdb_report_user_rec_t;

typedef struct {
	list_t *accounting_list; /* list of slurmdb_accounting_rec_t *'s */
	list_t *assoc_list; /* list of slurmdb_report_assoc_rec_t *'s */
	char *name;
	list_t *tres_list; /* list of slurmdb_tres_rec_t *'s */
	list_t *user_list; /* list of slurmdb_report_user_rec_t *'s */
} slurmdb_report_cluster_rec_t;

typedef struct {
	uint32_t count; /* count of jobs */
	list_t *jobs; /* This should be a NULL destroy since we are just
		       * putting a pointer to a slurmdb_job_rec_t here
		       * not allocating any new memory */
	uint32_t min_size; /* smallest size of job in cpus here 0 if first */
	uint32_t max_size; /* largest size of job in cpus here INFINITE if
			    * last */
	list_t *tres_list; /* list of slurmdb_tres_rec_t *'s */
} slurmdb_report_job_grouping_t;

typedef struct {
	char *acct;	/* account name */
	uint32_t count; /* total count of jobs taken up by this acct */
	list_t *groups;	/* containing slurmdb_report_job_grouping_t's */
	char *lineage;	/* Complete path up the hierarchy to the root
			 * association */
	list_t *tres_list; /* list of slurmdb_tres_rec_t *'s */
} slurmdb_report_acct_grouping_t;

typedef struct {
	list_t *acct_list; /* containing slurmdb_report_acct_grouping_t's */
	char *cluster; 	/* cluster name */
	uint32_t count;	/* total count of jobs taken up by this cluster */
	list_t *tres_list;	/* list of slurmdb_tres_rec_t *'s */
} slurmdb_report_cluster_grouping_t;

enum {
	DBD_ROLLUP_HOUR,
	DBD_ROLLUP_DAY,
	DBD_ROLLUP_MONTH,
	DBD_ROLLUP_COUNT
};

typedef struct {
	char *cluster_name;                      /* Cluster name */
	uint16_t count[DBD_ROLLUP_COUNT]; /* How many rollups have
					   * happened in time period */
	time_t timestamp[DBD_ROLLUP_COUNT]; /* Timestamps of last rollup. */
	uint64_t time_last[DBD_ROLLUP_COUNT]; /* Last rollup time */
	uint64_t time_max[DBD_ROLLUP_COUNT]; /* What was the longest time
						     * for each rollup */
	uint64_t time_total[DBD_ROLLUP_COUNT]; /* Time it took to do each
						 * rollup */
} slurmdb_rollup_stats_t;

typedef struct {
	uint32_t cnt;	   /* count of object processed */
	uint32_t id;	   /* ID of object */
	uint64_t time;	   /* total usecs this object */
	uint64_t time_ave; /* ave usecs this object (DON'T PACK) */
} slurmdb_rpc_obj_t;

typedef struct {
	slurmdb_rollup_stats_t *dbd_rollup_stats;
	list_t *rollup_stats;              /* List of Clusters rollup stats */
	list_t *rpc_list;                  /* list of RPCs sent to the dbd. */
	time_t time_start;              /* When we started collecting data */
	list_t *user_list;                 /* list of users issuing RPCs */
} slurmdb_stats_rec_t;


/* global variable for cross cluster communication */
extern slurmdb_cluster_rec_t *working_cluster_rec;


/************** account functions **************/

/*
 * add accounts to accounting system
 * IN:  account_list List of slurmdb_account_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_accounts_add(void *db_conn, list_t *acct_list);

/*
 * add accounts to accounting system
 * IN: slurmdb_add_assoc_cond_t *assoc_cond with cluster (optional) and acct
 *     lists filled in along with any limits in the assoc rec.
 * IN: slurmdb_account_rec_t *
 * RET: Return char * to print out of what was added or NULL and errno set on
 *      error.
 */
extern char *slurmdb_accounts_add_cond(void *db_conn,
				       slurmdb_add_assoc_cond_t *add_assoc,
				       slurmdb_account_rec_t *acct);

/*
 * get info from the storage
 * IN:  slurmdb_account_cond_t *
 * IN:  params void *
 * returns List of slurmdb_account_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_accounts_get(void *db_conn,
				    slurmdb_account_cond_t *acct_cond);

/*
 * modify existing accounts in the accounting system
 * IN:  slurmdb_acct_cond_t *acct_cond
 * IN:  slurmdb_account_rec_t *acct
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_accounts_modify(void *db_conn,
				       slurmdb_account_cond_t *acct_cond,
				       slurmdb_account_rec_t *acct);

/*
 * remove accounts from accounting system
 * IN:  slurmdb_account_cond_t *acct_cond
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_accounts_remove(void *db_conn,
				       slurmdb_account_cond_t *acct_cond);


/************** archive functions **************/

/*
 * expire old info from the storage
 */
extern int slurmdb_archive(void *db_conn, slurmdb_archive_cond_t *arch_cond);

/*
 * expire old info from the storage
 */
extern int slurmdb_archive_load(void *db_conn,
				slurmdb_archive_rec_t *arch_rec);


/************** association functions **************/

/*
 * add associations to accounting system
 * IN:  assoc_list List of slurmdb_assoc_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_associations_add(void *db_conn, list_t *assoc_list);

/*
 * get info from the storage
 * IN:  slurmdb_assoc_cond_t *
 * RET: List of slurmdb_assoc_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_associations_get(void *db_conn,
					slurmdb_assoc_cond_t *assoc_cond);

/*
 * modify existing associations in the accounting system
 * IN:  slurmdb_assoc_cond_t *assoc_cond
 * IN:  slurmdb_assoc_rec_t *assoc
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_associations_modify(void *db_conn,
					   slurmdb_assoc_cond_t *assoc_cond,
					   slurmdb_assoc_rec_t *assoc);

/*
 * remove associations from accounting system
 * IN:  slurmdb_assoc_cond_t *assoc_cond
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_associations_remove(void *db_conn,
					   slurmdb_assoc_cond_t *assoc_cond);

/************** cluster functions **************/

/*
 * add clusters to accounting system
 * IN:  cluster_list List of slurmdb_cluster_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_clusters_add(void *db_conn, list_t *cluster_list);

/*
 * get info from the storage
 * IN:  slurmdb_cluster_cond_t *
 * IN:  params void *
 * returns List of slurmdb_cluster_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_clusters_get(void *db_conn,
				    slurmdb_cluster_cond_t *cluster_cond);

/*
 * modify existing clusters in the accounting system
 * IN:  slurmdb_cluster_cond_t *cluster_cond
 * IN:  slurmdb_cluster_rec_t *cluster
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_clusters_modify(void *db_conn,
				       slurmdb_cluster_cond_t *cluster_cond,
				       slurmdb_cluster_rec_t *cluster);

/*
 * remove clusters from accounting system
 * IN:  slurmdb_cluster_cond_t *cluster_cond
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_clusters_remove(void *db_conn,
				       slurmdb_cluster_cond_t *cluster_cond);

/************** cluster report functions **************/

/* report for clusters of account per user
 * IN: slurmdb_assoc_cond_t *assoc_cond
 * RET: List containing (slurmdb_report_cluster_rec_t *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_report_cluster_account_by_user(void *db_conn,
						      slurmdb_assoc_cond_t *assoc_cond);

/* report for clusters of users per account
 * IN: slurmdb_assoc_cond_t *assoc_cond
 * RET: List containing (slurmdb_report_cluster_rec_t *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_report_cluster_user_by_account(void *db_conn,
						      slurmdb_assoc_cond_t *assoc_cond);

/* report for clusters of wckey per user
 * IN: slurmdb_wckey_cond_t *wckey_cond
 * RET: List containing (slurmdb_report_cluster_rec_t *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_report_cluster_wckey_by_user(void *db_conn,
						    slurmdb_wckey_cond_t *wckey_cond);

/* report for clusters of users per wckey
 * IN: slurmdb_wckey_cond_t *wckey_cond
 * RET: List containing (slurmdb_report_cluster_rec_t *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_report_cluster_user_by_wckey(void *db_conn,
						    slurmdb_wckey_cond_t *wckey_cond);


extern list_t *slurmdb_report_job_sizes_grouped_by_account(
	void *db_conn,
	slurmdb_job_cond_t *job_cond,
	list_t *grouping_list,
	bool flat_view,
	bool acct_as_parent);

extern list_t *slurmdb_report_job_sizes_grouped_by_wckey(void *db_conn,
							 slurmdb_job_cond_t *job_cond,
							 list_t *grouping_list);

extern list_t *slurmdb_report_job_sizes_grouped_by_account_then_wckey(
	void *db_conn,
	slurmdb_job_cond_t *job_cond,
	list_t *grouping_list,
	bool flat_view,
	bool acct_as_parent);


/* report on users with top usage
 * IN: slurmdb_user_cond_t *user_cond
 * IN: group_accounts - Whether or not to group all accounts together
 *                      for each  user. If 0 a separate entry for each
 *                      user and account reference is displayed.
 * RET: List containing (slurmdb_report_cluster_rec_t *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_report_user_top_usage(void *db_conn,
					     slurmdb_user_cond_t *user_cond,
					     bool group_accounts);

/************** connection functions **************/

/*
 * get a new connection to the slurmdb
 * OUT: persist_conn_flags - Flags returned from connection if any see
 *                           slurm_persist_conn.h.
 * RET: pointer used to access db
 */
extern void *slurmdb_connection_get(uint16_t *persist_conn_flags);

/*
 * release connection to the storage unit
 * IN/OUT: void ** pointer returned from
 *         slurmdb_connection_get() which will be freed.
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_connection_close(void **db_conn);

/*
 * commit or rollback changes made without closing connection
 * IN: void * pointer returned from slurmdb_connection_get()
 * IN: bool - true will commit changes false will rollback
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_connection_commit(void *db_conn, bool commit);

/************** coordinator functions **************/

/*
 * add users as account coordinators
 * IN: acct_list list of char *'s of names of accounts
 * IN:  slurmdb_user_cond_t *user_cond
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_coord_add(void *db_conn,
			     list_t *acct_list,
			     slurmdb_user_cond_t *user_cond);

/*
 * remove users from being a coordinator of an account
 * IN: acct_list list of char *'s of names of accounts
 * IN: slurmdb_user_cond_t *user_cond
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_coord_remove(void *db_conn, list_t *acct_list,
				    slurmdb_user_cond_t *user_cond);

/*************** Federation functions **************/

/*
 * add federations to accounting system
 * IN:  list List of slurmdb_federation_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_federations_add(void *db_conn, list_t *federation_list);

/*
 * modify existing federations in the accounting system
 * IN:  slurmdb_federation_cond_t *fed_cond
 * IN:  slurmdb_federation_rec_t  *fed
 * RET: List containing (char *'s) else NULL on error
 */
extern list_t *slurmdb_federations_modify(void *db_conn,
					  slurmdb_federation_cond_t *fed_cond,
					  slurmdb_federation_rec_t *fed);

/*
 * remove federations from accounting system
 * IN:  slurmdb_federation_cond_t *fed_cond
 * RET: List containing (char *'s) else NULL on error
 */
extern list_t *slurmdb_federations_remove(void *db_conn,
					  slurmdb_federation_cond_t *fed_cond);

/*
 * get info from the storage
 * IN:  slurmdb_federation_cond_t *
 * RET: List of slurmdb_federation_rec_t *
 * note List needs to be freed when called
 */
extern list_t *slurmdb_federations_get(void *db_conn,
				       slurmdb_federation_cond_t *fed_cond);

/*************** Job functions **************/

/*
 * modify existing job in the accounting system
 * IN:  slurmdb_job_cond_t *job_cond
 * IN:  slurmdb_job_rec_t *job
 * RET: List containing (char *'s) else NULL on error
 */
extern list_t *slurmdb_job_modify(void *db_conn,
				  slurmdb_job_cond_t *job_cond,
				  slurmdb_job_rec_t *job);

/*
 * get info from the storage
 * returns List of slurmdb_job_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_jobs_get(void *db_conn, slurmdb_job_cond_t *job_cond);

/*
 * Fix runaway jobs
 * IN: jobs, a list of all the runaway jobs
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_jobs_fix_runaway(void *db_conn, list_t *jobs);

/* initialization of job completion logging */
extern int slurmdb_jobcomp_init(void);

/* terminate pthreads and free, general clean-up for termination */
extern int slurmdb_jobcomp_fini(void);

/*
 * get info from the storage
 * returns List of jobcomp_job_rec_t *
 * note List needs to be freed when called
 */
extern list_t *slurmdb_jobcomp_jobs_get(slurmdb_job_cond_t *job_cond);

/************** extra get functions **************/

/*
 * reconfigure the slurmdbd
 */
extern int slurmdb_reconfig(void *db_conn);

/*
 * shutdown the slurmdbd
 */
extern int slurmdb_shutdown(void *db_conn);

/*
 * clear the slurmdbd statistics
 */
extern int slurmdb_clear_stats(void *db_conn);

/*
 * get the slurmdbd statistics
 * Call slurmdb_destroy_stats_rec() to free stats_pptr
 */
extern int slurmdb_get_stats(void *db_conn, slurmdb_stats_rec_t **stats_pptr);

/*
 * get info from the storage
 * RET: List of config_key_pair_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_config_get(void *db_conn);

/*
 * get info from the storage
 * IN:  slurmdb_event_cond_t *
 * RET: List of slurmdb_event_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_events_get(void *db_conn,
				  slurmdb_event_cond_t *event_cond);

/*
 * get info from the storage
 *
 * IN:  slurmdb_instance_cond_t *
 * RET: List of slurmdb_instance_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_instances_get(void *db_conn,
				     slurmdb_instance_cond_t *instance_cond);

/*
 * get info from the storage
 * IN:  slurmdb_assoc_cond_t *
 * RET: List of slurmdb_assoc_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_problems_get(void *db_conn,
				    slurmdb_assoc_cond_t *assoc_cond);

/*
 * get info from the storage
 * IN:  slurmdb_reservation_cond_t *
 * RET: List of slurmdb_reservation_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_reservations_get(void *db_conn,
					slurmdb_reservation_cond_t *resv_cond);

/*
 * get info from the storage
 * IN:  slurmdb_txn_cond_t *
 * RET: List of slurmdb_txn_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_txn_get(void *db_conn, slurmdb_txn_cond_t *txn_cond);

/*
 * Get information about requested cluster(s). Similar to
 * slurmdb_clusters_get, but should be used when setting up the
 * working_cluster_rec.  It sets up the control_addr and dim_size parts of the
 * structure.
 *
 * IN: cluster_names - comma separated string of cluster names
 * RET: List of slurmdb_cluster_rec_t *
 * note List needs to bbe freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_get_info_cluster(char *cluster_names);

/*
 * get the first cluster that will run a job
 * IN: req - description of resource allocation request
 * IN: cluster_names - comma separated string of cluster names
 * OUT: cluster_rec - record of selected cluster or NULL if none found or
 * 		      cluster_names is NULL
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 *
 * Note: Cluster_rec needs to be freed with slurmdb_destroy_cluster_rec() when
 * called
 * Note: The will_runs are not threaded. Currently it relies on the
 * working_cluster_rec to pack the job_desc's jobinfo. See previous commit for
 * an example of how to thread this.
 */
extern int slurmdb_get_first_avail_cluster(job_desc_msg_t *req,
					   char *cluster_names,
					   slurmdb_cluster_rec_t **cluster_rec);

/*
 * get the first cluster that will run a heterogeneous job
 * IN: req - description of resource allocation request
 * IN: cluster_names - comma separated string of cluster names
 * OUT: cluster_rec - record of selected cluster or NULL if none found or
 * 		      cluster_names is NULL
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 *
 * Note: Cluster_rec needs to be freed with slurmdb_destroy_cluster_rec() when
 * called
 * Note: The will_runs are not threaded. Currently it relies on the
 * working_cluster_rec to pack the job_desc's jobinfo. See previous commit for
 * an example of how to thread this.
 */
extern int slurmdb_get_first_het_job_cluster(
	list_t *job_req_list, char *cluster_names,
	slurmdb_cluster_rec_t **cluster_rec);

/************** helper functions **************/
extern void slurmdb_destroy_assoc_usage(void *object);
extern void slurmdb_destroy_bf_usage(void *object);
extern void slurmdb_destroy_bf_usage_members(void *object);
extern void slurmdb_destroy_qos_usage(void *object);
extern void slurmdb_free_user_rec_members(slurmdb_user_rec_t *slurmdb_user);
extern void slurmdb_destroy_user_rec(void *object);
extern void slurmdb_destroy_account_rec(void *object);
extern void slurmdb_destroy_coord_rec(void *object);
extern void slurmdb_destroy_clus_res_rec(void *object);
extern void slurmdb_destroy_cluster_accounting_rec(void *object);
extern void slurmdb_destroy_cluster_rec(void *object);
extern void slurmdb_destroy_federation_rec(void *object);
extern void slurmdb_destroy_accounting_rec(void *object);
extern void slurmdb_free_assoc_mgr_state_msg(void *object);
extern void slurmdb_free_assoc_rec_members(slurmdb_assoc_rec_t *assoc);
extern void slurmdb_destroy_assoc_rec(void *object);
extern void slurmdb_destroy_event_rec(void *object);
extern void slurmdb_destroy_instance_rec(void *object);
extern void slurmdb_destroy_job_rec(void *object);
extern void slurmdb_free_qos_rec_members(slurmdb_qos_rec_t *qos);
extern void slurmdb_destroy_qos_rec(void *object);
extern void slurmdb_destroy_reservation_rec(void *object);
extern void slurmdb_destroy_step_rec(void *object);
extern void slurmdb_destroy_res_rec(void *object);
extern void slurmdb_destroy_txn_rec(void *object);
extern void slurmdb_destroy_wckey_rec(void *object);
extern void slurmdb_destroy_archive_rec(void *object);
extern void slurmdb_destroy_tres_rec_noalloc(void *object);
extern void slurmdb_destroy_tres_rec(void *object);
extern void slurmdb_destroy_report_assoc_rec(void *object);
extern void slurmdb_destroy_report_user_rec(void *object);
extern void slurmdb_destroy_report_cluster_rec(void *object);

extern void slurmdb_destroy_user_cond(void *object);
extern void slurmdb_destroy_account_cond(void *object);
extern void slurmdb_destroy_cluster_cond(void *object);
extern void slurmdb_destroy_federation_cond(void *object);
extern void slurmdb_destroy_tres_cond(void *object);
extern void slurmdb_destroy_assoc_cond(void *object);
extern void slurmdb_destroy_event_cond(void *object);
extern void slurmdb_destroy_instance_cond(void *object);
extern void slurmdb_destroy_job_cond(void *object);
extern void slurmdb_destroy_job_cond_members(slurmdb_job_cond_t *job_cond);
extern void slurmdb_destroy_qos_cond(void *object);
extern void slurmdb_destroy_reservation_cond(void *object);
extern void slurmdb_destroy_res_cond(void *object);
extern void slurmdb_destroy_txn_cond(void *object);
extern void slurmdb_destroy_wckey_cond(void *object);
extern void slurmdb_destroy_archive_cond(void *object);
extern void slurmdb_free_add_assoc_cond_members(
	slurmdb_add_assoc_cond_t *add_assoc);
extern void slurmdb_destroy_add_assoc_cond(void *object);

extern void slurmdb_destroy_update_object(void *object);
extern void slurmdb_destroy_used_limits(void *object);
extern void slurmdb_destroy_print_tree(void *object);
extern void slurmdb_destroy_hierarchical_rec(void *object);

extern void slurmdb_destroy_report_job_grouping(void *object);
extern void slurmdb_destroy_report_acct_grouping(void *object);
extern void slurmdb_destroy_report_cluster_grouping(void *object);
extern void slurmdb_destroy_rpc_obj(void *object);
extern void slurmdb_destroy_rollup_stats(void *object);
extern void slurmdb_free_stats_rec_members(void *object);
extern void slurmdb_destroy_stats_rec(void *object);

extern void slurmdb_free_slurmdb_stats_members(slurmdb_stats_t *stats);
extern void slurmdb_destroy_slurmdb_stats(slurmdb_stats_t *stats);

extern void slurmdb_init_assoc_rec(slurmdb_assoc_rec_t *assoc,
				   bool free_it);
extern void slurmdb_init_clus_res_rec(slurmdb_clus_res_rec_t *clus_res,
				      bool free_it);
extern void slurmdb_init_cluster_rec(slurmdb_cluster_rec_t *cluster,
				     bool free_it);
extern void slurmdb_init_federation_rec(slurmdb_federation_rec_t *federation,
					bool free_it);
extern void slurmdb_init_instance_rec(slurmdb_instance_rec_t *instance);
extern void slurmdb_init_qos_rec(slurmdb_qos_rec_t *qos,
				 bool free_it,
				 uint32_t init_val);
extern void slurmdb_init_res_rec(slurmdb_res_rec_t *res,
				 bool free_it);
extern void slurmdb_init_wckey_rec(slurmdb_wckey_rec_t *wckey,
				   bool free_it);
extern void slurmdb_init_add_assoc_cond(slurmdb_add_assoc_cond_t *add_assoc,
					bool free_it);
extern void slurmdb_init_tres_cond(slurmdb_tres_cond_t *tres,
				   bool free_it);
extern void slurmdb_init_cluster_cond(slurmdb_cluster_cond_t *cluster,
				      bool free_it);
extern void slurmdb_init_federation_cond(slurmdb_federation_cond_t *federation,
					 bool free_it);
extern void slurmdb_init_res_cond(slurmdb_res_cond_t *cluster,
				  bool free_it);

extern int slurmdb_ping(char *rem_host);
extern slurmdbd_ping_t *slurmdb_ping_all(void);

/* The next two functions have pointers to assoc_list so do not
 * destroy assoc_list before using the list returned from this function.
 */
extern list_t *slurmdb_get_hierarchical_sorted_assoc_list(list_t *assoc_list);
extern list_t *slurmdb_get_acct_hierarchical_rec_list(list_t *assoc_list);


/* IN/OUT: tree_list a list of slurmdb_print_tree_t's */
extern char *slurmdb_tree_name_get(char *name, char *parent, list_t *tree_list);

/************** job report functions **************/

/************** resource functions **************/
/*
 * add resource's to accounting system
 * IN:  res_list List of char *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_res_add(void *db_conn, list_t *res_list);

/*
 * get info from the storage
 * IN:  slurmdb_res_cond_t *
 * RET: List of slurmdb_res_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_res_get(void *db_conn, slurmdb_res_cond_t *res_cond);

/*
 * modify existing resource in the accounting system
 * IN:  slurmdb_res_cond_t *res_cond
 * IN:  slurmdb_res_rec_t *res
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_res_modify(void *db_conn,
				  slurmdb_res_cond_t *res_cond,
				  slurmdb_res_rec_t *res);

/*
 * remove resource from accounting system
 * IN:  slurmdb_res_cond_t *res
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_res_remove(void *db_conn, slurmdb_res_cond_t *res_cond);

/************** qos functions **************/

/*
 * add qos's to accounting system
 * IN:  qos_list List of char *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_qos_add(void *db_conn, list_t *qos_list);

/*
 * get info from the storage
 * IN:  slurmdb_qos_cond_t *
 * RET: List of slurmdb_qos_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_qos_get(void *db_conn, slurmdb_qos_cond_t *qos_cond);

/*
 * modify existing qos in the accounting system
 * IN:  slurmdb_qos_cond_t *qos_cond
 * IN:  slurmdb_qos_rec_t *qos
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_qos_modify(void *db_conn,
				  slurmdb_qos_cond_t *qos_cond,
				  slurmdb_qos_rec_t *qos);

/*
 * remove qos from accounting system
 * IN:  slurmdb_qos_cond_t *assoc_qos
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_qos_remove(void *db_conn, slurmdb_qos_cond_t *qos_cond);

/************** tres functions **************/

/*
 * add tres's to accounting system
 * IN:  tres_list List of char *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_tres_add(void *db_conn, list_t *tres_list);

/*
 * get info from the storage
 * IN:  slurmdb_tres_cond_t *
 * RET: List of slurmdb_tres_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_tres_get(void *db_conn, slurmdb_tres_cond_t *tres_cond);


/************** usage functions **************/

/*
 * get info from the storage
 * IN/OUT:  in void * (slurmdb_assoc_rec_t *) or
 *          (slurmdb_wckey_rec_t *) of (slurmdb_cluster_rec_t *) with
 *          the id, and cluster set.
 * IN:  type what type is 'in'
 * IN:  start time stamp for records >=
 * IN:  end time stamp for records <=
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_usage_get(void *db_conn,
			     void *in,
			     int type,
			     time_t start,
			     time_t end);

/*
 * roll up data in the storage
 * IN: sent_start (option time to do a re-roll or start from this point)
 * IN: sent_end (option time to do a re-roll or end at this point)
 * IN: archive_data (if 0 old data is not archived in a monthly rollup)
 * OUT: rollup_stats_list_in (list containing stats about each clusters rollup)
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_usage_roll(void *db_conn,
			      time_t sent_start,
			      time_t sent_end,
			      uint16_t archive_data,
			      list_t **rollup_stats_list_in);

/************** user functions **************/

/*
 * add users to accounting system
 * IN:  user_list List of slurmdb_user_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_users_add(void *db_conn, list_t *user_list);

/*
 * add users to accounting system
 * IN: slurmdb_add_assoc_cond_t *assoc_cond with cluster (optional) acct
 *     and user lists filled in along with any limits in the assoc rec.
 * IN: slurmdb_user_rec_t *
 * RET: Return char * to print out of what was added or NULL and errno set on
 *      error.
 */
extern char *slurmdb_users_add_cond(void *db_conn,
				    slurmdb_add_assoc_cond_t *add_assoc,
				    slurmdb_user_rec_t *user);

/*
 * add users to accounting system
 * IN:  slurmdb_user_rec_t *user
 * IN:  slurmdb_assoc_cond_t *assoc_cond
 * IN:  slurmdb_assoc_rec_t *assoc
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern list_t *slurmdb_users_add_conn(void *db_conn,
				      slurmdb_user_rec_t *user,
				      slurmdb_assoc_cond_t *assoc_cond,
				   slurmdb_assoc_rec_t *assoc);

/*
 * get info from the storage
 * IN:  slurmdb_user_cond_t *
 * IN:  params void *
 * returns List of slurmdb_user_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_users_get(void *db_conn, slurmdb_user_cond_t *user_cond);

/*
 * modify existing users in the accounting system
 * IN:  slurmdb_user_cond_t *user_cond
 * IN:  slurmdb_user_rec_t *user
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_users_modify(void *db_conn,
				    slurmdb_user_cond_t *user_cond,
				    slurmdb_user_rec_t *user);

/*
 * remove users from accounting system
 * IN:  slurmdb_user_cond_t *user_cond
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_users_remove(void *db_conn,
				    slurmdb_user_cond_t *user_cond);


/************** user report functions **************/


/************** wckey functions **************/

/*
 * add wckey's to accounting system
 * IN:  wckey_list List of slurmdb_wckey_rec_t *
 * RET: SLURM_SUCCESS on success SLURM_ERROR else
 */
extern int slurmdb_wckeys_add(void *db_conn, list_t *wckey_list);

/*
 * get info from the storage
 * IN:  slurmdb_wckey_cond_t *
 * RET: List of slurmdb_wckey_rec_t *
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_wckeys_get(void *db_conn,
				  slurmdb_wckey_cond_t *wckey_cond);

/*
 * modify existing wckey in the accounting system
 * IN:  slurmdb_wckey_cond_t *wckey_cond
 * IN:  slurmdb_wckey_rec_t *wckey
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_wckeys_modify(void *db_conn,
				     slurmdb_wckey_cond_t *wckey_cond,
				     slurmdb_wckey_rec_t *wckey);

/*
 * remove wckey from accounting system
 * IN:  slurmdb_wckey_cond_t *assoc_wckey
 * RET: List containing (char *'s) else NULL on error
 * note List needs to be freed with slurm_list_destroy() when called
 */
extern list_t *slurmdb_wckeys_remove(void *db_conn,
				     slurmdb_wckey_cond_t *wckey_cond);

/*
 * Take a path and return a string with all the wildcards in the path properly
 * expanded, based in the values of the passed slurmdb_job_rec_t. This function
 * is mainly used for batch steps, and the generated path refers to the job
 * output files.
 * IN: path - The raw path string which may contain wildcards to expand.
 * IN: job - A slurmdb_job_rec_t struct used to expand the wildcards.
 * RET: char * - An allocated string with all the paths expanded. The caller
 * must xfree it.
 */
extern char *slurmdb_expand_job_stdio_fields(char *path,
					     slurmdb_job_rec_t *job);

/*
 * Take a path and return a string with all the wildcards in the path properly
 * expanded, based in the values of the passed slurmdb_step_rec_t. This function
 * is mainly used for job steps, and the generated path refers to the step
 * output files.
 * IN: path - The raw path string which may contain wildcards to expand.
 * IN: step - A slurmdb_step_rec_t struct used to expand the wildcards.
 * RET: char * - An allocated string with all the paths expanded. The caller
 * must xfree it.
 */
extern char *slurmdb_expand_step_stdio_fields(char *path,
					      slurmdb_step_rec_t *step);

#ifdef __cplusplus
}
#endif

#endif /* !_SLURMDB_H */

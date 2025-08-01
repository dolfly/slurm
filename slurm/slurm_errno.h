/*****************************************************************************\
 *  slurm_errno.h - error codes and functions for slurm
 ******************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Kevin Tew <tew1@llnl.gov>,
 *	Jim Garlick <garlick@llnl.gov>, et. al.
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
#ifndef _SLURM_ERRNO_H
#define _SLURM_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

/* set errno to the specified value - then return -1 */
#define slurm_seterrno_ret(errnum) do {	\
	errno = errnum;			\
	return (errnum ? -1 : 0);	\
} while (0)

/* general return codes */
#define SLURM_SUCCESS   0
#define ESPANK_SUCCESS 0
#define SLURM_ERROR    -1

typedef enum {
	/* General Message error codes */
	SLURM_UNEXPECTED_MSG_ERROR = 			1000,
	SLURM_COMMUNICATIONS_CONNECTION_ERROR,
	SLURM_COMMUNICATIONS_SEND_ERROR,
	SLURM_COMMUNICATIONS_RECEIVE_ERROR,
	SLURM_COMMUNICATIONS_SHUTDOWN_ERROR,
	SLURM_PROTOCOL_VERSION_ERROR,
	SLURM_PROTOCOL_IO_STREAM_VERSION_ERROR,
	SLURM_PROTOCOL_AUTHENTICATION_ERROR,
	SLURM_PROTOCOL_INSANE_MSG_LENGTH,
	SLURM_MPI_PLUGIN_NAME_INVALID,
	SLURM_MPI_PLUGIN_PRELAUNCH_SETUP_FAILED,
	SLURM_PLUGIN_NAME_INVALID,
	SLURM_UNKNOWN_FORWARD_ADDR,
	SLURM_COMMUNICATIONS_MISSING_SOCKET_ERROR,
	SLURM_COMMUNICATIONS_INVALID_INCOMING_FD,
	SLURM_COMMUNICATIONS_INVALID_OUTGOING_FD,
	SLURM_COMMUNICATIONS_INVALID_FD,

	/* communication failures to/from slurmctld */
	SLURMCTLD_COMMUNICATIONS_CONNECTION_ERROR =     1800,
	SLURMCTLD_COMMUNICATIONS_SEND_ERROR,
	SLURMCTLD_COMMUNICATIONS_RECEIVE_ERROR,
	SLURMCTLD_COMMUNICATIONS_SHUTDOWN_ERROR,
	SLURMCTLD_COMMUNICATIONS_BACKOFF,
	SLURMCTLD_COMMUNICATIONS_HARD_DROP,

	/* _info.c/communication layer RESPONSE_SLURM_RC message codes */
	SLURM_NO_CHANGE_IN_DATA =			1900,

	/* slurmctld error codes */
	ESLURM_INVALID_PARTITION_NAME = 		2000,
	ESLURM_DEFAULT_PARTITION_NOT_SET,
	ESLURM_ACCESS_DENIED,
	ESLURM_JOB_MISSING_REQUIRED_PARTITION_GROUP,
	ESLURM_REQUESTED_NODES_NOT_IN_PARTITION,
	ESLURM_TOO_MANY_REQUESTED_CPUS,
	ESLURM_INVALID_NODE_COUNT,
	ESLURM_ERROR_ON_DESC_TO_RECORD_COPY,
	ESLURM_JOB_MISSING_SIZE_SPECIFICATION,
	ESLURM_JOB_SCRIPT_MISSING,
	ESLURM_USER_ID_MISSING =			2010,
	ESLURM_DUPLICATE_JOB_ID,
	ESLURM_PATHNAME_TOO_LONG,
	ESLURM_NOT_TOP_PRIORITY,
	ESLURM_REQUESTED_NODE_CONFIG_UNAVAILABLE,
	ESLURM_REQUESTED_PART_CONFIG_UNAVAILABLE,
	ESLURM_NODES_BUSY,
	ESLURM_INVALID_JOB_ID,
	ESLURM_INVALID_NODE_NAME,
	ESLURM_WRITING_TO_FILE,
	ESLURM_TRANSITION_STATE_NO_UPDATE =		2020,
	ESLURM_ALREADY_DONE,
	ESLURM_INTERCONNECT_FAILURE,
	ESLURM_BAD_DIST,
	ESLURM_JOB_PENDING,
	ESLURM_BAD_TASK_COUNT,
	ESLURM_INVALID_JOB_CREDENTIAL,
	ESLURM_IN_STANDBY_MODE,
	ESLURM_INVALID_NODE_STATE,
	ESLURM_INVALID_FEATURE,
	ESLURM_INVALID_AUTHTYPE_CHANGE =		2030,
	ESLURM_ACTIVE_FEATURE_NOT_SUBSET,
	ESLURM_INVALID_SCHEDTYPE_CHANGE,
	ESLURM_INVALID_SELECTTYPE_CHANGE,
	ESLURM_INVALID_SWITCHTYPE_CHANGE,
	ESLURM_FRAGMENTATION,
	ESLURM_NOT_SUPPORTED,
	ESLURM_DISABLED,
	ESLURM_DEPENDENCY,
	ESLURM_BATCH_ONLY,
	ESLURM_LICENSES_UNAVAILABLE =			2040,
	ESLURM_TAKEOVER_NO_HEARTBEAT,
	ESLURM_JOB_HELD,
	ESLURM_INVALID_CRED_TYPE_CHANGE,
	ESLURM_INVALID_TASK_MEMORY,
	ESLURM_INVALID_ACCOUNT,
	ESLURM_INVALID_PARENT_ACCOUNT,
	ESLURM_SAME_PARENT_ACCOUNT,
	ESLURM_INVALID_LICENSES,
	ESLURM_NEED_RESTART,
	ESLURM_ACCOUNTING_POLICY =			2050,
	ESLURM_INVALID_TIME_LIMIT,
	ESLURM_RESERVATION_ACCESS,
	ESLURM_RESERVATION_INVALID,
	ESLURM_INVALID_TIME_VALUE,
	ESLURM_RESERVATION_BUSY,
	ESLURM_RESERVATION_NOT_USABLE,
	ESLURM_INVALID_WCKEY,
	ESLURM_RESERVATION_OVERLAP,
	ESLURM_PORTS_BUSY,
	ESLURM_PORTS_INVALID =				2060,
	ESLURM_PROLOG_RUNNING,
	ESLURM_NO_STEPS,
	ESLURM_MISSING_WORK_DIR,

	ESLURM_INVALID_QOS =				2066,
	ESLURM_QOS_PREEMPTION_LOOP,
	ESLURM_NODE_NOT_AVAIL,
	ESLURM_INVALID_CPU_COUNT,
	ESLURM_PARTITION_NOT_AVAIL =			2070,
	ESLURM_CIRCULAR_DEPENDENCY,
	ESLURM_INVALID_GRES,
	ESLURM_JOB_NOT_PENDING,
	ESLURM_QOS_THRES,
	ESLURM_PARTITION_IN_USE,
	ESLURM_STEP_LIMIT,
	ESLURM_JOB_SUSPENDED,
	ESLURM_CAN_NOT_START_IMMEDIATELY,
	ESLURM_INTERCONNECT_BUSY,
	ESLURM_RESERVATION_EMPTY =			2080,
	ESLURM_INVALID_ARRAY,
	ESLURM_RESERVATION_NAME_DUP,
	ESLURM_JOB_STARTED,
	ESLURM_JOB_FINISHED,
	ESLURM_JOB_NOT_RUNNING,
	ESLURM_JOB_NOT_PENDING_NOR_RUNNING,
	ESLURM_JOB_NOT_SUSPENDED,
	ESLURM_JOB_NOT_FINISHED,
	ESLURM_TRIGGER_DUP,
	ESLURM_INTERNAL =				2090,
	ESLURM_INVALID_BURST_BUFFER_CHANGE,
	ESLURM_BURST_BUFFER_PERMISSION,
	ESLURM_BURST_BUFFER_LIMIT,
	ESLURM_INVALID_BURST_BUFFER_REQUEST,
	ESLURM_PRIO_RESET_FAIL,
	ESLURM_CANNOT_MODIFY_CRON_JOB,
	ESLURM_INVALID_JOB_CONTAINER_CHANGE,
	ESLURM_CANNOT_CANCEL_CRON_JOB,

	ESLURM_INVALID_MCS_LABEL =			2099,
	ESLURM_BURST_BUFFER_WAIT =			2100,
	ESLURM_PARTITION_DOWN,
	ESLURM_DUPLICATE_GRES,

	ESLURM_RSV_ALREADY_STARTED =			2104,
	ESLURM_SUBMISSIONS_DISABLED,
	ESLURM_NOT_HET_JOB,
	ESLURM_NOT_HET_JOB_LEADER,
	ESLURM_NOT_WHOLE_HET_JOB,
	ESLURM_CORE_RESERVATION_UPDATE,
	ESLURM_DUPLICATE_STEP_ID =			2110,
	ESLURM_INVALID_CORE_CNT,
	ESLURM_X11_NOT_AVAIL,
	ESLURM_GROUP_ID_MISSING,
	ESLURM_BATCH_CONSTRAINT,
	ESLURM_INVALID_TRES,
	ESLURM_INVALID_TRES_BILLING_WEIGHTS,
	ESLURM_INVALID_JOB_DEFAULTS,
	ESLURM_RESERVATION_MAINT,
	ESLURM_INVALID_GRES_TYPE,
	ESLURM_REBOOT_IN_PROGRESS =			2120,
	ESLURM_MULTI_KNL_CONSTRAINT,
	ESLURM_UNSUPPORTED_GRES,
	ESLURM_INVALID_NICE,
	ESLURM_INVALID_TIME_MIN_LIMIT,
	ESLURM_DEFER,
	ESLURM_CONFIGLESS_DISABLED,
	ESLURM_ENVIRONMENT_MISSING,
	ESLURM_RESERVATION_NO_SKIP,
	ESLURM_RESERVATION_USER_GROUP,
	ESLURM_PARTITION_ASSOC =			2130,
	ESLURM_IN_STANDBY_USE_BACKUP,
	ESLURM_BAD_THREAD_PER_CORE,
	ESLURM_INVALID_PREFER,
	ESLURM_INSUFFICIENT_GRES,
	ESLURM_INVALID_CONTAINER_ID,
	ESLURM_EMPTY_JOB_ID,
	ESLURM_INVALID_JOB_ID_ZERO,
	ESLURM_INVALID_JOB_ID_NEGATIVE,
	ESLURM_INVALID_JOB_ID_TOO_LARGE,
	ESLURM_INVALID_JOB_ID_NON_NUMERIC,
	ESLURM_EMPTY_JOB_ARRAY_ID,
	ESLURM_INVALID_JOB_ARRAY_ID_NEGATIVE,
	ESLURM_INVALID_JOB_ARRAY_ID_TOO_LARGE,
	ESLURM_INVALID_JOB_ARRAY_ID_NON_NUMERIC,
	ESLURM_INVALID_HET_JOB_AND_ARRAY,
	ESLURM_EMPTY_HET_JOB_COMP,
	ESLURM_INVALID_HET_JOB_COMP_NEGATIVE,
	ESLURM_INVALID_HET_JOB_COMP_TOO_LARGE,
	ESLURM_INVALID_HET_JOB_COMP_NON_NUMERIC,
	ESLURM_EMPTY_STEP_ID,
	ESLURM_INVALID_STEP_ID_NEGATIVE,
	ESLURM_INVALID_STEP_ID_TOO_LARGE,
	ESLURM_INVALID_STEP_ID_NON_NUMERIC,
	ESLURM_EMPTY_HET_STEP,
	ESLURM_INVALID_HET_STEP_ZERO,
	ESLURM_INVALID_HET_STEP_NEGATIVE,
	ESLURM_INVALID_HET_STEP_TOO_LARGE,
	ESLURM_INVALID_HET_STEP_NON_NUMERIC,
	ESLURM_INVALID_HET_STEP_JOB,
	ESLURM_JOB_TIMEOUT_KILLED,
	ESLURM_JOB_NODE_FAIL_KILLED,
	ESLURM_EMPTY_LIST,
	ESLURM_GROUP_ID_INVALID,
	ESLURM_GROUP_ID_UNKNOWN,
	ESLURM_USER_ID_INVALID,
	ESLURM_USER_ID_UNKNOWN,
	ESLURM_INVALID_ASSOC,
	ESLURM_NODE_ALREADY_EXISTS,
	ESLURM_NODE_TABLE_FULL,
	ESLURM_INVALID_RELATIVE_QOS,
	ESLURM_INVALID_EXTRA,
	ESLURM_JOB_SIGNAL_FAILED,
	ESLURM_SIGNAL_JOBS_INVALID,
	ESLURM_RES_CORES_PER_GPU_UNIQUE,
	ESLURM_RES_CORES_PER_GPU_TOPO,
	ESLURM_RES_CORES_PER_GPU_NO,
	ESLURM_MAX_POWERED_NODES,
	ESLURM_REQUESTED_TOPO_CONFIG_UNAVAILABLE,
	ESLURM_PREEMPTION_REQUIRED,
	ESLURM_INVALID_NODE_STATE_TRANSITION,

	/* SPANK errors */
	ESPANK_ERROR = 					3000,
	ESPANK_BAD_ARG,
	ESPANK_NOT_TASK,
	ESPANK_ENV_EXISTS,
	ESPANK_ENV_NOEXIST,
	ESPANK_NOSPACE,
	ESPANK_NOT_REMOTE,
	ESPANK_NOEXIST,
	ESPANK_NOT_EXECD,
	ESPANK_NOT_AVAIL,
	ESPANK_NOT_LOCAL,
	ESPANK_NODE_FAILURE,
	ESPANK_JOB_FAILURE,

	/* slurmd error codes */
	ESLURMD_KILL_TASK_FAILED =			4001,
	ESLURMD_KILL_JOB_ALREADY_COMPLETE,
	ESLURMD_INVALID_ACCT_FREQ,
	ESLURMD_INVALID_JOB_CREDENTIAL,

	ESLURMD_CREDENTIAL_EXPIRED =			4007,
	ESLURMD_CREDENTIAL_REVOKED,
	ESLURMD_CREDENTIAL_REPLAYED,
	ESLURMD_CREATE_BATCH_DIR_ERROR =		4010,
	ESLURMD_SETUP_ENVIRONMENT_ERROR =		4014,
	ESLURMD_SET_UID_OR_GID_ERROR =			4016,
	ESLURMD_EXECVE_FAILED =				4020,
	ESLURMD_IO_ERROR,
	ESLURMD_PROLOG_FAILED,
	ESLURMD_EPILOG_FAILED,

	ESLURMD_TOOMANYSTEPS =				4025,
	ESLURMD_STEP_EXISTS,
	ESLURMD_STEP_NOTRUNNING,
	ESLURMD_STEP_SUSPENDED,
	ESLURMD_STEP_NOTSUSPENDED,
	ESLURMD_INVALID_SOCKET_NAME_LEN =		4030,
	ESLURMD_CONTAINER_RUNTIME_INVALID,
	ESLURMD_CPU_BIND_ERROR,
	ESLURMD_CPU_LAYOUT_ERROR,
	ESLURMD_TOO_MANY_RPCS,
	ESLURMD_STEPD_PROXY_FAILED,

	/* socket specific Slurm communications error */
	ESLURM_PROTOCOL_INCOMPLETE_PACKET = 5003,
	SLURM_PROTOCOL_SOCKET_IMPL_TIMEOUT ,
	SLURM_PROTOCOL_SOCKET_ZERO_BYTES_SENT,

	/* slurm_auth errors */
	ESLURM_AUTH_CRED_INVALID	= 6000,
	ESLURM_AUTH_EXPIRED,
	ESLURM_AUTH_BADARG		= 6004,
	ESLURM_AUTH_UNPACK		= 6007,
	ESLURM_AUTH_SKIP,
	ESLURM_AUTH_UNABLE_TO_GENERATE_TOKEN,

	/* accounting errors */
	ESLURM_DB_CONNECTION            = 7000,
	ESLURM_JOBS_RUNNING_ON_ASSOC,
	ESLURM_CLUSTER_DELETED,
	ESLURM_ONE_CHANGE,
	ESLURM_BAD_NAME,
	ESLURM_OVER_ALLOCATE,
	ESLURM_RESULT_TOO_LARGE,
	ESLURM_DB_QUERY_TOO_WIDE,
	ESLURM_DB_CONNECTION_INVALID,
	ESLURM_NO_REMOVE_DEFAULT_ACCOUNT,
	ESLURM_BAD_SQL,
	ESLURM_NO_REMOVE_DEFAULT_QOS,
	ESLURM_COORD_NO_INCREASE_JOB_LIMIT,
	ESLURM_NO_RPC_STATS,

	/* Federation Errors */
	ESLURM_FED_CLUSTER_MAX_CNT              = 7100,
	ESLURM_FED_CLUSTER_MULTIPLE_ASSIGNMENT,
	ESLURM_INVALID_CLUSTER_FEATURE,
	ESLURM_JOB_NOT_FEDERATED,
	ESLURM_INVALID_CLUSTER_NAME,
	ESLURM_FED_JOB_LOCK,
	ESLURM_FED_NO_VALID_CLUSTERS,

	/* plugin and custom errors */
	ESLURM_MISSING_TIME_LIMIT       = 8000,
	ESLURM_INVALID_KNL,
	ESLURM_PLUGIN_INVALID,
	ESLURM_PLUGIN_INCOMPLETE,
	ESLURM_PLUGIN_NOT_LOADED,
	ESLURM_PLUGIN_NOTFOUND,
	ESLURM_PLUGIN_ACCESS_ERROR,
	ESLURM_PLUGIN_DLOPEN_FAILED,
	ESLURM_PLUGIN_INIT_FAILED,
	ESLURM_PLUGIN_MISSING_NAME,
	ESLURM_PLUGIN_BAD_VERSION,

	/* REST errors */
	ESLURM_REST_INVALID_QUERY = 9000,
	ESLURM_REST_FAIL_PARSING,
	ESLURM_REST_INVALID_JOBS_DESC,
	ESLURM_REST_EMPTY_RESULT,
	ESLURM_REST_MISSING_UID,
	ESLURM_REST_MISSING_GID,

	/* data_t errors */
	ESLURM_DATA_PATH_NOT_FOUND = 9200,
	ESLURM_DATA_PTR_NULL,
	ESLURM_DATA_CONV_FAILED,
	ESLURM_DATA_REGEX_COMPILE,
	ESLURM_DATA_UNKNOWN_MIME_TYPE,
	ESLURM_DATA_TOO_LARGE,
	ESLURM_DATA_FLAGS_INVALID_TYPE,
	ESLURM_DATA_FLAGS_INVALID,
	ESLURM_DATA_EXPECTED_LIST,
	ESLURM_DATA_EXPECTED_DICT,
	ESLURM_DATA_AMBIGUOUS_MODIFY,
	ESLURM_DATA_AMBIGUOUS_QUERY,
	ESLURM_DATA_PARSE_NOTHING,
	ESLURM_DATA_INVALID_PARSER,
	ESLURM_DATA_PARSING_DEPTH,
	ESLURM_DATA_PARSER_INVALID_STATE,

	/* container errors */
	ESLURM_CONTAINER_NOT_CONFIGURED = 10000,

	/* http and URL errors */
	ESLURM_URL_UNKNOWN_SCHEME = 11000,
	ESLURM_URL_EMPTY,
} slurm_err_t;

/* Type for error string table entries */
typedef struct {
	int xe_number;
	char *xe_name;
	char *xe_message;
} slurm_errtab_t;

extern slurm_errtab_t slurm_errtab[];
extern unsigned int slurm_errtab_size;

/* look up an errno value */
char * slurm_strerror(int errnum);

/* set an errno value */
void slurm_seterrno(int errnum);

/* print message: error string for current errno value */
void slurm_perror(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* !_SLURM_ERRNO_H */

/*****************************************************************************\
 *  acct_gather_profile_influxdb.c - slurm accounting plugin for influxdb
 *				     profiling.
 *****************************************************************************
 *  Author: Carlos Fenoy Garcia
 *  Copyright (C) 2016 F. Hoffmann - La Roche
 *
 *  Based on the HDF5 profiling plugin and Elasticsearch job completion plugin.
 *
 *  Portions Copyright (C) 2013 Bull S. A. S.
 *		Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois.
 *
 *  Copyright (C) SchedMD LLC.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <http://www.schedmd.com/slurmdocs/>.
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>

#include "src/common/slurm_xlator.h"
#include "src/common/fd.h"
#include "src/common/macros.h"
#include "src/interfaces/acct_gather_profile.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/slurm_protocol_defs.h"
#include "src/common/slurm_time.h"
#include "src/common/timers.h"
#include "src/common/xstring.h"
#include "src/curl/slurm_curl.h"
#include "src/interfaces/proctrack.h"

#define DEFAULT_INFLUXDB_TIMEOUT 10

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
const char plugin_name[] = "AcctGatherProfile influxdb plugin";
const char plugin_type[] = "acct_gather_profile/influxdb";
const uint32_t plugin_version = SLURM_VERSION_NUMBER;

typedef struct {
	char *host;
	char *database;
	uint32_t def;
	char *password;
	char *rt_policy;
	uint32_t timeout;
	char *username;
} slurm_influxdb_conf_t;

typedef struct {
	char ** names;
	uint32_t *types;
	size_t size;
	char * name;
} table_t;

union data_t{
	uint64_t u;
	double	 d;
};

static slurm_influxdb_conf_t influxdb_conf;
static uint32_t g_profile_running = ACCT_GATHER_PROFILE_NOT_SET;
static stepd_step_rec_t *g_job = NULL;

static char *datastr = NULL;
static int datastrlen = 0;

static table_t *tables = NULL;
static size_t tables_max_len = 0;
static size_t tables_cur_len = 0;

static void _free_tables(void)
{
	int i, j;

	debug3("%s %s called", plugin_type, __func__);

	if (!tables)
		return;

	for (i = 0; i < tables_cur_len; i++) {
		table_t *table = &(tables[i]);
		for (j = 0; j < table->size; j++)
			xfree(table->names[j]);
		xfree(table->name);
		xfree(table->names);
		xfree(table->types);
	}
	xfree(tables);
}

static uint32_t _determine_profile(void)
{
	uint32_t profile;

	debug3("%s %s called", plugin_type, __func__);
	xassert(g_job);

	if (g_profile_running != ACCT_GATHER_PROFILE_NOT_SET)
		profile = g_profile_running;
	else if (g_job->profile >= ACCT_GATHER_PROFILE_NONE)
		profile = g_job->profile;
	else
		profile = influxdb_conf.def;

	return profile;
}

/* Try to send data to influxdb */
static int _send_data(const char *data)
{
	int rc = SLURM_SUCCESS;
	long response_code = 0;
	char *url = NULL, *response_str = NULL;
	size_t length;

	debug3("%s %s called", plugin_type, __func__);

	/*
	 * Every compute node which is sampling data will try to establish a
	 * different connection to the influxdb server. In order to reduce the
	 * number of connections, every time a new sampled data comes in, it
	 * is saved in the 'datastr' buffer. Once this buffer is full, then we
	 * try to open the connection and send this buffer, instead of opening
	 * one per sample.
	 */
	if (data && ((datastrlen + strlen(data)) <= BUF_SIZE)) {
		xstrcat(datastr, data);
		length = strlen(data);
		datastrlen += length;
		log_flag(PROFILE, "%s %s: %zu bytes of data added to buffer. New buffer size: %d",
			 plugin_type, __func__, length, datastrlen);
		return rc;
	}

	xstrfmtcat(url, "%s/write?db=%s&rp=%s&precision=s", influxdb_conf.host,
		   influxdb_conf.database, influxdb_conf.rt_policy);
	rc = slurm_curl_request(datastr, url, influxdb_conf.username,
				influxdb_conf.password, NULL, NULL, NULL, false,
				influxdb_conf.timeout, &response_str,
				&response_code, HTTP_REQUEST_POST, true, false);
	xfree(url);

	/* In general, status codes of the form 2xx indicate success,
	 * 4xx indicate that InfluxDB could not understand the request, and
	 * 5xx indicate that the system is overloaded or significantly impaired.
	 * Errors are returned in JSON.
	 * https://docs.influxdata.com/influxdb/v0.13/concepts/api/
	 */
	if (rc != SLURM_SUCCESS) {
		error("send data failed");
	} else if ((response_code >= 200) && (response_code <= 205)) {
		debug2("%s %s: data write success", plugin_type, __func__);
	} else {
		rc = SLURM_ERROR;
		debug2("%s %s: data write failed, response code: %ld",
		       plugin_type, __func__, response_code);
		if (slurm_conf.debug_flags & DEBUG_FLAG_PROFILE) {
			/* Strip any trailing newlines. */
			while (response_str[strlen(response_str) - 1] == '\n')
				response_str[strlen(response_str) - 1] = '\0';
			info("%s %s: JSON response body: %s", plugin_type,
			     __func__, response_str);
		}
	}

	xfree(response_str);

	if (data) {
		datastr = xstrdup(data);
		datastrlen = strlen(data);
	} else {
		datastr[0] = '\0';
		datastrlen = 0;
	}

	return rc;
}

extern int init(void)
{
	debug3("%s %s called", plugin_type, __func__);

	if (!running_in_slurmstepd())
		return SLURM_SUCCESS;

	if (slurm_curl_init())
		return SLURM_ERROR;

	datastr = xmalloc(BUF_SIZE);
	return SLURM_SUCCESS;
}

extern void fini(void)
{
	debug3("%s %s called", plugin_type, __func__);

	slurm_curl_fini();

	_free_tables();
	xfree(datastr);
	xfree(influxdb_conf.host);
	xfree(influxdb_conf.database);
	xfree(influxdb_conf.password);
	xfree(influxdb_conf.rt_policy);
	xfree(influxdb_conf.username);
}

extern void acct_gather_profile_p_conf_options(s_p_options_t **full_options,
					       int *full_options_cnt)
{
	debug3("%s %s called", plugin_type, __func__);

	s_p_options_t options[] = {
		{"ProfileInfluxDBHost", S_P_STRING},
		{"ProfileInfluxDBDatabase", S_P_STRING},
		{"ProfileInfluxDBDefault", S_P_STRING},
		{"ProfileInfluxDBPass", S_P_STRING},
		{"ProfileInfluxDBRTPolicy", S_P_STRING},
		{"ProfileInfluxDBTimeout", S_P_UINT32},
		{"ProfileInfluxDBUser", S_P_STRING},
		{NULL} };

	transfer_s_p_options(full_options, options, full_options_cnt);
	return;
}

extern void acct_gather_profile_p_conf_set(s_p_hashtbl_t *tbl)
{
	char *tmp = NULL;

	debug3("%s %s called", plugin_type, __func__);

	influxdb_conf.def = ACCT_GATHER_PROFILE_ALL;
	if (tbl) {
		s_p_get_string(&influxdb_conf.host, "ProfileInfluxDBHost", tbl);
		if (s_p_get_string(&tmp, "ProfileInfluxDBDefault", tbl)) {
			influxdb_conf.def =
				acct_gather_profile_from_string(tmp);
			if (influxdb_conf.def == ACCT_GATHER_PROFILE_NOT_SET)
				fatal("ProfileInfluxDBDefault can not be set to %s, please specify a valid option",
				      tmp);
			xfree(tmp);
		}
		s_p_get_string(&influxdb_conf.database,
			       "ProfileInfluxDBDatabase", tbl);
		s_p_get_string(&influxdb_conf.password,
			       "ProfileInfluxDBPass", tbl);
		s_p_get_string(&influxdb_conf.rt_policy,
			       "ProfileInfluxDBRTPolicy", tbl);
		if (!s_p_get_uint32(&influxdb_conf.timeout,
				    "ProfileInfluxDBTimeout", tbl))
			influxdb_conf.timeout = DEFAULT_INFLUXDB_TIMEOUT;
		s_p_get_string(&influxdb_conf.username,
			       "ProfileInfluxDBUser", tbl);
	}

	if (!influxdb_conf.host)
		fatal("No ProfileInfluxDBHost in your acct_gather.conf file. This is required to use the %s plugin",
		      plugin_type);

	if (!influxdb_conf.database)
		fatal("No ProfileInfluxDBDatabase in your acct_gather.conf file. This is required to use the %s plugin",
		      plugin_type);

	if (influxdb_conf.password && !influxdb_conf.username)
		fatal("No ProfileInfluxDBUser in your acct_gather.conf file. This is required if ProfileInfluxDBPass is specified to use the %s plugin",
		      plugin_type);

	if (!influxdb_conf.rt_policy)
		fatal("No ProfileInfluxDBRTPolicy in your acct_gather.conf file. This is required to use the %s plugin",
		      plugin_type);

	debug("%s loaded", plugin_name);
}

extern void acct_gather_profile_p_get(enum acct_gather_profile_info info_type,
				      void *data)
{
	uint32_t *uint32 = (uint32_t *) data;
	char **tmp_char = (char **) data;

	debug3("%s %s called", plugin_type, __func__);

	switch (info_type) {
	case ACCT_GATHER_PROFILE_DIR:
		*tmp_char = xstrdup(influxdb_conf.host);
		break;
	case ACCT_GATHER_PROFILE_DEFAULT:
		*uint32 = influxdb_conf.def;
		break;
	case ACCT_GATHER_PROFILE_RUNNING:
		*uint32 = g_profile_running;
		break;
	default:
		debug2("%s %s: info_type %d invalid", plugin_type,
		       __func__, info_type);
	}
}

extern int acct_gather_profile_p_node_step_start(stepd_step_rec_t* job)
{
	int rc = SLURM_SUCCESS;
	char *profile_str;

	debug3("%s %s called", plugin_type, __func__);

	xassert(running_in_slurmstepd());

	g_job = job;
	profile_str = acct_gather_profile_to_string(g_job->profile);
	debug2("%s %s: option --profile=%s", plugin_type, __func__,
	       profile_str);
	g_profile_running = _determine_profile();
	return rc;
}

extern int acct_gather_profile_p_child_forked(void)
{
	debug3("%s %s called", plugin_type, __func__);
	return SLURM_SUCCESS;
}

extern int acct_gather_profile_p_node_step_end(void)
{
	int rc = SLURM_SUCCESS;
	debug3("%s %s called", plugin_type, __func__);

	xassert(running_in_slurmstepd());

	return rc;
}

extern int acct_gather_profile_p_task_start(uint32_t taskid)
{
	int rc = SLURM_SUCCESS;

	debug3("%s %s called with %d prof", plugin_type, __func__,
	       g_profile_running);

	xassert(running_in_slurmstepd());
	xassert(g_job);

	xassert(g_profile_running != ACCT_GATHER_PROFILE_NOT_SET);

	if (g_profile_running <= ACCT_GATHER_PROFILE_NONE)
		return rc;

	return rc;
}

extern int acct_gather_profile_p_task_end(pid_t taskpid)
{
	debug3("%s %s called", plugin_type, __func__);

	_send_data(NULL);
	return SLURM_SUCCESS;
}

extern int64_t acct_gather_profile_p_create_group(const char* name)
{
	debug3("%s %s called", plugin_type, __func__);

	return 0;
}

extern int acct_gather_profile_p_create_dataset(const char* name,
						int64_t parent,
						acct_gather_profile_dataset_t
						*dataset)
{
	table_t * table;
	acct_gather_profile_dataset_t *dataset_loc = dataset;

	debug3("%s %s called", plugin_type, __func__);

	if (g_profile_running <= ACCT_GATHER_PROFILE_NONE)
		return SLURM_ERROR;

	/* compute the size of the type needed to create the table */
	if (tables_cur_len == tables_max_len) {
		if (tables_max_len == 0)
			++tables_max_len;
		tables_max_len *= 2;
		tables = xrealloc(tables, tables_max_len * sizeof(table_t));
	}

	table = &(tables[tables_cur_len]);
	table->name = xstrdup(name);
	table->size = 0;

	while (dataset_loc && (dataset_loc->type != PROFILE_FIELD_NOT_SET)) {
		table->names = xrealloc(table->names,
					(table->size+1) * sizeof(char *));
		table->types = xrealloc(table->types,
					(table->size+1) * sizeof(char *));
		(table->names)[table->size] = xstrdup(dataset_loc->name);
		switch (dataset_loc->type) {
		case PROFILE_FIELD_UINT64:
			table->types[table->size] =
				PROFILE_FIELD_UINT64;
			break;
		case PROFILE_FIELD_DOUBLE:
			table->types[table->size] =
				PROFILE_FIELD_DOUBLE;
			break;
		case PROFILE_FIELD_NOT_SET:
			break;
		}
		table->size++;
		dataset_loc++;
	}
	++tables_cur_len;
	return tables_cur_len - 1;
}

extern int acct_gather_profile_p_add_sample_data(int table_id, void *data,
						 time_t sample_time)
{
	table_t *table = &tables[table_id];
	int i = 0;
	char *str = NULL;

	debug3("%s %s called", plugin_type, __func__);

	for(; i < table->size; i++) {
		switch (table->types[i]) {
		case PROFILE_FIELD_UINT64:
			xstrfmtcat(str, "%s,job=%d,step=%d,task=%s,"
				   "host=%s value=%"PRIu64" "
				   "%"PRIu64"\n", table->names[i],
				   g_job->step_id.job_id,
				   g_job->step_id.step_id,
				   table->name, g_job->node_name,
				   ((union data_t*)data)[i].u,
				   (uint64_t)sample_time);
			break;
		case PROFILE_FIELD_DOUBLE:
			xstrfmtcat(str, "%s,job=%d,step=%d,task=%s,"
				   "host=%s value=%.2f %"PRIu64""
				   "\n", table->names[i],
				   g_job->step_id.job_id,
				   g_job->step_id.step_id,
				   table->name, g_job->node_name,
				   ((union data_t*)data)[i].d,
				   (uint64_t)sample_time);
			break;
		case PROFILE_FIELD_NOT_SET:
			break;
		}
	}

	_send_data(str);
	xfree(str);

	return SLURM_SUCCESS;
}

extern void acct_gather_profile_p_conf_values(list_t **data)
{
	add_key_pair(*data, "ProfileInfluxDBHost", "%s",
		     influxdb_conf.host);

	add_key_pair(*data, "ProfileInfluxDBDatabase", "%s",
		     influxdb_conf.database);

	add_key_pair(*data, "ProfileInfluxDBDefault", "%s",
		     acct_gather_profile_to_string(influxdb_conf.def));

	/* skip over ProfileInfluxDBPass for security reasons */

	add_key_pair(*data, "ProfileInfluxDBRTPolicy", "%s",
		     influxdb_conf.rt_policy);

	add_key_pair(*data, "ProfileInfluxDBTimeout", "%u",
		     influxdb_conf.timeout);

	/* skip over ProfileInfluxDBUser for security reasons */
}

extern bool acct_gather_profile_p_is_active(uint32_t type)
{
	debug3("%s %s called", plugin_type, __func__);

	if (g_profile_running <= ACCT_GATHER_PROFILE_NONE)
		return false;

	return (type == ACCT_GATHER_PROFILE_NOT_SET) ||
		(g_profile_running & type);
}

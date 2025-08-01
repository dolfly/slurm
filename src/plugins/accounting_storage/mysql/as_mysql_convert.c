/*****************************************************************************\
 *  as_mysql_convert.c - functions dealing with converting from tables in
 *                    slurm <= 17.02.
 *****************************************************************************
 *  Copyright (C) SchedMD LLC.
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

#include "as_mysql_cluster.h"
#include "as_mysql_convert.h"
#include "as_mysql_tres.h"
#include "src/interfaces/jobacct_gather.h"

/*
 * Any time you have to add to an existing convert update this number.
 * NOTE: 15 was the last version before 24.11.
 * NOTE: 16 was the first version of 24.11.
 */
#define CONVERT_VERSION 16

#define MIN_CONVERT_VERSION 15

#define JOB_CONVERT_LIMIT_CNT 1000

typedef enum {
	MOVE_ENV,
	MOVE_BATCH
} move_large_type_t;

typedef struct {
	uint64_t count;
	uint32_t id;
} local_tres_t;

static uint32_t db_curr_ver = NO_VAL;

static int _convert_clus_res_table_pre(mysql_conn_t *mysql_conn)
{
	int rc = SLURM_SUCCESS;

	return rc;
}

static int _convert_job_table_pre(mysql_conn_t *mysql_conn, char *cluster_name)
{
	int rc = SLURM_SUCCESS;

	return rc;
}

static int _convert_step_table_pre(mysql_conn_t *mysql_conn, char *cluster_name)
{
	int rc = SLURM_SUCCESS;

	return rc;
}
static int _set_db_curr_ver(mysql_conn_t *mysql_conn)
{
	char *query;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;
	int rc = SLURM_SUCCESS;

	if (db_curr_ver != NO_VAL)
		return SLURM_SUCCESS;

	query = xstrdup_printf("select version from %s", convert_version_table);
	DB_DEBUG(DB_QUERY, mysql_conn->conn, "query\n%s", query);
	if (!(result = mysql_db_query_ret(mysql_conn, query, 0))) {
		xfree(query);
		return SLURM_ERROR;
	}
	xfree(query);
	row = mysql_fetch_row(result);

	if (row) {
		db_curr_ver = slurm_atoul(row[0]);
		mysql_free_result(result);
	} else {
		int tmp_ver = CONVERT_VERSION;
		mysql_free_result(result);

		query = xstrdup_printf("insert into %s (version) values (%d);",
				       convert_version_table, tmp_ver);
		DB_DEBUG(DB_QUERY, mysql_conn->conn, "query\n%s", query);
		rc = mysql_db_query(mysql_conn, query);
		xfree(query);
		if (rc != SLURM_SUCCESS)
			return SLURM_ERROR;
		db_curr_ver = tmp_ver;
	}

	return rc;
}

extern int as_mysql_print_dbver(mysql_conn_t *mysql_conn)
{
	int rc = 0;

	as_mysql_convert_possible(mysql_conn);

	if (db_curr_ver > CONVERT_VERSION) {
		printf("Slurm Database is somehow higher than expected '%u' but I only know as high as '%u'. Conversion needed.\n",
		       db_curr_ver, CONVERT_VERSION);
		rc = 1;
	} else if (db_curr_ver != CONVERT_VERSION) {
		printf("Slurm Database current version is '%u' needs to be at '%u'. Conversion needed.\n",
		       db_curr_ver, CONVERT_VERSION);
		rc = 1;
	} else {
		printf("Slurm Database already at the highest version '%u'. No conversion needed.\n",
		       db_curr_ver);
	}

	return rc;
}

extern void as_mysql_convert_possible(mysql_conn_t *mysql_conn)
{
	(void) _set_db_curr_ver(mysql_conn);

	/*
	 * Check to see if conversion is possible.
	 */
	if (db_curr_ver == NO_VAL) {
		/*
		 * Check if the cluster_table exists before deciding if this is
		 * a new database or a database that predates the
		 * convert_version_table.
		 */
		MYSQL_RES *result = NULL;
		char *query = xstrdup_printf("select name from %s limit 1",
					     cluster_table);
		DB_DEBUG(DB_QUERY, mysql_conn->conn, "query\n%s", query);
		if ((result = mysql_db_query_ret(mysql_conn, query, 0))) {
			/*
			 * knowing that the table exists is enough to say this
			 * is an old database.
			 */
			xfree(query);
			mysql_free_result(result);
			fatal("Database schema is too old for this version of Slurm to upgrade.");
		}
		xfree(query);
		debug4("Database is new, conversion is not required");
	} else if (db_curr_ver < MIN_CONVERT_VERSION) {
		fatal("Database schema is too old for this version of Slurm to upgrade.");
	} else if (db_curr_ver > CONVERT_VERSION) {
		char *err_msg = "Database schema is from a newer version of Slurm, downgrading is not possible.";
		/*
		 * If we are configured --enable-debug only make this a
		 * debug statement instead of fatal to allow developers
		 * easier bisects.
		 */
#ifdef NDEBUG
		fatal("%s", err_msg);
#else
		debug("%s", err_msg);
#endif
	}
}

extern int as_mysql_convert_tables_pre_create(mysql_conn_t *mysql_conn)
{
	int rc = SLURM_SUCCESS;
	list_itr_t *itr;
	char *cluster_name;

	xassert(as_mysql_total_cluster_list);

	if ((rc = _set_db_curr_ver(mysql_conn)) != SLURM_SUCCESS)
		return rc;

	if (db_curr_ver == CONVERT_VERSION) {
		debug4("No conversion needed, Hooray!");
		return SLURM_SUCCESS;
	} else if (backup_dbd) {
		/*
		 * We do not want to create/check the database if we are the
		 * backup (see Bug 3827). This is only handled on the primary.
		 *
		 * To avoid situations where someone might upgrade the database
		 * through the backup we want to fatal so they know what
		 * happened instead of potentially starting with the older
		 * database.
		 */
		fatal("Backup DBD can not convert database, please start the primary DBD before starting the backup.");
		return SLURM_ERROR;
	}

	/*
	 * At this point, its clear an upgrade is being performed.
	 * Setup the galera cluster specific options if applicable.
	 *
	 * If this fails for whatever reason, it does not mean that the upgrade
	 * will fail, but it might.
	 */
	mysql_db_enable_streaming_replication(mysql_conn);

	info("pre-converting cluster resource table");
	if ((rc = _convert_clus_res_table_pre(mysql_conn)) != SLURM_SUCCESS)
		return rc;

	/* make it up to date */
	itr = list_iterator_create(as_mysql_total_cluster_list);
	while ((cluster_name = list_next(itr))) {
		/*
		 * When calling alters on tables here please remember to use
		 * as_mysql_convert_alter_query instead of mysql_db_query to be
		 * able to detect a previous failed conversion.
		 */
		info("pre-converting job table for %s", cluster_name);
		if ((rc = _convert_job_table_pre(mysql_conn, cluster_name))
		     != SLURM_SUCCESS)
			break;
		info("pre-converting step table for %s", cluster_name);
		if ((rc = _convert_step_table_pre(mysql_conn, cluster_name))
		     != SLURM_SUCCESS)
			break;
	}
	list_iterator_destroy(itr);

	return rc;
}

static int _convert_assoc_table_post(mysql_conn_t *mysql_conn,
				     char *cluster_name)
{
	int rc = SLURM_SUCCESS;

	return rc;
}

static int _foreach_post_create(void *x, void *arg)
{
	char *cluster_name = x;
	mysql_conn_t *mysql_conn = arg;
	int rc;

	info("post-converting assoc table for %s", cluster_name);
	if ((rc = _convert_assoc_table_post(mysql_conn, cluster_name)) !=
	     SLURM_SUCCESS)
		return rc;

	if (db_curr_ver < 16) {
		uint16_t id = as_mysql_cluster_get_unique_id(mysql_conn,
							     cluster_name, 0);
		char *query = xstrdup_printf(
			"update %s set id=%u, mod_time=UNIX_TIMESTAMP() where name='%s'",
			cluster_table, id, cluster_name);

		info("Setting cluster id '%u' on cluster %s", id, cluster_name);

		DB_DEBUG(DB_QUERY, mysql_conn->conn, "query\n%s", query);
		rc = mysql_db_query(mysql_conn, query);
		xfree(query);
	}

	return SLURM_SUCCESS;
}

extern int as_mysql_convert_tables_post_create(mysql_conn_t *mysql_conn)
{
	int rc = SLURM_SUCCESS;

	xassert(as_mysql_total_cluster_list);

	if ((rc = _set_db_curr_ver(mysql_conn)) != SLURM_SUCCESS)
		return rc;

	if (db_curr_ver == CONVERT_VERSION) {
		debug4("No conversion needed, Hooray!");
		return SLURM_SUCCESS;
	} else if (backup_dbd) {
		/*
		 * We do not want to create/check the database if we are the
		 * backup (see Bug 3827). This is only handled on the primary.
		 *
		 * To avoid situations where someone might upgrade the database
		 * through the backup we want to fatal so they know what
		 * happened instead of potentially starting with the older
		 * database.
		 */
		fatal("Backup DBD can not convert database, please start the primary DBD before starting the backup.");
		return SLURM_ERROR;
	}

	/* make it up to date */
	if (list_for_each_ro(as_mysql_total_cluster_list,
			     _foreach_post_create, mysql_conn) < 0)
		return SLURM_ERROR;

	return SLURM_SUCCESS;
}

extern int as_mysql_convert_non_cluster_tables_post_create(
	mysql_conn_t *mysql_conn)
{
	int rc = SLURM_SUCCESS;

	if ((rc = _set_db_curr_ver(mysql_conn)) != SLURM_SUCCESS)
		return rc;

	if (db_curr_ver == CONVERT_VERSION) {
		debug4("No conversion needed, Hooray!");
		return SLURM_SUCCESS;
	}

	if (rc == SLURM_SUCCESS) {
		char *query = xstrdup_printf(
			"update %s set version=%d, mod_time=UNIX_TIMESTAMP()",
			convert_version_table, CONVERT_VERSION);

		info("Conversion done: success!");

		DB_DEBUG(DB_QUERY, mysql_conn->conn, "query\n%s", query);
		rc = mysql_db_query(mysql_conn, query);
		xfree(query);
	}

	return rc;
}

/*
 * Only use this when running "ALTER TABLE" during an upgrade.  This is to get
 * around that mysql cannot rollback an "ALTER TABLE", but its possible that the
 * rest of the upgrade transaction was aborted.
 *
 * We may not always use this function, but don't delete it just in case we
 * need to alter tables in the future.
 */
extern int as_mysql_convert_alter_query(mysql_conn_t *mysql_conn, char *query)
{
	int rc = SLURM_SUCCESS;

	rc = mysql_db_query(mysql_conn, query);
	if ((rc != SLURM_SUCCESS) && (errno == ER_BAD_FIELD_ERROR)) {
		errno = 0;
		rc = SLURM_SUCCESS;
		info("The database appears to have been altered by a previous upgrade attempt, continuing with upgrade.");
	}

	return rc;
}

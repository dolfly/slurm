/*****************************************************************************\
 *  auth_none.c - NO-OP slurm authentication plugin, validates all users.
 *****************************************************************************
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
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

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "slurm/slurm_errno.h"
#include "src/common/slurm_xlator.h"

#include "src/common/pack.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/xmalloc.h"
#include "src/common/xstring.h"

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
 * the plugin (e.g., "auth" for Slurm authentication) and <method> is a
 * description of how this plugin satisfies that application.  Slurm will
 * only load authentication plugins if the plugin_type string has a prefix
 * of "auth/".
 *
 * plugin_version - an unsigned 32-bit integer containing the Slurm version
 * (major.minor.micro combined into a single number).
 */
const char plugin_name[] = "Null authentication plugin";
const char plugin_type[] = "auth/none";
const uint32_t plugin_id = AUTH_PLUGIN_NONE;
const uint32_t plugin_version = SLURM_VERSION_NUMBER;
const bool hash_enable = false;

/*
 * An opaque type representing authentication credentials.  This type can be
 * called whatever is meaningful and may contain any data required by the
 * plugin.  However, the plugin must be able to recover the Linux UID and
 * GID given only an object of this type.
 *
 * Since no verification of the credentials is performed in the "none"
 * authentication, this plugin simply uses the system-supplied UID and GID.
 * In a more robust authentication context, this might include tickets or
 * other signatures which the functions of this API can use to conduct
 * verification.
 *
 * The client code never sees the inside of this structure directly.
 * Objects of this type are passed in and out of the plugin via
 * anonymous pointers.  Because of this, robust plugins may wish to add
 * some form of runtime typing to ensure that the pointers they have
 * received are actually appropriate credentials and not pointers to
 * random memory.
 *
 * A word about thread safety.  The authentication plugin API specifies
 * that Slurm will exercise the plugin sanely.  That is, the authenticity
 * of a credential which has not been activated should not be tested.
 * However, the credential should be thread-safe.  This does not mean
 * necessarily that a plugin must recognize when an inconsistent sequence
 * of API calls is in progress, but if a plugin will crash or otherwise
 * misbehave if it is handed a credential in an inconsistent state (perhaps
 * it is in the process of being activated and a signature is incomplete)
 * then it is the plugin's responsibility to provide its own serialization\
 * to avoid that.
 *
 */
typedef struct {
	int index; /* MUST ALWAYS BE FIRST. DO NOT PACK. */
	char *hostname;
	uid_t uid;
	gid_t gid;
} auth_credential_t;

extern int init(void)
{
	debug("%s loaded", plugin_name);
	return SLURM_SUCCESS;
}

extern void fini(void)
{
	return;
}

/*
 * The remainder of this file implements the standard Slurm authentication
 * API.
 */

/*
 * Allocate and initializes a credential.  This function should return
 * NULL if it cannot allocate a credential.
 */
auth_credential_t *auth_p_create(char *auth_info, uid_t r_uid,
				 void *data, int dlen)
{
	auth_credential_t *cred = xmalloc(sizeof(*cred));

	cred->uid = geteuid();
	cred->gid = getegid();

	cred->hostname = xshort_hostname();

	return cred;
}

/*
 * Free a credential that was allocated with auth_p_create() or
 * auth_p_unpack().
 */
extern void auth_p_destroy(auth_credential_t *cred)
{
	if (!cred)
		return;

	xfree(cred->hostname);
	xfree(cred);
}

/*
 * Verify a credential to approve or deny authentication.
 *
 * Return SLURM_SUCCESS if the credential is in order and valid.
 */
int auth_p_verify(auth_credential_t *cred, char *auth_info)
{
	return SLURM_SUCCESS;
}

extern void auth_p_get_ids(auth_credential_t *cred, uid_t *uid, gid_t *gid)
{
	if (!cred) {
		*uid = SLURM_AUTH_NOBODY;
		*gid = SLURM_AUTH_NOBODY;
		return;
	}

	*uid = cred->uid;
	*gid = cred->gid;
}

/*
 * Obtain the originating hostname from the credential.
 * See auth_p_get_uid() above for details on correct behavior.
 */
char *auth_p_get_host(auth_credential_t *cred)
{
	if (!cred) {
		errno = ESLURM_AUTH_BADARG;
		return NULL;
	}

	return xstrdup(cred->hostname);
}

extern int auth_p_get_data(auth_credential_t *cred, char **data, uint32_t *len)
{
	if (!cred) {
		errno = ESLURM_AUTH_BADARG;
		return SLURM_ERROR;
	}

	*data = NULL;
	*len = 0;

	return SLURM_SUCCESS;
}

extern void *auth_p_get_identity(auth_credential_t *cred)
{
	if (!cred) {
		errno = ESLURM_AUTH_BADARG;
		return NULL;
	}

	return NULL;
}

/*
 * Marshall a credential for transmission over the network, according to
 * Slurm's marshalling protocol.
 */
int auth_p_pack(auth_credential_t *cred, buf_t *buf, uint16_t protocol_version)
{
	if (!cred || !buf) {
		errno = ESLURM_AUTH_BADARG;
		return SLURM_ERROR;
	}

	if (protocol_version >= SLURM_MIN_PROTOCOL_VERSION) {
		pack32(cred->uid, buf);
		pack32(cred->gid, buf);
		packstr(cred->hostname, buf);
	} else {
		error("%s: Unknown protocol version %d",
		      __func__, protocol_version);
		return SLURM_ERROR;
	}

	return SLURM_SUCCESS;
}

/*
 * Unmarshall a credential after transmission over the network according
 * to Slurm's marshalling protocol.
 */
auth_credential_t *auth_p_unpack(buf_t *buf, uint16_t protocol_version)
{
	auth_credential_t *cred = NULL;

	if (!buf) {
		errno = ESLURM_AUTH_BADARG;
		return NULL;
	}

	/* Allocate a new credential. */
	cred = xmalloc(sizeof(*cred));

	if (protocol_version >= SLURM_MIN_PROTOCOL_VERSION) {
		safe_unpack32(&cred->uid, buf);
		safe_unpack32(&cred->gid, buf);
		safe_unpackstr(&cred->hostname, buf);
	} else {
		error("%s: unknown protocol version %u",
		      __func__, protocol_version);
		goto unpack_error;
	}

	return cred;

unpack_error:
	auth_p_destroy(cred);
	errno = ESLURM_AUTH_UNPACK;
	return NULL;
}

int auth_p_thread_config(const char *token, const char *username)
{
	/* No auth -> everything works */
	return SLURM_SUCCESS;
}

void auth_p_thread_clear(void)
{
	/* no op */
}

char *auth_p_token_generate(const char *username, int lifespan)
{
	return NULL;
}

extern int auth_p_get_reconfig_fd(void)
{
	return -1;
}

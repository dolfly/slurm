/*****************************************************************************\
 *  state_save.h - common state save and load handling
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

#ifndef _COMMON_STATE_SAVE_H
#define _COMMON_STATE_SAVE_H

#include "src/common/pack.h"

extern int clustername_existed;

/* mutex used for saving state of slurmctld */
extern void lock_state_files(void);
extern void unlock_state_files(void);

/*
 * Safe buffer data to state file
 *
 * IN target_file - Path to state file in slurm_conf.state_save_location,
 *                  or absolute path to state file.
 * IN buf - Data to save to target_file
 * OUT high_buffer_size - High watermark of buffers saved in previous calls.
 *
 * RET SLURM_SUCCESS or error
 */
extern int save_buf_to_state(const char *target_file, buf_t *buf,
			     uint32_t *high_buffer_size);

/*
 * Open buffer to state file
 *
 * IN target_file - Path to state file in slurm_conf.state_save_location,
 *                  or absolute path to state file.
 * OUT state_file - Absolute path to state file. Must be xfree'd.
 *
 * RET buf_t pointing to data in target_file
 */
extern buf_t *state_save_open(const char *target_file, char **state_file);

#endif

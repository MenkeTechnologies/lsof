/*
 * dproto.h -- Darwin function prototypes for libproc-based lsof
 *
 * dproto.h -- Darwin function prototypes for libproc-based lsof
 */

/*
 * Portions Copyright 2005 Apple Computer, Inc.  All rights reserved.
 *
 *
 * Written by Allan Nathanson, Apple Computer, Inc., and Victor A.
 * Abell.
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors, nor Apple Computer, Inc.
 *    are responsible for any consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either
 *    by explicit claim or by omission.  Credit to the authors, Apple
 *    Computer, Inc. must appear in documentation
 *    and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * $Id: dproto.h,v 1.5 2006/03/27 23:23:13 abe Exp $
 */

extern void enter_file_info(struct proc_fileinfo *pfi);

extern void enter_vnode_info(struct vnode_info_path *vip);

extern void err2nm(char *pfx);

extern int is_file_named(char *p, int cd);

extern void process_atalk(int pid, int32_t fd);

extern void process_fsevents(int pid, int32_t fd);

extern void process_kqueue(int pid, int32_t fd);

extern void process_pipe(int pid, int32_t fd);

extern void process_psem(int pid, int32_t fd);

extern void process_pshm(int pid, int32_t fd);

extern void process_socket(int pid, int32_t fd);

extern void process_vnode(int pid, int32_t fd);

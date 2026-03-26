/*
 * node.c - common node reading functions for lsof
 */


/*
 *
 * Written by Jacob Menke
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
        "@(#) Copyright 1994 lsof contributors.\nAll rights reserved.\n";
#endif


#include "lsof.h"


/*
 * print_kptr() - print kernel pointer
 */

char *
print_kptr(kern_ptr, buf, bufl)
        KA_T kern_ptr;            /* kernel pointer address */
        char *buf;            /* optional destination buffer */
        size_t bufl;            /* size of buf[] */
{
    static char dbuf[32];

    (void) snpf(buf ? buf : dbuf,
                buf ? bufl : sizeof(dbuf),
                KA_T_FMT_X, kern_ptr);
    return (buf ? buf : dbuf);
}


#if    defined(HASCDRNODE)
/*
 * readcdrnode() - read CD-ROM node
 */

int
readcdrnode(cdr_addr, cdr_buf)
    KA_T cdr_addr;			/* cdrnode kernel address */
    struct cdrnode *cdr_buf;		/* cdrnode buffer */
{
    if (kread((KA_T)cdr_addr, (char *)cdr_buf, sizeof(struct cdrnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read cdrnode at %s",
        print_kptr(cdr_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASCDRNODE) */


#if    defined(HASFIFONODE)
/*
 * readfifonode() - read fifonode
 */

int
readfifonode(fifo_addr, fifo_buf)
    KA_T fifo_addr;			/* fifonode kernel address */
    struct fifonode *fifo_buf;		/* fifonode buffer */
{
    if (kread((KA_T)fifo_addr, (char *)fifo_buf, sizeof(struct fifonode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read fifonode at %s",
        print_kptr(fifo_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASFIFONODE) */


#if    defined(HASGNODE)
/*
 * readgnode() - read gnode
 */

int
readgnode(gnode_addr, gnode_buf)
    KA_T gnode_addr;			/* gnode kernel address */
    struct gnode *gnode_buf;		/* gnode buffer */
{
    if (kread((KA_T)gnode_addr, (char *)gnode_buf, sizeof(struct gnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read gnode at %s",
        print_kptr(gnode_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASGNODE) */


#if    defined(HASHSNODE)
/*
 * readhsnode() - read High Sierra file system node
 */

int
readhsnode(hs_addr, hs_buf)
    KA_T hs_addr;			/* hsnode kernel address */
    struct hsnode *hs_buf;		/* hsnode buffer */
{
    if (kread((KA_T)hs_addr, (char *)hs_buf, sizeof(struct hsnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read hsnode at %s",
        print_kptr(hs_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASHSNODE) */


#if    defined(HASINODE)
/*
 * readinode() - read inode
 */

int
readinode(inode_addr, inode_buf)
    KA_T inode_addr;			/* inode kernel address */
    struct inode *inode_buf;		/* inode buffer */
{
    if (kread((KA_T)inode_addr, (char *)inode_buf, sizeof(struct inode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read inode at %s",
        print_kptr(inode_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASINODE) */


#if    defined(HASPIPENODE)
/*
 * readpipenode() - read pipe node
 */

int
readpipenode(pipe_addr, pipe_buf)
    KA_T pipe_addr;			/* pipe node kernel address */
    struct pipenode *pipe_buf;		/* pipe node buffer */
{
    if (kread((KA_T)pipe_addr, (char *)pipe_buf, sizeof(struct pipenode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read pipenode at %s",
        print_kptr(pipe_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASPIPENODE) */


#if    defined(HASRNODE)
/*
 * readrnode() - read rnode
 */

int
readrnode(rnode_addr, rnode_buf)
    KA_T rnode_addr;			/* rnode kernel space address */
    struct rnode *rnode_buf;		/* rnode buffer pointer */
{
    if (kread((KA_T)rnode_addr, (char *)rnode_buf, sizeof(struct rnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read rnode at %s",
        print_kptr(rnode_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASRNODE) */


#if    defined(HASSNODE)
/*
 * readsnode() - read snode
 */

int
readsnode(snode_addr, snode_buf)
    KA_T snode_addr;			/* snode kernel space address */
    struct snode *snode_buf;		/* snode buffer pointer */
{
    if (kread((KA_T)snode_addr, (char *)snode_buf, sizeof(struct snode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read snode at %s",
        print_kptr(snode_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASSNODE) */


#if    defined(HASTMPNODE)
/*
 * readtnode() - read tmpnode
 */

int
readtnode(tmp_addr, tmp_buf)
    KA_T tmp_addr;			/* tmpnode kernel space address */
    struct tmpnode *tmp_buf;		/* tmpnode buffer pointer */
{
    if (kread((KA_T)tmp_addr, (char *)tmp_buf, sizeof(struct tmpnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read tmpnode at %s",
        print_kptr(tmp_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASTMPNODE) */


#if    defined(HASVNODE)
/*
 * readvnode() - read vnode
 */

int
readvnode(vnode_addr, vnode_buf)
    KA_T vnode_addr;			/* vnode kernel space address */
    struct vnode *vnode_buf;		/* vnode buffer pointer */
{
    if (kread((KA_T)vnode_addr, (char *)vnode_buf, sizeof(struct vnode))) {
        (void) snpf(NameChars, NameCharsLength, "can't read vnode at %s",
        print_kptr(vnode_addr, (char *)NULL, 0));
        return(1);
    }
    return(0);
}
#endif    /* defined(HASVNODE) */

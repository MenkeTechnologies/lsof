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

#include "lsof.h"

/*
 * print_kptr() - print kernel pointer
 */

char *print_kptr(KA_T kern_ptr, char *buf, size_t bufl) {
    static char dbuf[32];

    snpf(buf ? buf : dbuf, buf ? bufl : sizeof(dbuf), KA_T_FMT_X, kern_ptr);
    return (buf ? buf : dbuf);
}

#if defined(HASCDRNODE)
/*
 * readcdrnode() - read CD-ROM node
 */

int readcdrnode(KA_T cdr_addr, struct cdrnode *cdr_buf) {
    if (kread((KA_T)cdr_addr, (char *)cdr_buf, sizeof(struct cdrnode))) {
        snpf(NameChars, NameCharsLength, "can't read cdrnode at %s", print_kptr(cdr_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASFIFONODE)
/*
 * readfifonode() - read fifonode
 */

int readfifonode(KA_T fifo_addr, struct fifonode *fifo_buf) {
    if (kread((KA_T)fifo_addr, (char *)fifo_buf, sizeof(struct fifonode))) {
        snpf(NameChars, NameCharsLength, "can't read fifonode at %s",
             print_kptr(fifo_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASGNODE)
/*
 * readgnode() - read gnode
 */

int readgnode(KA_T gnode_addr, struct gnode *gnode_buf) {
    if (kread((KA_T)gnode_addr, (char *)gnode_buf, sizeof(struct gnode))) {
        snpf(NameChars, NameCharsLength, "can't read gnode at %s", print_kptr(gnode_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASHSNODE)
/*
 * readhsnode() - read High Sierra file system node
 */

int readhsnode(KA_T hs_addr, struct hsnode *hs_buf) {
    if (kread((KA_T)hs_addr, (char *)hs_buf, sizeof(struct hsnode))) {
        snpf(NameChars, NameCharsLength, "can't read hsnode at %s", print_kptr(hs_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASINODE)
/*
 * readinode() - read inode
 */

int readinode(KA_T inode_addr, struct inode *inode_buf) {
    if (kread((KA_T)inode_addr, (char *)inode_buf, sizeof(struct inode))) {
        snpf(NameChars, NameCharsLength, "can't read inode at %s", print_kptr(inode_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASPIPENODE)
/*
 * readpipenode() - read pipe node
 */

int readpipenode(KA_T pipe_addr, struct pipenode *pipe_buf) {
    if (kread((KA_T)pipe_addr, (char *)pipe_buf, sizeof(struct pipenode))) {
        snpf(NameChars, NameCharsLength, "can't read pipenode at %s",
             print_kptr(pipe_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASRNODE)
/*
 * readrnode() - read rnode
 */

int readrnode(KA_T rnode_addr, struct rnode *rnode_buf) {
    if (kread((KA_T)rnode_addr, (char *)rnode_buf, sizeof(struct rnode))) {
        snpf(NameChars, NameCharsLength, "can't read rnode at %s", print_kptr(rnode_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASSNODE)
/*
 * readsnode() - read snode
 */

int readsnode(KA_T snode_addr, struct snode *snode_buf) {
    if (kread((KA_T)snode_addr, (char *)snode_buf, sizeof(struct snode))) {
        snpf(NameChars, NameCharsLength, "can't read snode at %s", print_kptr(snode_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASTMPNODE)
/*
 * readtnode() - read tmpnode
 */

int readtnode(KA_T tmp_addr, struct tmpnode *tmp_buf) {
    if (kread((KA_T)tmp_addr, (char *)tmp_buf, sizeof(struct tmpnode))) {
        snpf(NameChars, NameCharsLength, "can't read tmpnode at %s", print_kptr(tmp_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

#if defined(HASVNODE)
/*
 * readvnode() - read vnode
 */

int readvnode(KA_T vnode_addr, struct vnode *vnode_buf) {
    if (kread((KA_T)vnode_addr, (char *)vnode_buf, sizeof(struct vnode))) {
        snpf(NameChars, NameCharsLength, "can't read vnode at %s", print_kptr(vnode_addr, NULL, 0));
        return (1);
    }
    return (0);
}
#endif

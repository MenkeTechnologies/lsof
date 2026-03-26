/*
 * proto.h - common function prototypes for lsof
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


/*
 * $Id: proto.h,v 1.36 2011/09/07 19:13:49 abe Exp $
 */


#if    !defined(PROTO_H)
#define    PROTO_H    1


/*
 * The _PROTOTYPE macro provides strict ANSI C prototypes if __STDC__
 * is defined, and old-style K&R prototypes otherwise.
 *
 * (With thanks to Andy Tanenbaum)
 */

# if    defined(__STDC__)
#define    _PROTOTYPE(function, params)    function params
# else	/* !defined(__STDC__) */
#define	_PROTOTYPE(function, params)	function()
# endif /* defined(__STDC__) */


/*
 * The following define keeps gcc>=2.7 from complaining about the failure
 * of the Exit() function to return.
 *
 * Paul Eggert supplied it.
 */

# if    defined(__GNUC__) && !(__GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7))
#define    exiting    __attribute__((__noreturn__))
# else	/* !gcc || gcc<2.7 */
#define	exiting
# endif    /* gcc && gcc>=2.7 */


_PROTOTYPE(extern void add_nma, (char *str, int len));

_PROTOTYPE(extern void alloc_lfile, (char *nm, int num));

_PROTOTYPE(extern void alloc_lproc, (int pid, int pgid, int ppid, UID_ARG uid, char *cmd, int pss, int sel_flags));

_PROTOTYPE(extern void build_IPstates, (void));

_PROTOTYPE(extern void childx, (void));

_PROTOTYPE(extern int ck_fd_status, (char *nm, int num));

_PROTOTYPE(extern int ck_file_arg, (int first_arg_idx, int arg_count, char *av[], int fs_value, int readlink_status, struct stat *sbp));

_PROTOTYPE(extern void ckkv, (char *dialect, char *expected_rev, char *expected_ver, char *expected_arch));

_PROTOTYPE(extern void clr_devtab, (void));

_PROTOTYPE(extern int compdev, (COMP_P * lhs, COMP_P * rhs));

_PROTOTYPE(extern int comppid, (COMP_P * lhs, COMP_P * rhs));

# if    defined(WILLDROPGID)
_PROTOTYPE(extern void dropgid,(void));
# endif    /* defined(WILLDROPGID) */

_PROTOTYPE(extern char *endnm, (size_t * remaining_size));

_PROTOTYPE(extern int enter_cmd_rx, (char *regex_str));

_PROTOTYPE(extern void enter_dev_ch, (char *m));

_PROTOTYPE(extern int enter_dir, (char *dir_path, int descend));

# if    defined(HASEOPT)
_PROTOTYPE(extern int enter_efsys,(char *path, int rdlnk));
# endif    /* defined(HASEOPT) */

_PROTOTYPE(extern int enter_fd, (char *fd_str));

_PROTOTYPE(extern int enter_network_address, (char *na));

_PROTOTYPE(extern int enter_id, (enum IDType type, char *id_str));

_PROTOTYPE(extern void enter_IPstate, (char *type, char *name, int nr));

_PROTOTYPE(extern void enter_nm, (char *m));

# if    defined(HASTCPUDPSTATE)
_PROTOTYPE(extern int enter_state_spec,(char *ss));
# endif    /* defined(HASTCPUDPSTATE) */

_PROTOTYPE(extern int enter_str_lst, (char *opt, char *str_val, struct str_lst **list_ptr,
        int *incl, int *excl));

_PROTOTYPE(extern int enter_uid, (char *us));

_PROTOTYPE(extern void ent_inaddr, (unsigned char *local_addr, int lp, unsigned char *foreign_addr, int fp, int af));

_PROTOTYPE(extern int examine_lproc, (void));

_PROTOTYPE(extern void Exit, (int xv)) exiting;

_PROTOTYPE(extern void find_ch_ino, (void));

_PROTOTYPE(extern void free_lproc, (struct lproc *proc));

_PROTOTYPE(extern void gather_proc_info, (void));

_PROTOTYPE(extern char *gethostnm, (unsigned char *inet_addr, int addr_family));

# if    !defined(GET_MAX_FD)
/*
 * This is not strictly a prototype, but GET_MAX_FD is the name of the
 * function that, in lieu of getdtablesize(), returns the maximum file
 * descriptor plus one (or file descriptor count).  GET_MAX_FD may be
 * defined in the dialect's machine.h.  If it is not, the following
 * selects getdtablesize().
 */

#define    GET_MAX_FD    getdtablesize
# endif    /* !defined(GET_MAX_FD) */

_PROTOTYPE(extern int hashbyname, (char *name, int mod));

_PROTOTYPE(extern void hashSfile, (void));

_PROTOTYPE(extern void initialize, (void));

_PROTOTYPE(extern int is_cmd_excl, (char *cmd, short *pss, short *sf));

_PROTOTYPE(extern int is_nw_addr, (unsigned char *inet_addr, int p, int af));

#if    defined(HASTASKS)
_PROTOTYPE(extern int is_proc_excl,(int pid, int pgid, UID_ARG uid, short *pss, short *sf, int tid));
#else	/* !defined(HASTASKS) */

_PROTOTYPE(extern int is_proc_excl, (int pid, int pgid, UID_ARG uid, short *pss, short *sf));

#endif    /* defined(HASTASKS) */

_PROTOTYPE(extern int is_readable, (char *path, int msg));

_PROTOTYPE(extern int kread, (KA_T
        addr,
        char *buf, READLEN_T
        len));

_PROTOTYPE(extern void link_lfile, (void));

_PROTOTYPE(extern struct l_dev *lkupdev, (dev_t * dev, dev_t * rdev,
        int i,
        int r));

_PROTOTYPE(extern int main, (int argc, char *argv[]));

_PROTOTYPE(extern int lstatsafely, (char *path, struct stat *buf));

_PROTOTYPE(extern char *mkstrcpy, (char *src, MALLOC_S *rlp));

_PROTOTYPE(extern char *mkstrcat, (char *str1, int len1, char *str2, int len2, char *str3, int len3, MALLOC_S *clp));

_PROTOTYPE(extern int printdevname, (dev_t * dev, dev_t * rdev,
        int force,
        int nty));

_PROTOTYPE(extern void print_file, (void));

_PROTOTYPE(extern void print_init, (void));

_PROTOTYPE(extern void printname, (int newline));

_PROTOTYPE(extern char *print_kptr, (KA_T
        kp,
        char *buf, size_t
        bufl));

_PROTOTYPE(extern int print_proc, (void));

_PROTOTYPE(extern void printrawaddr, (struct sockaddr *sock_addr));

_PROTOTYPE(extern void print_tcptpi, (int newline));

_PROTOTYPE(extern char *printuid, (UID_ARG
        uid,
        int *type));

_PROTOTYPE(extern void printunkaf, (int fam, int type));

_PROTOTYPE(extern char *printsockty, (int type));

_PROTOTYPE(extern void process_file, (KA_T
        fp));

_PROTOTYPE(extern void process_node, (KA_T
        f));

_PROTOTYPE(extern char *Readlink, (char *arg));

_PROTOTYPE(extern void readdev, (int skip));

_PROTOTYPE(extern struct mounts *readmnt, (void));

_PROTOTYPE(extern void rereaddev, (void));

_PROTOTYPE(extern int safestrlen, (char *str, int flags));

_PROTOTYPE(extern void safestrprtn, (char *str, int len, FILE *stream, int flags));

_PROTOTYPE(extern void safestrprt, (char *str, FILE *stream, int flags));

_PROTOTYPE(extern int statsafely, (char *path, struct stat *buf));

_PROTOTYPE(extern void stkdir, (char *p));

_PROTOTYPE(extern void usage, (int xv, int fh, int version));

_PROTOTYPE(extern int util_strftime, (char *fmtr, int fmtl, char *fmt));

_PROTOTYPE(extern int vfy_dev, (struct l_dev *dp));

_PROTOTYPE(extern char *x2dev, (char *hex_str, dev_t *dev_ptr));

# if    defined(HASBLKDEV)
_PROTOTYPE(extern void find_bl_ino,(void));
_PROTOTYPE(extern struct l_dev *lkupbdev,(dev_t *dev,dev_t *rdev,int i,int r));
_PROTOTYPE(extern int printbdevname,(dev_t *dev, dev_t *rdev, int f));
# endif    /* defined(HASBLKDEV) */

# if    defined(HASCDRNODE)
_PROTOTYPE(extern int readcdrnode,(KA_T ca, struct cdrnode *c));
# endif    /* defined(HASCDRNODE) */

# if    defined(HASDCACHE)
_PROTOTYPE(extern void alloc_dcache,(void));
_PROTOTYPE(extern void crc,(char *b, int l, unsigned *s));
_PROTOTYPE(extern void crdbld,(void));
_PROTOTYPE(extern int ctrl_dcache,(char *ctrl_char));
_PROTOTYPE(extern int dcpath,(int read_write, int npw));
_PROTOTYPE(extern int open_dcache,(int m, int r, struct stat *sb));
_PROTOTYPE(extern int read_dcache,(void));
_PROTOTYPE(extern int wr2DCfd,(char *buf, unsigned *cnt));
_PROTOTYPE(extern void write_dcache,(void));
# endif    /* defined(HASDCACHE) */

# if    defined(HASFIFONODE)
_PROTOTYPE(extern int readfifonode,(KA_T fa, struct fifonode *f));
# endif    /* defined(HASFIFONODE) */

# if    defined(HASFSTRUCT)
_PROTOTYPE(extern char *print_fflags,(long ffg, long pof));
# endif    /* defined(HASFSTRUCT) */

# if    defined(HASGNODE)
_PROTOTYPE(extern int readgnode,(KA_T ga, struct gnode *g));
# endif    /* defined(HASGNODE) */

# if    defined(HASKQUEUE)
_PROTOTYPE(extern void process_kqueue,(KA_T ka));
# endif    /* defined(HASKQUEUE) */

# if    defined(HASHSNODE)
_PROTOTYPE(extern int readhsnode,(KA_T ha, struct hsnode *h));
# endif    /* defined(HASHSNODE) */

# if    defined(HASINODE)
_PROTOTYPE(extern int readinode,(KA_T ia, struct inode *i));
# endif    /* defined(HASINODE) */

# if    defined(HASNCACHE)
_PROTOTYPE(extern void ncache_load,(void));
_PROTOTYPE(extern char *ncache_lookup,(char *buf, int blen, int *fp));
# endif    /* defined(HASNCACHE) */

# if    defined(HASNLIST)
_PROTOTYPE(extern void build_Nl,(struct drive_Nl *d));
_PROTOTYPE(extern int get_Nl_value,(char *nn, struct drive_Nl *d, KA_T *value));
# endif    /* defined(HASNLIST) */

# if    defined(HASPIPENODE)
_PROTOTYPE(extern int readpipenode,(KA_T pa, struct pipenode *p));
# endif    /* defined(HASPIPENODE) */

# if    defined(HASPRINTDEV)
_PROTOTYPE(extern char *HASPRINTDEV,(struct lfile *lf, dev_t *dev));
# endif    /* defined(HASPRINTDEV) */

# if    defined(HASPRINTINO)
_PROTOTYPE(extern char *HASPRINTINO,(struct lfile *lf));
# endif    /* defined(HASPRINTINO) */

# if    defined(HASPRINTNM)
_PROTOTYPE(extern void HASPRINTNM,(struct lfile *lf));
# endif    /* defined(HASPRINTNM) */

# if    defined(HASPRINTOFF)
_PROTOTYPE(extern char *HASPRINTOFF,(struct lfile *lf, int ty));
# endif    /* defined(HASPRINTOFF) */

# if    defined(HASPRINTSZ)
_PROTOTYPE(extern char *HASPRINTSZ,(struct lfile *lf));
# endif    /* defined(HASPRINTSZ) */

# if    defined(HASPRIVNMCACHE)
_PROTOTYPE(extern int HASPRIVNMCACHE,(struct lfile *lf));
# endif    /* defined(HASPRIVNMCACHE) */

# if    !defined(HASPRIVPRIPP)

_PROTOTYPE(extern void printiproto, (int proto));

# endif    /* !defined(HASPRIVPRIPP) */

# if    defined(HASRNODE)
_PROTOTYPE(extern int readrnode,(KA_T ra, struct rnode *r));
# endif    /* defined(HASRNODE) */

# if    defined(HASSPECDEVD)
_PROTOTYPE(extern void HASSPECDEVD,(char *p, struct stat *s));
# endif    /* defined(HASSPECDEVD) */

# if    defined(HASSNODE)
_PROTOTYPE(extern int readsnode,(KA_T sa, struct snode *s));
# endif    /* defined(HASSNODE) */

# if    defined(HASSTREAMS)
_PROTOTYPE(extern int readstdata,(KA_T addr, struct stdata *buf));
_PROTOTYPE(extern int readsthead,(KA_T addr, struct queue *buf));
_PROTOTYPE(extern int readstidnm,(KA_T addr, char *buf, READLEN_T len));
_PROTOTYPE(extern int readstmin,(KA_T addr, struct module_info *buf));
_PROTOTYPE(extern int readstqinit,(KA_T addr, struct qinit *buf));
# endif    /* defined(HASSTREAMS) */

# if    defined(HASTMPNODE)
_PROTOTYPE(extern int readtnode,(KA_T ta, struct tmpnode *t));
# endif    /* defined(HASTMPNODE) */

# if    defined(HASVNODE)
_PROTOTYPE(extern int readvnode,(KA_T va, struct vnode *v));
# endif    /* defined(HASVNODE) */

# if    defined(USE_LIB_SNPF)
_PROTOTYPE(extern int snpf,(char *str, int len, char *fmt, ...));
# endif    /* defined(USE_LIB_SNPF) */

# endif    /* !defined(PROTO_H) */

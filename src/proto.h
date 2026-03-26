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

#ifndef PROTO_H
#define PROTO_H

#if defined(__GNUC__) || defined(__clang__)
#define exiting __attribute__((__noreturn__))
#else
#define exiting
#endif

extern void add_nma(char *str, int len);

extern void alloc_lfile(char *nm, int num);

extern void alloc_lproc(int pid, int pgid, int ppid, UID_ARG uid, char *cmd, int pss, int sel_flags);

extern void build_IPstates(void);

extern void childx(void);

extern int ck_fd_status(char *nm, int num);

extern int ck_file_arg(int first_arg_idx, int arg_count, char *av[], int fs_value, int readlink_status, struct stat *sbp);

extern void ckkv(char *dialect, char *expected_rev, char *expected_ver, char *expected_arch);

extern void clr_devtab(void);

extern int compdev(COMP_P * lhs, COMP_P * rhs);

extern int comppid(COMP_P * lhs, COMP_P * rhs);

#if defined(WILLDROPGID)
extern void dropgid(void);
#endif

extern char *endnm(size_t * remaining_size);

extern int enter_cmd_rx(char *regex_str);

extern void enter_dev_ch(char *m);

extern int enter_dir(char *dir_path, int descend);

#if defined(HASEOPT)
extern int enter_efsys(char *path, int rdlnk);
#endif

extern int enter_fd(char *fd_str);

extern int enter_network_address(char *na);

extern int enter_id(enum IDType type, char *id_str);

extern void enter_IPstate(char *type, char *name, int nr);

extern void enter_nm(char *m);

#if defined(HASTCPUDPSTATE)
extern int enter_state_spec(char *ss);
#endif

extern int enter_str_lst(char *opt, char *str_val, struct str_lst **list_ptr,
        int *incl, int *excl);

extern int enter_uid(char *us);

extern void ent_inaddr(unsigned char *local_addr, int lp, unsigned char *foreign_addr, int fp, int af);

extern int examine_lproc(void);

extern void Exit(int xv) exiting;

extern void find_ch_ino(void);

extern void free_lproc(struct lproc *proc);

extern void gather_proc_info(void);

extern char *gethostnm(unsigned char *inet_addr, int addr_family);

#ifndef GET_MAX_FD/*
 * This is not strictly a prototype, but GET_MAX_FD is the name of the
 * function that, in lieu of getdtablesize(), returns the maximum file
 * descriptor plus one (or file descriptor count).  GET_MAX_FD may be
 * defined in the dialect's machine.h.  If it is not, the following
 * selects getdtablesize().
 */

#define    GET_MAX_FD    getdtablesize
#endif

extern int hashbyname(char *name, int mod);

extern void hashSfile(void);

extern void initialize(void);

extern int is_cmd_excl(char *cmd, short *pss, short *sf);

extern int is_nw_addr(unsigned char *inet_addr, int p, int af);

#if defined(HASTASKS)
extern int is_proc_excl(int pid, int pgid, UID_ARG uid, short *pss, short *sf, int tid);
#else

extern int is_proc_excl(int pid, int pgid, UID_ARG uid, short *pss, short *sf);

#endif

extern int is_readable(char *path, int msg);

extern int kread(KA_T addr, char *buf, READLEN_T len);

extern void link_lfile(void);

extern struct l_dev *lkupdev(dev_t *dev, dev_t *rdev, int i, int r);

extern int main(int argc, char *argv[]);

extern int lstatsafely(char *path, struct stat *buf);

extern char *mkstrcpy(char *src, MALLOC_S *rlp);

extern char *mkstrcat(char *str1, int len1, char *str2, int len2, char *str3, int len3, MALLOC_S *clp);

extern int printdevname(dev_t *dev, dev_t *rdev, int force, int nty);

extern void print_file(void);

extern void print_init(void);

extern void printname(int newline);

extern char *print_kptr(KA_T kp, char *buf, size_t bufl);

extern int print_proc(void);

extern void printrawaddr(struct sockaddr *sock_addr);

extern void print_tcptpi(int newline);

extern char *printuid(UID_ARG uid, int *type);

extern void printunkaf(int fam, int type);

extern char *printsockty(int type);

extern void process_file(KA_T fp);

extern void process_node(KA_T f);

extern char *Readlink(char *arg);

extern void readdev(int skip);

extern struct mounts *readmnt(void);

extern void rereaddev(void);

extern int safestrlen(char *str, int flags);

extern void safestrprtn(char *str, int len, FILE *stream, int flags);

extern void safestrprt(char *str, FILE *stream, int flags);

extern int statsafely(char *path, struct stat *buf);

extern void stkdir(char *p);

extern void usage(int xv, int fh, int version);

extern int util_strftime(char *fmtr, int fmtl, char *fmt);

extern int vfy_dev(struct l_dev *dp);

extern char *x2dev(char *hex_str, dev_t *dev_ptr);

#if defined(HASBLKDEV)
extern void find_bl_ino(void);
extern struct l_dev *lkupbdev(dev_t *dev,dev_t *rdev,int i,int r);
extern int printbdevname(dev_t *dev, dev_t *rdev, int f);
#endif

#if defined(HASCDRNODE)
extern int readcdrnode(KA_T ca, struct cdrnode *c);
#endif

#if defined(HASDCACHE)
extern void alloc_dcache(void);
extern void crc(char *b, int l, unsigned *s);
extern void crdbld(void);
extern int ctrl_dcache(char *ctrl_char);
extern int dcpath(int read_write, int npw);
extern int open_dcache(int m, int r, struct stat *sb);
extern int read_dcache(void);
extern int wr2DCfd(char *buf, unsigned *cnt);
extern void write_dcache(void);
#endif

#if defined(HASFIFONODE)
extern int readfifonode(KA_T fa, struct fifonode *f);
#endif

#if defined(HASFSTRUCT)
extern char *print_fflags(long ffg, long pof);
#endif

#if defined(HASGNODE)
extern int readgnode(KA_T ga, struct gnode *g);
#endif

#if defined(HASKQUEUE)
extern void process_kqueue(KA_T ka);
#endif

#if defined(HASHSNODE)
extern int readhsnode(KA_T ha, struct hsnode *h);
#endif

#if defined(HASINODE)
extern int readinode(KA_T ia, struct inode *i);
#endif

#if defined(HASNCACHE)
extern void ncache_load(void);
extern char *ncache_lookup(char *buf, int blen, int *fp);
#endif

#if defined(HASNLIST)
extern void build_Nl(struct drive_Nl *d);
extern int get_Nl_value(char *nn, struct drive_Nl *d, KA_T *value);
#endif

#if defined(HASPIPENODE)
extern int readpipenode(KA_T pa, struct pipenode *p);
#endif

#if defined(HASPRINTDEV)
extern char *HASPRINTDEV(struct lfile *lf, dev_t *dev);
#endif

#if defined(HASPRINTINO)
extern char *HASPRINTINO(struct lfile *lf);
#endif

#if defined(HASPRINTNM)
extern void HASPRINTNM(struct lfile *lf);
#endif

#if defined(HASPRINTOFF)
extern char *HASPRINTOFF(struct lfile *lf, int ty);
#endif

#if defined(HASPRINTSZ)
extern char *HASPRINTSZ(struct lfile *lf);
#endif

#if defined(HASPRIVNMCACHE)
extern int HASPRIVNMCACHE(struct lfile *lf);
#endif

#ifndef HASPRIVPRIPP
extern void printiproto(int proto);

#endif

#if defined(HASRNODE)
extern int readrnode(KA_T ra, struct rnode *r);
#endif

#if defined(HASSPECDEVD)
extern void HASSPECDEVD(char *p, struct stat *s);
#endif

#if defined(HASSNODE)
extern int readsnode(KA_T sa, struct snode *s);
#endif

#if defined(HASSTREAMS)
extern int readstdata(KA_T addr, struct stdata *buf);
extern int readsthead(KA_T addr, struct queue *buf);
extern int readstidnm(KA_T addr, char *buf, READLEN_T len);
extern int readstmin(KA_T addr, struct module_info *buf);
extern int readstqinit(KA_T addr, struct qinit *buf);
#endif

#if defined(HASTMPNODE)
extern int readtnode(KA_T ta, struct tmpnode *t);
#endif

#if defined(HASVNODE)
extern int readvnode(KA_T va, struct vnode *v);
#endif

#if defined(USE_LIB_SNPF)
extern int snpf(char *str, int len, char *fmt, ...);
#endif

#endif

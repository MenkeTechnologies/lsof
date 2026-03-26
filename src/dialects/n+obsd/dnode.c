/*
 * dnode.c - NetBSD and OpenBSD node functions for lsof
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

#if defined(HAS_DINODE_U)
#define DINODE_U dinode_u
#else /* !defined(HAS_DINODE_U) */
#define DINODE_U i_din
#endif /* defined(HAS_DINODE_U) */

#if defined(HASFDESCFS) && HASFDESCFS == 1
_PROTOTYPE(static int lkup_dev_tty, (dev_t * dr, INODETYPE *ir));
#endif /* defined(HASFDESCFS) && HASFDESCFS==1 */

#if defined(HAS_UM_UFS)
#define UFS1 UM_UFS1
#define UFS2 UM_UFS2
#endif /* defined(HAS_UM_UFS) */

#if defined(HASPROCFS)
_PROTOTYPE(static void getmemsz, (pid_t pid));

#if !defined(PGSHIFT)
#define PGSHIFT pgshift
#endif /* !defined(PGSHIFT) */

/*
 * getmemsz() - get memory size of a /proc/<n>/mem entry
 */

static void getmemsz(pid_t pid) {
    int n;
    struct vmspace vm;

#if defined(HASKVMGETPROC2)
    struct kinfo_proc2 *p;
#else  /* !defined(HASKVMGETPROC2) */
    struct kinfo_proc *p;
#endif /* defined(HASKVMGETPROC2) */

    for (n = 0, p = P; n < Np; n++, p++) {
        if (p->P_PID == pid) {
            if (!p->P_VMSPACE || kread((KA_T)p->P_VMSPACE, (char *)&vm, sizeof(vm)))
                return;
#if defined(OPENBSDV)
            CurrentLocalFile->sz =
                (SZOFFTYPE)((vm.vm_tsize + vm.vm_dsize + vm.vm_ssize) * sysconf(_SC_PAGESIZE));
#else  /* !defined(OPENBSDV */
            CurrentLocalFile->sz = (SZOFFTYPE)ctob(vm.vm_tsize + vm.vm_dsize + vm.vm_ssize);
#endif /* defined(OPENBSDV) */

            CurrentLocalFile->sz_def = 1;
            return;
        }
    }
}
#undef PGSHIFT
#endif /* defined(HASPROCFS) */

#if defined(HASFDESCFS) && HASFDESCFS == 1
/*
 * lkup_dev_tty() - look up /dev/tty
 */

static int lkup_dev_tty(dev_t *dr, INODETYPE *ir) {
    int i;

    readdev(0);

    for (;;) {
        for (i = 0; i < NumDevices; i++) {
            if (strcmp(DeviceTable[i].name, "/dev/tty") == 0) {

#if defined(HASDCACHE)
                if (DevCacheUnsafe && !DeviceTable[i].v && !vfy_dev(&DeviceTable[i]))
                    break;
#endif /* defined(HASDCACHE) */

                *dr = DeviceTable[i].rdev;
                *ir = DeviceTable[i].inode;
                return (1);
            }
        }

#if defined(HASDCACHE)
        if (i < NumDevices)
            continue;
        if (DevCacheUnsafe) {
            (void)rereaddev();
            continue;
        }
#endif /* defined(HASDCACHE) */

        return (-1);
    }
}
#endif /* defined(HASFDESCFS) && HASFDESCFS==1 */

#if defined(HASKQUEUE)
/*
 * process_kqueue() -- process kqueue file
 *
 * Strictly speaking this function should appear in dfile.c, because it is
 * a file processing function.  However, the Net and Open BSD sources don't
 * require a dfile.c, so this is the next best location for the function.
 */

void process_kqueue(KA_T ka) {

#if defined(OPENBSDV)
    struct kqueue kq; /* kqueue structure */
#endif                /* defined(OPENBSDV) */

    (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "KQUEUE");
    enter_dev_ch(print_kptr(ka, (char *)NULL, 0));

#if defined(OPENBSDV)
    if (!ka || kread(ka, (char *)&kq, sizeof(kq)))
        return;
    (void)snpf(NameChars, NameCharsLength, "count=%d, state=%#x", kq.kq_count, kq.kq_state);
    enter_nm(NameChars);
#endif /* defined(OPENBSDV) */
}
#endif /* defined(HASKQUEUE) */

/*
 * process_node() - process vnode
 */

void process_node(KA_T va) {
    dev_t dev, rdev;
    unsigned char devs;
    unsigned char lt;
    unsigned char ns;
    unsigned char rdevs;
    char *ep, *ty;
    struct lockf lf, *lff, *lfp;
    struct inode i;
    struct mfsnode m;
    struct nfsnode n;
    enum nodetype {
        NONODE,
        CDFSNODE,
        DOSNODE,
        EXT2NODE,
        FDESCNODE,
        INODE,
        KERNFSNODE,
        MFSNODE,
        NFSNODE,
        PFSNODE,
        PTYFSNODE
    } nty;
    enum vtype type;
    struct vnode *v, vb;
    struct l_vfs *vfs;

#if defined(HAS9660FS)
    dev_t iso_dev;
    INODETYPE iso_ino;
    long iso_nlink;
    int iso_stat;
    SZOFFTYPE iso_sz;
#endif /* defined(HAS9660FS) */

#if defined(HASFDESCFS)
    struct fdescnode f;

#if HASFDESCFS == 1
    static dev_t f_tty_dev;
    static INODETYPE f_tty_ino;
    static int f_tty_s = 0;
#endif /* HASFDESCFS==1 */

#endif /* defined(HASFDESCFS) */

#if defined(HASEXT2FS)
#if defined(HASI_E2FS_PTR)
    struct ext2fs_dinode ed;
#endif /* defined(HASI_E2FS_PTR) */
    struct ext2fs_dinode *edp = (struct ext2fs_dinode *)NULL;
#endif /* defined(HASEXT2FS) */

#if defined(HASI_FFS1)
    unsigned char ffs = 0;
    unsigned char u1s = 0;
    unsigned char u2s = 0;
    struct ufs1_dinode u1;
    struct ufs2_dinode u2;
    struct ufsmount um;
#endif /* defined(HASI_FFS1) */

#if defined(HASKERNFS)
    struct kernfs_node kn;
    struct stat ksb;
    int ksbs;
    struct kern_target kt;
    int ktnl;
    char ktnm[MAXPATHLEN + 1];
#endif /* defined(HASKERNFS) */

#if defined(HASMSDOSFS)
    struct denode d;
    u_long dpb;
    INODETYPE nn;
    struct msdosfsmount pm;
#endif /* defined(HASMSDOSFS) */

#if defined(HASNFSVATTRP)
    struct vattr nv;
#define NVATTR nv
#else /* !defined(HASNFSVATTRP) */
#define NVATTR n.n_vattr
#endif /* defined(HASNFSVATTRP) */

#if defined(HASNULLFS)
    struct null_node nu;
    int sc = 0;
    struct l_vfs *nvfs = (struct l_vfs *)NULL;
#endif /* defined(HASNULLFS) */

#if defined(HASPROCFS)
    struct pfsnode p;
    struct procfsid *pfi;
    size_t sz;
#endif /* defined(HASPROCFS) */

#if defined(HASPTYFS)
    struct ptyfsnode pt;
    struct specinfo si;
#endif /* defined(HASPTYFS) */

#if defined(HASNULLFS)
    for (;;) {

    if (++sc > 1024) {
        (void)snpf(NameChars, NameCharsLength, "too many overlaid nodes");
        enter_nm(NameChars);
        return;
    }
#endif /* defined(HASNULLFS) */

    /*
 * Initialize miscellaneous variables.  This is done so that processing an
 * overlaid node will be a fresh start.
 */
    devs = rdevs = 0;
    nty = NONODE;
    NameChars[0] = '\0';

#if defined(HAS9660FS)
    iso_stat = 0;
#endif /* defined(HAS9660FS) */

#if defined(HASKERNFS)
    ksbs = 0;
#endif /* defined(HASKERNFS) */

#if defined(HASEXT2FS)
    edp = (struct ext2fs_dinode *)NULL;
#endif /* defined(HASEXT2FS) */

#if defined(HASI_FFS1)
    ffs = u1s = u2s = 0;
#endif /* defined(HASI_FFS1) */

    /*
 * Read the vnode.
 */
    if (!va) {
        enter_nm("no vnode address");
        return;
    }
    v = &vb;
    if (readvnode(va, v)) {
        enter_nm(NameChars);
        return;
    }

#if defined(HASNCACHE)
    CurrentLocalFile->node_addr = va;
#if defined(HASNCVPID)
    CurrentLocalFile->cap_id = v->v_id;
#endif /* defined(HASNCVPID) */
#endif /* defined(HASNCACHE) */

#if defined(HASFSTRUCT)
    CurrentLocalFile->fna = va;
    CurrentLocalFile->fsv |= FSV_NODE_ID;
#endif /* defined(HASFSTRUCT) */

    /*
 * Get the vnode type.
 */
    if (!v->v_mount)
        vfs = (struct l_vfs *)NULL;
    else {
        vfs = readvfs((KA_T)v->v_mount);
        if (vfs) {
            if (strcmp(vfs->type, MOUNT_NFS) == 0)
                NodeType = N_NFS;

#if defined(HASKERNFS)
            else if (strcmp(vfs->type, MOUNT_KERNFS) == 0)
                NodeType = N_KERN;
#endif /* defined(HASKERNFS) */

#if defined(HASPROCFS)
            else if (strcmp(vfs->type, MOUNT_PROCFS) == 0)
                NodeType = N_PROC;
#endif /* defined(HASPROCFS) */

#if defined(HAS9660FS)
            else if (strcmp(vfs->type, MOUNT_CD9660) == 0)
                NodeType = N_CDFS;
#endif /* defined(HAS9660FS) */
        }
    }
    if (NodeType == N_REGLR) {
        switch (v->v_type) {
        case VFIFO:
            NodeType = N_FIFO;
            break;
        }
    }
    /*
 * Read the successor node.
 */
    switch (v->v_tag) {

#if defined(HAS9660FS)
    case VT_ISOFS:
        if (read_iso_node(v, &iso_dev, &iso_ino, &iso_nlink, &iso_sz)) {
            (void)snpf(NameChars, NameCharsLength, "can't read iso_node at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        iso_stat = 1;
        nty = CDFSNODE;
        break;
#endif /* defined(HAS9660FS) */

#if defined(HASFDESCFS)
    case VT_FDESC:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&f, sizeof(f))) {
            (void)snpf(NameChars, NameCharsLength, "can't read fdescnode at: %x",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        nty = FDESCNODE;
        break;
#endif /* defined(HASFDESCFS) */

#if defined(HASKERNFS)
    case VT_KERNFS:

        /*
         * Read the kernfs_node.
         */
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&kn, sizeof(kn))) {
            if (v->v_type != VDIR || !(v->v_flag && VROOT)) {
                (void)snpf(NameChars, NameCharsLength, "can't read kernfs_node at: %s",
                           print_kptr((KA_T)v->v_data, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
            } else
                kn.kf_kt = (struct kern_target *)NULL;
        }
        /*
         * Generate the /kern file name by reading the kern_target to which
         * the kernfs_node points.
         */
        if (kn.kf_kt && kread((KA_T)kn.kf_kt, (char *)&kt, sizeof(kt)) == 0 &&
            (ktnl = (int)kt.kt_namlen) > 0 && kt.kt_name) {
            if (ktnl > (sizeof(ktnm) - 1))
                ktnl = sizeof(ktnm) - 1;
            if (!kread((KA_T)kt.kt_name, ktnm, ktnl)) {
                ktnm[ktnl] = 0;
                ktnl = strlen(ktnm);
                if (ktnl > (MAXPATHLEN - strlen(_PATH_KERNFS) - 2)) {
                    ktnl = MAXPATHLEN - strlen(_PATH_KERNFS) - 2;
                    ktnm[ktnl] = '\0';
                }
                (void)snpf(NameChars, NameCharsLength, "%s/%s", _PATH_KERNFS, ktnm);
            }
        }
        /*
         * If this is the /kern root directory, its name, inode number and
         * size are fixed; otherwise, safely stat() the file to get the
         * inode number and size.
         */
        if (v->v_type == VDIR && (v->v_flag & VROOT)) {
            (void)snpf(NameChars, NameCharsLength, "%s", _PATH_KERNFS);
            ksb.st_ino = (ino_t)2;
            ksb.st_size = DEV_BSIZE;
            ksbs = 1;
        } else if (NameChars[0] && statsafely(NameChars, &ksb) == 0)
            ksbs = 1;
        nty = KERNFSNODE;
        break;
#endif /* defined(HASKERNFS) */

    case VT_MFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&m, sizeof(m))) {
            (void)snpf(NameChars, NameCharsLength, "can't read mfsnode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        nty = MFSNODE;
        break;

#if defined(HASMSDOSFS)
    case VT_MSDOSFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&d, sizeof(d))) {
            (void)snpf(NameChars, NameCharsLength, "can't read denode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        nty = DOSNODE;
        break;
#endif /* defined(HASMSDOSFS) */

    case VT_NFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&n, sizeof(n))) {
            (void)snpf(NameChars, NameCharsLength, "can't read nfsnode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }

#if defined(HASNFSVATTRP)
        if (!n.n_vattr || kread((KA_T)n.n_vattr, (char *)&nv, sizeof(nv))) {
            (void)snpf(NameChars, NameCharsLength, "can't read n_vattr at: %x",
                       print_kptr((KA_T)n.n_vattr, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
#endif /* defined(HASNFSVATTRP) */

        nty = NFSNODE;
        break;

#if defined(HASNULLFS)
    case VT_NULL:
        if ((sc == 1) && vfs)
            nvfs = vfs;
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&nu, sizeof(nu))) {
            (void)snpf(NameChars, NameCharsLength, "can't read null_node at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        if (!nu.null_lowervp) {
            (void)snpf(NameChars, NameCharsLength, "null_node overlays nothing");
            enter_nm(NameChars);
            return;
        }
        va = (KA_T)nu.null_lowervp;
        continue;
#endif /* defined(HASNULLFS) */

#if defined(HASPROCFS)
    case VT_PROCFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&p, sizeof(p))) {
            (void)snpf(NameChars, NameCharsLength, "can't read pfsnode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        nty = PFSNODE;
        break;
#endif /* defined(HASPROCFS) */

#if defined(HASPTYFS)
    case VT_PTYFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&pt, sizeof(pt))) {
            (void)snpf(NameChars, NameCharsLength, "can't read ptyfsnode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        nty = PTYFSNODE;
        break;
#endif /* defined(HASPTYFS) */

#if defined(HASEXT2FS)
    case VT_EXT2FS:
#endif /* defined(HASEXT2FS) */

#if defined(HASLFS)
    case VT_LFS:
#endif /* defined(HASLFS) */

    case VT_UFS:
        if (!v->v_data || kread((KA_T)v->v_data, (char *)&i, sizeof(i))) {
            (void)snpf(NameChars, NameCharsLength, "can't read inode at: %s",
                       print_kptr((KA_T)v->v_data, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }

#if defined(HASEXT2FS)
        if (v->v_tag == VT_EXT2FS) {
            nty = EXT2NODE;

#if defined(HASI_E2FS_PTR)
            if (i.DINODE_U.e2fs_din && !kread((KA_T)i.DINODE_U.e2fs_din, (char *)&ed, sizeof(ed)))
                edp = &ed;
#else /* !defined(HASI_E2FS_PTR) */
#if HASEXT2FS < 2
            edp = &i.DINODE_U.e2fs_din;
#else  /* HASEXT2FS>=2 */
            edp = &i.i_e2din;
#endif /* HASEXT2FS>=2 */
#endif /* defined(HASI_E2FS_PTR) */

        } else
#endif /* defined(HASEXT2FS) */

        {
            nty = INODE;

#if defined(HASI_FFS1)
            /*
                 * If there are multiple FFS's, read the relevant structures.
                 */
            if (i.i_ump && !kread((KA_T)i.i_ump, (char *)&um, sizeof(um))) {
                if (um.um_fstype == UFS1) {
                    ffs = 1;
                    if (i.DINODE_U.ffs1_din &&
                        !kread((KA_T)i.DINODE_U.ffs1_din, (char *)&u1, sizeof(u1))) {
                        u1s = 1;
                    }
                } else if (um.um_fstype == UFS2) {
                    ffs = 2;
                    if (i.DINODE_U.ffs2_din &&
                        !kread((KA_T)i.DINODE_U.ffs2_din, (char *)&u2, sizeof(u2))) {
                        u2s = 1;
                    }
                }
            }
#endif /* defined(HASI_FFS1) */
        }

        if ((lff = i.i_lockf)) {

            /*
                 * Determine the lock state.
                 */
            lfp = lff;
            do {
                if (kread((KA_T)lfp, (char *)&lf, sizeof(lf)))
                    break;
                lt = 0;
                switch (lf.lf_flags & (F_FLOCK | F_POSIX)) {
                case F_FLOCK:
                    if (Cfp && (struct file *)lf.lf_id == Cfp)
                        lt = 1;
                    break;
                case F_POSIX:
                    if ((KA_T)lf.lf_id == Kpa)
                        lt = 1;

#if defined(HAS_LWP_H) && !defined(HAS_LF_LWP)
                    else {

                        struct lwp lw;

                        if (!kread((KA_T)lf.lf_id, (char *)&lw, sizeof(lw)) &&
                            (KA_T)lw.l_proc == Kpa)
                            lt = 1;
                    }
#endif /* defined(HAS_LWP_H) && !defined(HAS_LF_LWP) */

                    break;
                }
                if (!lt)
                    continue;
                if (lf.lf_start == (off_t)0 && lf.lf_end == 0xffffffffffffffffLL)
                    lt = 1;
                else
                    lt = 0;
                if (lf.lf_type == F_RDLCK)
                    CurrentLocalFile->lock = lt ? 'R' : 'r';
                else if (lf.lf_type == F_WRLCK)
                    CurrentLocalFile->lock = lt ? 'W' : 'w';
                else if (lf.lf_type == (F_RDLCK | F_WRLCK))
                    CurrentLocalFile->lock = 'u';
                break;
            } while ((lfp = lf.lf_next) && lfp != lff);
        }
        break;
    default:
        if (v->v_type == VBAD || v->v_type == VNON)
            break;
        (void)snpf(NameChars, NameCharsLength, "unknown file system type: %d", v->v_tag);
        enter_nm(NameChars);
        return;
    }
    /*
 * Get device and type for printing.
 */
    type = v->v_type;
    switch (nty) {

#if defined(HASMSDOSFS)
    case DOSNODE:
        dev = d.de_dev;
        devs = 1;
        break;
#endif /* defined(HASMSDOSFS) */

#if defined(HASFDESCFS)
    case FDESCNODE:

#if defined(HASFDLINK)
        if (f.fd_link && !kread((KA_T)f.fd_link, NameChars, NameCharsLength - 1)) {
            NameChars[NameCharsLength - 1] = '\0';
            break;
        }
#endif /* defined(HASFDLINK) */

#if HASFDESCFS == 1
        if (f.fd_type == Fctty) {
            if (f_tty_s == 0)
                f_tty_s = lkup_dev_tty(&f_tty_dev, &f_tty_ino);
            if (f_tty_s == 1) {
                dev = DeviceOfDev;
                rdev = f_tty_dev;
                CurrentLocalFile->inode = f_tty_ino;
                devs = CurrentLocalFile->inp_ty = rdevs = 1;
            }
        }
        break;
#endif /* HASFDESCFS==1 */
#endif /* defined(HASFDESCFS) */

#if defined(HASEXT2FS)
    case EXT2NODE:

        dev = i.i_dev;
        devs = 1;
        if ((type == VCHR) || (type == VBLK)) {

#if defined(HASI_E2FS_PTR)
            if (edp) {
                rdev = edp->e2di_rdev;
                rdevs = 1;
            }
#else /* !defined(HASI_E2FS_PTR) */
#if HASEXT2FS < 2
            rdev = i.DINODE_U.e2fs_din.e2di_rdev;
#else  /* HASEXT2FS>=2 */
            rdev = i.i_e2din.e2di_rdev;
#endif /* HASEXT2FS>=2 */
            rdevs = 1;
#endif /* defined(HASI_E2FS_PTR) */
        }
        break;
#endif /* defined(HASEXT2FS) */

    case INODE:
        dev = i.i_dev;
        devs = 1;
        if ((type == VCHR) || (type == VBLK)) {

#if defined(HASI_FFS)
            rdev = i.i_ffs_rdev;
            rdevs = 1;
#else /* !defined(HASI_FFS) */
#if defined(HASI_FFS1)
            if (ffs == 1) {
                if (u1s) {
                    rdev = u1.di_rdev;
                    rdevs = 1;
                }
            } else if (ffs == 2) {
                if (u2s) {
                    rdev = u2.di_rdev;
                    rdevs = 1;
                }
            }
#else  /* !defined(HASI_FFS1) */
            rdev = i.i_rdev;
            rdevs = 1;
#endif /* defined(HASI_FFS1) */
#endif /* defined(HASI_FFS) */
        }
        break;

#if defined(HASKERNFS)
    case KERNFSNODE:
        if (vfs) {

#if defined(HASSTATVFS)
            dev = (dev_t)vfs->fsid.__fsid_val[0];
#else  /* !defined(HASSTATVFS) */
            dev = (dev_t)vfs->fsid.val[0];
#endif /* defined(HASSTATVFS) */

            devs = 1;
        }
        break;
#endif /* defined(HASKERNFS) */

#if defined(HAS9660FS)
    case CDFSNODE:
        if (iso_stat) {
            dev = iso_dev;
            devs = 1;
        }
        break;
#endif /* defined(HAS9660FS) */

    case NFSNODE:
        dev = NVATTR.va_fsid;
        devs = 1;
        break;

#if defined(HASPTYFS)
    case PTYFSNODE:
        if (v->v_un.vu_specinfo && !kread((KA_T)v->v_un.vu_specinfo, (char *)&si, sizeof(si))) {
            rdev = si.si_rdev;
            rdevs = 1;
        }
        if (vfs) {

#if defined(HASSTATVFS)
            dev = (dev_t)vfs->fsid.__fsid_val[0];
#else  /* !defined(HASSTATVFS) */
            dev = (dev_t)vfs->fsid.val[0];
#endif /* defined(HASSTATVFS) */

            devs = 1;
        }
        break;
#endif /* defined(HASPTYFS) */
    }
    /*
 * Obtain the inode number.
 */
    switch (nty) {

#if defined(HASMSDOSFS)
    case DOSNODE:
        if (d.de_pmp && !kread((KA_T)d.de_pmp, (char *)&pm, sizeof(pm))) {
            dpb = (u_long)(pm.pm_BytesPerSec / sizeof(struct direntry));
            if (d.de_Attributes & ATTR_DIRECTORY) {
                if (d.de_StartCluster == MSDOSFSROOT)
                    nn = (INODETYPE)1;
                else
                    nn = (INODETYPE)(cntobn(&pm, d.de_StartCluster) * dpb);
            } else {
                if (d.de_dirclust == MSDOSFSROOT)
                    nn = (INODETYPE)(roottobn(&pm, 0) * dpb);
                else
                    nn = (INODETYPE)(cntobn(&pm, d.de_dirclust) * dpb);
                nn += (INODETYPE)(d.de_diroffset / sizeof(struct direntry));
            }
            CurrentLocalFile->inode = nn;
            CurrentLocalFile->inp_ty = 1;
        }
        break;
#endif /* defined(HASMSDOSFS) */

#if defined(HASEXT2FS)
    case EXT2NODE:
#endif /* defined(HASEXT2FS) */

    case INODE:
        CurrentLocalFile->inode = (INODETYPE)i.i_number;
        CurrentLocalFile->inp_ty = 1;
        break;

#if defined(HASKERNFS)
    case KERNFSNODE:
        if (ksbs) {
            CurrentLocalFile->inode = (INODETYPE)ksb.st_ino;
            CurrentLocalFile->inp_ty = 1;
        }
        break;
#endif /* defined(HASKERNFS) */

#if defined(HAS9660FS)
    case CDFSNODE:
        if (iso_stat) {
            CurrentLocalFile->inode = iso_ino;
            CurrentLocalFile->inp_ty = 1;
        }
        break;
#endif /* defined(HAS9660FS) */

    case NFSNODE:
        CurrentLocalFile->inode = (INODETYPE)NVATTR.va_fileid;
        CurrentLocalFile->inp_ty = 1;
        break;

#if defined(HASPROCFS)
    case PFSNODE:
        CurrentLocalFile->inode = (INODETYPE)p.pfs_fileno;
        CurrentLocalFile->inp_ty = 1;
        break;
#endif /* defined(HASPROCFS) */

#if defined(HASPTYFS)
    case PTYFSNODE:
        if (pt.ptyfs_type == PTYFSptc) {
            if (pt.ptyfs_fileno > 0x3fffffff)
                CurrentLocalFile->inode = (INODETYPE)(pt.ptyfs_fileno & 0x3fffffff);
            else
                CurrentLocalFile->inode = (INODETYPE)(pt.ptyfs_fileno - 1);
        } else
            CurrentLocalFile->inode = (INODETYPE)pt.ptyfs_fileno;
        CurrentLocalFile->inp_ty = 1;
        break;
#endif /* defined(HASPTYFS) */
    }

    /*
 * Obtain the file size.
 */
    if (OptOffset)
        CurrentLocalFile->off_def = 1;
    else {
        switch (NodeType) {

#if defined(HAS9660FS)
        case N_CDFS:
            if (iso_stat) {
                CurrentLocalFile->sz = (SZOFFTYPE)iso_sz;
                CurrentLocalFile->sz_def = 1;
            }
            break;
#endif /* defined(HAS9660FS) */

        case N_FIFO:
            if (!OptSize)
                CurrentLocalFile->off_def = 1;
            break;

#if defined(HASKERNFS)
        case N_KERN:
            if (ksbs) {
                CurrentLocalFile->sz = (SZOFFTYPE)ksb.st_size;
                CurrentLocalFile->sz_def = 1;
            }
            break;
#endif /* defined(HASKERNFS) */

        case N_NFS:
            if (nty == NFSNODE) {
                CurrentLocalFile->sz = (SZOFFTYPE)NVATTR.va_size;
                CurrentLocalFile->sz_def = 1;
            }
            break;

#if defined(HASPROCFS)
        case N_PROC:
            if (nty == PFSNODE) {
                switch (p.pfs_type) {
                case Proot:
                case Pproc:
                    CurrentLocalFile->sz = (SZOFFTYPE)DEV_BSIZE;
                    CurrentLocalFile->sz_def = 1;
                    break;
                case Pcurproc:
                    CurrentLocalFile->sz = (SZOFFTYPE)DEV_BSIZE;
                    CurrentLocalFile->sz_def = 1;
                    break;
                case Pmem:
                    (void)getmemsz(p.pfs_pid);
                    break;
                case Pregs:
                    CurrentLocalFile->sz = (SZOFFTYPE)sizeof(struct reg);
                    CurrentLocalFile->sz_def = 1;
                    break;

#if defined(FP_QSIZE)
                case Pfpregs:
                    CurrentLocalFile->sz = (SZOFFTYPE)sizeof(struct fpreg);
                    CurrentLocalFile->sz_def = 1;
                    break;
#endif /* defined(FP_QSIZE) */
                }
            }
            break;
#endif /* defined(HASPROCFS) */

        case N_REGLR:
            if (type == VREG || type == VDIR) {
                switch (nty) {
                case INODE:

#if defined(HASI_FFS)

                    CurrentLocalFile->sz = (SZOFFTYPE)i.i_ffs_size;
                    CurrentLocalFile->sz_def = 1;
                    break;
#else /* !defined(HASI_FFS) */
#if defined(HASI_FFS1)

                    if (ffs == 1) {
                        if (u1s) {
                            CurrentLocalFile->sz = (SZOFFTYPE)u1.di_size;
                            CurrentLocalFile->sz_def = 1;
                        }
                    } else if (ffs == 2) {
                        if (u2s) {
                            CurrentLocalFile->sz = (SZOFFTYPE)u2.di_size;
                            CurrentLocalFile->sz_def = 1;
                        }
                    }
                    break;
#else  /* !defined(HASI_FFS1) */
                    CurrentLocalFile->sz = (SZOFFTYPE)i.i_size;
                    CurrentLocalFile->sz_def = 1;
#endif /* defined(HASI_FFS1) */
#endif /* defined(HASI_FFS) */

                    break;

#if defined(HASMSDOSFS)
                case DOSNODE:
                    CurrentLocalFile->sz = (SZOFFTYPE)d.de_FileSize;
                    CurrentLocalFile->sz_def = 1;
                    break;
#endif /* defined(HASMSDOSFS) */

                case MFSNODE:
                    CurrentLocalFile->sz = (SZOFFTYPE)m.mfs_size;
                    CurrentLocalFile->sz_def = 1;
                    break;

#if defined(HASEXT2FS)
                case EXT2NODE:
#if defined(HASI_E2FS_PTR)
                    if (edp) {
                        CurrentLocalFile->sz = (SZOFFTYPE)edp->e2di_size;
                        CurrentLocalFile->sz_def = 1;
                    }
#else  /* !defined(HASI_E2FS_PTR) */
                    CurrentLocalFile->sz = (SZOFFTYPE)i.i_e2fs_size;
                    CurrentLocalFile->sz_def = 1;
#endif /* defined(HASI_E2FS_PTR) */
                    break;
#endif /* defined(HASEXT2FS) */
                }
            } else if ((type == VCHR || type == VBLK) && !OptSize)
                CurrentLocalFile->off_def = 1;
            break;
        }
    }
    /*
 * Record the link count.
 */
    if (OptLinkCount) {
        switch (NodeType) {

#if defined(HAS9660FS)
        case N_CDFS:
            if (iso_stat) {
                CurrentLocalFile->nlink = iso_nlink;
                CurrentLocalFile->nlink_def = 1;
            }
            break;
#endif /* defined(HAS9660FS) */

#if defined(HASKERNFS)
        case N_KERN:
            if (ksbs) {
                CurrentLocalFile->nlink = (long)ksb.st_nlink;
                CurrentLocalFile->nlink_def = 1;
            }
            break;
#endif /* defined(HASKERNFS) */

        case N_NFS:
            if (nty == NFSNODE) {
                CurrentLocalFile->nlink = (long)NVATTR.va_nlink;
                CurrentLocalFile->nlink_def = 1;
            }
            break;
        case N_REGLR:
            switch (nty) {
            case INODE:

#if defined(HASEFFNLINK)
                CurrentLocalFile->nlink = (long)i.HASEFFNLINK;
#else /* !defined(HASEFFNLINK) */
#if defined(HASI_FFS)
                CurrentLocalFile->nlink = (long)i.i_ffs_nlink;
#else /* !defined(HASI_FFS) */
#if defined(HASI_FFS1)
                if (ffs == 1) {
                    if (u1s)
                        CurrentLocalFile->nlink = (long)u1.di_nlink;
                } else if (ffs == 2) {
                    if (u2s)
                        CurrentLocalFile->nlink = (long)u2.di_nlink;
                }
#else  /* !defined(HASI_FFS1) */

                CurrentLocalFile->nlink = (long)i.i_nlink;
#endif /* defined(HASI_FFS1) */
#endif /* defined(HASI_FFS) */
#endif /* defined(HASEFFNLINK) */

                CurrentLocalFile->nlink_def = 1;
                break;

#if defined(HASMSDOSFS)
            case DOSNODE:
                CurrentLocalFile->nlink = (long)d.de_refcnt;
                CurrentLocalFile->nlink_def = 1;
                break;
#endif /* defined(HASMSDOSFS) */

#if defined(HASEXT2FS)
            case EXT2NODE:
#if defined(HASI_E2FS_PTR)
                if (edp) {
                    CurrentLocalFile->nlink = (long)edp->e2di_nlink;
                    CurrentLocalFile->nlink_def = 1;
                }
#else  /* !defined(HASI_E2FS_PTR) */
                CurrentLocalFile->nlink = (long)i.i_e2fs_nlink;
                CurrentLocalFile->nlink_def = 1;
#endif /* defined(HASI_E2FS_PTR) */

                break;

#endif /* defined(HASEXT2FS) */
            }
            break;
        }
        if (CurrentLocalFile->nlink_def && LinkCountThreshold &&
            (CurrentLocalFile->nlink < LinkCountThreshold))
            CurrentLocalFile->sel_flags |= SELNLINK;
    }
    /*
 * Record an NFS file selection.
 */
    if (NodeType == N_NFS && OptNfs)
        CurrentLocalFile->sel_flags |= SELNFS;

#if defined(HASNULLFS)
    /*
     * If there is a saved nullfs vfs pointer, propagate its device number.
     */
    if (nvfs) {

#if defined(HASSTATVFS)
        dev = nvfs->fsid.__fsid_val[0];
#else  /* !defined(HASSTATVFS) */
        dev = nvfs->fsid.val[0];
#endif /* defined(HASSTATVFS) */

        devs = 1;
    }
#endif /* defined(HASNULLFS) */

    /*
 * Save the file system names.
 */
    if (vfs) {
        CurrentLocalFile->fsdir = vfs->dir;
        CurrentLocalFile->fsdev = vfs->fsname;
    }
    /*
 * Save the device numbers and their states.
 *
 * Format the vnode type, and possibly the device name.
 */
    CurrentLocalFile->dev = dev;
    CurrentLocalFile->dev_def = devs;
    CurrentLocalFile->rdev = rdev;
    CurrentLocalFile->rdev_def = rdevs;
    switch (type) {
    case VNON:
        ty = "VNON";
        break;
    case VREG:
        ty = "VREG";
        break;
    case VDIR:
        ty = "VDIR";
        break;
    case VBLK:
        ty = "VBLK";
        NodeType = N_BLK;
        break;
    case VCHR:
        ty = "VCHR";
        NodeType = N_CHR;
        break;
    case VLNK:
        ty = "VLNK";
        break;

#if defined(VSOCK)
    case VSOCK:
        ty = "SOCK";
        break;
#endif /* defined(VSOCK) */

    case VBAD:
        ty = "VBAD";
        break;
    case VFIFO:
        ty = "FIFO";
        break;
    default:
        (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "%04o", (type & 0xfff));
        ty = NULL;
    }
    if (ty)
        (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "%s", ty);
    CurrentLocalFile->ntype = NodeType;
    /*
 * Handle some special cases:
 *
 * 	ioctl(fd, TIOCNOTTY) files;
 *	/kern files
 *	memory node files;
 *	/proc files;
 *	ptyfs files.
 */

    if (type == VBAD)
        (void)snpf(NameChars, NameCharsLength, "(revoked)");
    else if (nty == MFSNODE) {
        CurrentLocalFile->dev_def = CurrentLocalFile->rdev_def = 0;
        (void)snpf(NameChars, NameCharsLength, "%#x", m.mfs_baseoff);
        enter_dev_ch("memory");
    }

#if defined(HASPROCFS)
    else if (nty == PFSNODE) {
        CurrentLocalFile->dev_def = CurrentLocalFile->rdev_def = 0;
        ty = NULL;
        (void)snpf(NameChars, NameCharsLength, "/%s", HASPROCFS);
        switch (p.pfs_type) {
        case Proot:
            ty = "PDIR";
            break;
        case Pcurproc:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/curproc");
            ty = "PCUR";
            break;
        case Pproc:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d", p.pfs_pid);
            ty = "PDIR";
            break;
        case Pfile:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/file", p.pfs_pid);
            ty = "PFIL";
            break;
        case Pmem:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/mem", p.pfs_pid);
            ty = "PMEM";
            break;
        case Pregs:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/regs", p.pfs_pid);
            ty = "PREG";
            break;
        case Pfpregs:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/fpregs", p.pfs_pid);
            ty = "PFPR";
            break;
        case Pctl:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/ctl", p.pfs_pid);
            ty = "PCTL";
            break;
        case Pstatus:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/status", p.pfs_pid);
            ty = "PSTA";
            break;
        case Pnote:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/note", p.pfs_pid);
            ty = "PNTF";
            break;
        case Pnotepg:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/notepg", p.pfs_pid);
            ty = "PGID";
            break;

#if defined(Pfd)
        case Pfd:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/fd", p.pfs_pid);
            ty = "PFD";
            break;
#endif /* defined(Pfd) */

#if defined(Pmap)
        case Pmap:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/map", p.pfs_pid);
            ty = "PMAP";
            break;
#endif /* defined(Pmap) */

#if defined(Pmaps)
        case Pmaps:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%d/maps", p.pfs_pid);
            ty = "PMPS";
            break;
#endif /* defined(Pmaps) */
        }
        if (ty)
            (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), ty);
    }
#endif /* defined(HASPROCFS) */

#if defined(HASPTYFS)
    else if (nty == PTYFSNODE) {
        (void)snpf(NameChars, NameCharsLength, "%s", CurrentLocalFile->fsdir);
        CurrentLocalFile->nlink = 1;
        CurrentLocalFile->nlink_def = 1;
        switch (pt.ptyfs_type) {
        case PTYFSpts:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%lu", (unsigned long)pt.ptyfs_pty);
            break;
        case PTYFSptc:
            ep = endnm(&sz);
            (void)snpf(ep, sz, "/%lu (master)", (unsigned long)pt.ptyfs_pty);
            break;
        case PTYFSroot:
            CurrentLocalFile->sz = 512;
            CurrentLocalFile->sz_def = 1;
            break;
        }
        if (ty)
            (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), ty);
    }
#endif /* defined(HASPTYFS) */

#if defined(HASBLKDEV)
    /*
     * If this is a VBLK file and it's missing an inode number, try to
     * supply one.
     */
    if ((CurrentLocalFile->inp_ty == 0) && (type == VBLK))
        find_bl_ino();
#endif /* defined(HASBLKDEV) */

    /*
 * If this is a VCHR file and it's missing an inode number, try to
 * supply one.
 */
    if ((CurrentLocalFile->inp_ty == 0) && (type == VCHR))
        find_ch_ino();
    /*
 * Test for specified file.
 */

#if defined(HASPROCFS)
    if (NodeType == N_PROC) {
        if (ProcFsSearching) {
            ProcFsFound = 1;
            CurrentLocalFile->sel_flags |= SELNM;
        } else if (nty == PFSNODE) {
            for (pfi = ProcFsIdTable; pfi; pfi = pfi->next) {
                if ((pfi->pid && pfi->pid == p.pfs_pid)

#if defined(HASPINODEN)
                    || ((CurrentLocalFile->inp_ty == 1) && (pfi->inode == CurrentLocalFile->inode))
#endif /* defined(HASPINODEN) */

                ) {
                    pfi->f = 1;
                    if (NameChars[0] && pfi->nm)
                        (void)snpf(NameChars, NameCharsLength, "%s", pfi->nm);
                    CurrentLocalFile->sel_flags |= SELNM;
                    break;
                }
            }
        }
    } else
#endif /* defined(HASPROCFS) */

    {
        if (NameChars[0]) {
            enter_nm(NameChars);
            ns = 1;
        } else
            ns = 0;
        if (SearchFileChain &&
            is_file_named((char *)NULL, ((type == VCHR) || (type == VBLK)) ? 1 : 0)) {
            CurrentLocalFile->sel_flags |= SELNM;
        }
        if (ns)
            NameChars[0] = '\0';
    }
    /*
 * Enter name characters.
 */
    if (NameChars[0])
        enter_nm(NameChars);

#if defined(HASNULLFS)
    break;
    } /* end for(;;) process_overlaid_node loop */
#endif /* defined(HASNULLFS) */
}

#if defined(HAS_SYS_PIPEH)
/*
 * process_pipe() - process a file structure whose type is DTYPE_PIPE
 */

void process_pipe(KA_T pa) {
    char *ep;
    struct pipe p;
    size_t sz;

    if (!pa || kread((KA_T)pa, (char *)&p, sizeof(p))) {
        (void)snpf(NameChars, NameCharsLength, "can't read DTYPE_PIPE pipe struct: %#s",
                   print_kptr(pa, (char *)NULL, 0));
        enter_nm(NameChars);
        return;
    }
    (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "PIPE");
    enter_dev_ch(print_kptr(pa, (char *)NULL, 0));
    if (OptOffset)
        CurrentLocalFile->off_def = 1;
    else {
        CurrentLocalFile->sz = (SZOFFTYPE)p.pipe_buffer.size;
        CurrentLocalFile->sz_def = 1;
    }
    if (p.pipe_peer)
        (void)snpf(NameChars, NameCharsLength, "->%s",
                   print_kptr((KA_T)p.pipe_peer, (char *)NULL, 0));
    else
        NameChars[0] = '\0';
    if (p.pipe_buffer.cnt) {
        ep = endnm(&sz);
        (void)snpf(ep, sz, ", cnt=%d", p.pipe_buffer.cnt);
    }
    if (p.pipe_buffer.in) {
        ep = endnm(&sz);
        (void)snpf(ep, sz, ", in=%d", p.pipe_buffer.in);
    }
    if (p.pipe_buffer.out) {
        ep = endnm(&sz);
        (void)snpf(ep, sz, ", out=%d", p.pipe_buffer.out);
    }
    /*
 * Enter name characters.
 */
    if (NameChars[0])
        enter_nm(NameChars);
}
#endif /* defined(HAS_SYS_PIPEH) */

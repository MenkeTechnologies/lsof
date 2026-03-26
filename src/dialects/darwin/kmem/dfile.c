/*
 * dfile.c - Darwin file processing functions for /dev/kmem-based lsof
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
 * Local definitions
 */

#if    DARWINV >= 800
#define	file		fileglob
#define	f_flag		fg_flag
#define	f_type		fg_type
#define	f_count		fg_count
#define	f_ops		fg_ops
#define	f_offset	fg_offset
#define	f_data		fg_data
#endif    /* DARWINV>=800 */

#if    defined(HASPSXSEM)
#define	PSEMNAMLEN	31		/* from kern/posix_sem.c */
#endif    /* defined(HASPSXSEM) */

#if    defined(HASPSXSHM)
#define	PSHMNAMLEN	31		/* from kern/posix_shm.c */
#endif    /* defined(HASPSXSHM) */


/*
 * Local structure definitions
 */

#if    defined(HASPSXSEM)
struct pseminfo {			/* from kern/posix_sem.c */
    unsigned int psem_flags;
    unsigned int psem_usecount;
    mode_t psem_mode;
    uid_t psem_uid;
    gid_t psem_gid;
    char psem_name[PSEMNAMLEN + 1];
    void *psem_semobject;
    struct proc *sem_proc;
};

struct psemnode {
    struct pseminfo *pinfo;
};
#endif    /* defined(HASPSXSEM) */

#if    defined(HASPSXSHM)        /* from kern/posix_shm.c */
struct pshminfo {
    unsigned int pshm_flags;
    unsigned int pshm_usecount;
    off_t pshm_length;
    mode_t pshm_mode;
    uid_t pshm_uid;
    gid_t pshm_gid;       
    char pshm_name[PSHMNAMLEN + 1];
    void *pshm_memobject;
};

struct pshmnode {
    off_t mapp_addr;

# if	DARWINV<800
    size_t map_size;
# else	/* DARWINV>=800 */
    user_size_t map_size;
# endif	/* DARWINV>=800 */

    struct pshminfo *pinfo;
};
#endif    /* defined(HASPSXSHM) */


#if    DARWINV >= 800
/*
 * print_v_path() - print vnode's path
 */

int
print_v_path(struct lfile * lf)
{
    if (lf->V_path) {
        safestrprt(lf->V_path, stdout, 0);
        return(1);
    }
    return(0);
}
#endif    /* DARWINV>=800 */


#if    defined(HASKQUEUE)
/*
 * process_kqueue() -- process kqueue file
 */

void
process_kqueue(KA_T ka)
{
    struct kqueue kq;		/* kqueue structure */

    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "KQUEUE");
    enter_dev_ch(print_kptr(ka, (char *)NULL, 0));
    if (!ka || kread(ka, (char *)&kq, sizeof(kq)))
        return;
    (void) snpf(NameChars, NameCharsLength, "count=%d, state=%#x", kq.kq_count,
        kq.kq_state);
    enter_nm(NameChars);
}
#endif    /* defined(HASKQUEUE) */


#if    DARWINV >= 800
/*
 * process_pipe() - process a file structure whose type is DTYPE_PIPE
 */

void
process_pipe(KA_T pa)
{
    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "PIPE");
    enter_dev_ch(print_kptr(pa, (char *)NULL, 0));
    NameChars[0] = '\0';
}
#endif    /* DARWINV>=800 */


#if    defined(HASPSXSEM)
/*
 * process_psxsem() -- process POSIX semaphore file
 */

void
process_psxsem(KA_T pa)
{
    struct pseminfo pi;
     struct psemnode pn;

    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "PSXSEM");
    enter_dev_ch(print_kptr(pa, (char *)NULL, 0));
    if (!OptSize)
        CurrentLocalFile->off_def = 1;
    if (pa && !kread(pa, (char *)&pn, sizeof(pn))) {
        if (pn.pinfo && !kread((KA_T)pn.pinfo, (char *)&pi, sizeof(pi))) {
        if (pi.psem_name[0]) {
            pi.psem_name[PSEMNAMLEN] = '\0';
            (void) snpf(NameChars, NameCharsLength, "%s", pi.psem_name);
            enter_nm(NameChars);
        }
        }
    }
}
#endif    /* defined(HASPSXSEM) */


#if    defined(HASPSXSHM)
/*
 * process_psxshm() -- process POSIX shared memory file
 */

void
process_psxshm(KA_T pa)
{
    struct pshminfo pi;
    struct pshmnode pn;
    int pns = 0;

    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "PSXSHM");
    enter_dev_ch(print_kptr(pa, (char *)NULL, 0));
    if (pa && !kread(pa, (char *)&pn, sizeof(pn))) {
        pns = 1;
        if (pn.pinfo && !kread((KA_T)pn.pinfo, (char *)&pi, sizeof(pi))) {
        if (pi.pshm_name[0]) {
            pi.pshm_name[PSEMNAMLEN] = '\0';
            (void) snpf(NameChars, NameCharsLength, "%s", pi.pshm_name);
            enter_nm(NameChars);
        } else if (pi.pshm_memobject) {
            (void) snpf(NameChars, NameCharsLength, "obj=%s",
            print_kptr((KA_T)pi.pshm_memobject, (char *)NULL, 0));
            enter_nm(NameChars);
        }
        }
    }
    if (OptOffset)
        CurrentLocalFile->off_def = 1;
    else if (pns) {
        CurrentLocalFile->sz = (SZOFFTYPE)pn.map_size;
        CurrentLocalFile->sz_def = 1;
    }
}
#endif    /* defined(HASPSXSHM) */


/*
 * process_file() - process file
 */

/*
 * The caller may define:
 *
 *	FILEPTR	as the name of the location to store a pointer
 *			to the current file struct -- e.g.,
 *
 *			struct file *foobar;
 *			#define FILEPTR	foobar
 */

void
process_file(KA_T fp)
{

#if    DARWINV < 800
    struct file f;
#else	/* DARWINV>=800 */
    struct fileglob f;
    struct fileproc fileproc;
#endif    /* DARWINV>=800 */

    int flag;

#if    defined(FILEPTR)
    /*
     * Save file structure address for process_node().
     */
        FILEPTR = &f;
#endif    /* defined(FILEPTR) */

/*
 * Read file structure.
 */

#if    DARWINV < 800
    if (kread((KA_T) fp, (char *) &f, sizeof(f))) {
        (void) snpf(NameChars, NameCharsLength, "can't read file struct from %s",
                    print_kptr(fp, (char *) NULL, 0));
        enter_nm(NameChars);
        return;
    }
#else	/* DARWINV>=800 */
    if (kread((KA_T)fp, (char *)&fileproc, sizeof(fileproc))) {
        (void) snpf(NameChars, NameCharsLength, "can't read fileproc struct from %s",
        print_kptr(fp, (char *)NULL, 0));
        enter_nm(NameChars);
        return;
    }
    if (kread((KA_T)fileproc.f_fglob, (char *)&f, sizeof(f))) {
        (void) snpf(NameChars, NameCharsLength, "can't read fileglob struct from %s",
        print_kptr((KA_T)fileproc.f_fglob, (char *)NULL, 0));
        enter_nm(NameChars);
        return;
    }
#endif    /* DARWINV>=800 */

    CurrentLocalFile->off = (SZOFFTYPE) f.f_offset;
    if (f.f_count) {

        /*
         * Construct access code.
         */
        if ((flag = (f.f_flag & (FREAD | FWRITE))) == FREAD)
            CurrentLocalFile->access = 'r';
        else if (flag == FWRITE)
            CurrentLocalFile->access = 'w';
        else if (flag == (FREAD | FWRITE))
            CurrentLocalFile->access = 'u';

#if    defined(HASFSTRUCT)
        /*
         * Save file structure values.
         */
            if (OptFileStructValues & FSV_FILE_COUNT) {
            CurrentLocalFile->fct = (long)f.f_count;
            CurrentLocalFile->fsv |= FSV_FILE_COUNT;
            }
            if (OptFileStructValues & FSV_FILE_ADDR) {
            CurrentLocalFile->fsa = fp;
            CurrentLocalFile->fsv |= FSV_FILE_ADDR;
            }
            if (OptFileStructValues & FSV_FILE_FLAGS) {
            CurrentLocalFile->ffg = (long)f.f_flag;
            CurrentLocalFile->fsv |= FSV_FILE_FLAGS;
            }
            if (OptFileStructValues & FSV_NODE_ID) {
            CurrentLocalFile->fna = (KA_T)f.f_data;
            CurrentLocalFile->fsv |= FSV_NODE_ID;
            }
#endif    /* defined(HASFSTRUCT) */

        /*
         * Process structure by its type.
         */
        switch (f.f_type) {


#if    defined(DTYPE_PIPE)
            case DTYPE_PIPE:
# if	defined(HASPIPEFN)
            if (!SelectInetOnly)
                HASPIPEFN((KA_T)f.f_data);
# endif	/* defined(HASPIPEFN) */
            return;
#endif    /* defined(DTYPE_PIPE) */

            case DTYPE_VNODE:
                if (!SelectInetOnly)
                    process_node((KA_T) f.f_data);
                return;
            case DTYPE_SOCKET:
                process_socket((KA_T) f.f_data);
                return;

#if    defined(HASKQUEUE)
            case DTYPE_KQUEUE:
            process_kqueue((KA_T)f.f_data);
            return;
#endif    /* defined(HASKQUEUE) */

#if    defined(HASPSXSEM)
            case DTYPE_PSXSEM:
            process_psxsem((KA_T)f.f_data);
            return;
#endif    /* defined(HASPSXSEM) */

#if    defined(HASPSXSHM)
            case DTYPE_PSXSHM:
            process_psxshm((KA_T)f.f_data);
            return;
#endif    /* defined(HASPSXSHM) */

#if    defined(HASPRIVFILETYPE)
            case PRIVFILETYPE:
            HASPRIVFILETYPE((KA_T)f.f_data);
            return;
#endif    /* defined(HASPRIVFILETYPE) */

            default:
                if (f.f_type || f.f_ops) {
                    (void) snpf(NameChars, NameCharsLength,
                                "%s file struct, ty=%#x, op=%p",
                                print_kptr(fp, (char *) NULL, 0), f.f_type, f.f_ops);
                    enter_nm(NameChars);
                    return;
                }
        }
    }
    enter_nm("no more information");
}

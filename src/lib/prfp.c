/*
 * prfp.c -- process_file() function for lsof library
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

#include "../machine.h"

#if defined(USE_LIB_PROCESS_FILE)

#include "../lsof.h"

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

void process_file(KA_T file_addr) {
    struct file file_str;
    int flag;
    char tbuf[32];

#if defined(FILEPTR)
    /*
 * Save file structure address for process_node().
 */
    FILEPTR = &file_str;
#endif /* defined(FILEPTR) */

    /*
 * Read file structure.
 */
    if (kread((KA_T)file_addr, (char *)&file_str, sizeof(file_str))) {
        (void)snpf(NameChars, NameCharsLength, "can't read file struct from %s",
                   print_kptr(file_addr, (char *)NULL, 0));
        enter_nm(NameChars);
        return;
    }
    CurrentLocalFile->off = (SZOFFTYPE)file_str.f_offset;
    if (file_str.f_count) {

        /*
     * Construct access code.
     */
        if ((flag = (file_str.f_flag & (FREAD | FWRITE))) == FREAD)
            CurrentLocalFile->access = 'r';
        else if (flag == FWRITE)
            CurrentLocalFile->access = 'w';
        else if (flag == (FREAD | FWRITE))
            CurrentLocalFile->access = 'u';

#if defined(HASFSTRUCT)
        /*
     * Save file structure values.
     */

#if !defined(HASNOFSCOUNT)
        if (OptFileStructValues & FSV_FILE_COUNT) {
            CurrentLocalFile->fct = (long)file_str.f_count;
            CurrentLocalFile->fsv |= FSV_FILE_COUNT;
        }
#endif /* !defined(HASNOFSCOUNT) */

#if !defined(HASNOFSADDR)
        if (OptFileStructValues & FSV_FILE_ADDR) {
            CurrentLocalFile->fsa = file_addr;
            CurrentLocalFile->fsv |= FSV_FILE_ADDR;
        }
#endif /* !defined(HASNOFSADDR) */

#if !defined(HASNOFSFLAGS)
        if (OptFileStructValues & FSV_FILE_FLAGS) {
            CurrentLocalFile->ffg = (long)file_str.f_flag;
            CurrentLocalFile->fsv |= FSV_FILE_FLAGS;
        }
#endif /* !defined(HASNOFSFLAGS) */

#if !defined(HASNOFSNADDR)
        if (OptFileStructValues & FSV_NODE_ID) {
            CurrentLocalFile->fna = (KA_T)file_str.f_data;
            CurrentLocalFile->fsv |= FSV_NODE_ID;
        }
#endif /* !defined(HASNOFSNADDR) */
#endif /* defined(HASFSTRUCT) */

        /*
     * Process structure by its type.
     */
        switch (file_str.f_type) {

#if defined(DTYPE_PIPE)
        case DTYPE_PIPE:
#if defined(HASPIPEFN)
            if (!SelectInetOnly)
                HASPIPEFN((KA_T)file_str.f_data);
#endif /* defined(HASPIPEFN) */
            return;
#endif /* defined(DTYPE_PIPE) */

#if defined(DTYPE_GNODE)
        case DTYPE_GNODE:
#endif /* defined(DTYPE_GNODE) */

#if defined(DTYPE_INODE)
        case DTYPE_INODE:
#endif /* defined(DTYPE_INODE) */

#if defined(DTYPE_PORT)
        case DTYPE_PORT:
#endif /* defined(DTYPE_PORT) */

#if defined(DTYPE_VNODE)
        case DTYPE_VNODE:
#endif /* defined(DTYPE_VNODE) */

#if defined(HASF_VNODE)
            process_node((KA_T)file_str.f_vnode);
#else  /* !defined(HASF_VNODE) */
            process_node((KA_T)file_str.f_data);
#endif /* defined(HASF_VNODE) */

            return;
        case DTYPE_SOCKET:
            process_socket((KA_T)file_str.f_data);
            return;

#if defined(HASKQUEUE)
        case DTYPE_KQUEUE:
            process_kqueue((KA_T)file_str.f_data);
            return;
#endif /* defined(HASKQUEUE) */

#if defined(HASPSXSEM)
        case DTYPE_PSXSEM:
            process_psxsem((KA_T)file_str.f_data);
            return;
#endif /* defined(HASPSXSEM) */

#if defined(HASPSXSHM)
        case DTYPE_PSXSHM:
            process_psxshm((KA_T)file_str.f_data);
            return;
#endif /* defined(HASPSXSHM) */

#if defined(HASPRIVFILETYPE)
        case PRIVFILETYPE:
            HASPRIVFILETYPE((KA_T)file_str.f_data);
            return;
#endif /* defined(HASPRIVFILETYPE) */

        default:
            if (file_str.f_type || file_str.f_ops) {
                (void)snpf(NameChars, NameCharsLength, "%s file struct, ty=%#x, op=%s",
                           print_kptr(file_addr, tbuf, sizeof(tbuf)), (int)file_str.f_type,
                           print_kptr((KA_T)file_str.f_ops, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
            }
        }
    }
    enter_nm("no more information");
}
#else  /* !defined(USE_LIB_PROCESS_FILE) */
char prfp_d1[] = "d";
char *prfp_d2 = prfp_d1;
#endif /* defined(USE_LIB_PROCESS_FILE) */

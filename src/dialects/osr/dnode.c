/*
 * dnode.c - SCO OpenServer node functions for lsof
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


_PROTOTYPE(static struct l_dev *finddev, (dev_t * dev, dev_t * rdev,
        int stream));


/*
 * finddev() - look up device by device number
 */

static struct l_dev *
finddev(dev_t * dev, dev_t * rdev, int stream)
{
    struct clone *c;
    struct l_dev *dp;
/*
 * Search device table for match.
 */

#if    defined(HASDCACHE)

    finddev_again:

#endif    /* defined(HASDCACHE) */

    if ((dp = lkupdev(dev, rdev, 0, 0)))
        return (dp);
/*
 * Search for clone.
 */
    if (stream && Clone) {
        for (c = Clone; c; c = c->next) {
            if (GET_MAJ_DEV(*rdev) == GET_MIN_DEV(DeviceTable[c->dx].rdev)) {

#if    defined(HASDCACHE)
                if (DevCacheUnsafe && !DeviceTable[c->dx].v && !vfy_dev(&DeviceTable[c->dx]))
                goto finddev_again;
#endif    /* defined(HASDCACHE) */

                return (&DeviceTable[c->dx]);
            }
        }
    }
    return ((struct l_dev *) NULL);
}


/*
 * process_node() - process node
 */

void
process_node(KA_T na)
{
    char *cp, tbuf[32];
    short dl;
    struct l_dev *dp;
    unsigned char *fa = (unsigned char *) NULL;
    struct filock fl;
    KA_T flf, flp;
    int fp, lp;
    struct inode i;
    short ity, udpsf, udpsl;
    int j, k, l;
    KA_T ka, qp;
    unsigned char *la = (unsigned char *) NULL;
    struct mounts *lm;
    struct module_info mi;
    unsigned short *n;
    KA_T p;
    struct inpcb pcb;
    int port;
    int pt = -1;
    struct queue q;
    struct qinit qi;
    struct stdata sd;
    char *tn;
    int type;
    struct udpdev udp;
    short udptm = 0;

#if    defined(HAS_NFS)
    struct rnode r;
#endif    /* defined(HAS_NFS) */

#if    OSRV >= 500
    short hpps = 0;
    unsigned short *n1;
    struct pipeinode pi;
#endif    /* OSRV>=500 */

/*
 * Read the inode.
 */
    if (!na) {
        enter_nm("no inode address");
        return;
    }
    if (readinode(na, &i)) {
        enter_nm(NameChars);
        return;
    }

#if    defined(HASNCACHE)
    CurrentLocalFile->node_addr = na;
#endif    /* defined(HASNCACHE) */

#if    defined(HASFSTRUCT)
    CurrentLocalFile->fna = na;
    CurrentLocalFile->fsv |= FSV_NODE_ID;
#endif    /* defined(HASFSTRUCT) */

/*
 * Identify the node type.
 */
    if (HaveSockdev && (i.i_ftype & IFMT) == IFCHR
        && GET_MAJ_DEV(i.i_rdev) == Sockdev) {

        /*
         * Process a socket.
         */
        process_socket(&i);
        return;
    }
    if (SelectInetOnly)
        return;
    ity = i.i_fstyp;
    type = i.i_ftype & IFMT;
    if (ity < 1 || ity > Fsinfomax || !Fsinfo[ity - 1]) {

#if    OSRV >= 500
        if (ity) {
#endif    /* OSRV>=500 */

        (void) snpf(NameChars, NameCharsLength, "unknown fstyp (%d) in inode", ity);
        enter_nm(NameChars);
        return;

#if    OSRV >= 500
        }
#endif    /* OSRV>=500 */

    }
    if (ity && strcasecmp(Fsinfo[ity - 1], "HS") == 0)
        NodeType = N_HSFS;

#if    defined(HAS_NFS)
        else if (ity && strcasecmp(Fsinfo[ity-1], "NFS") == 0) {

       /*
        * Get information on NFS file.
        */
           NodeType = N_NFS;
           CurrentLocalFile->is_nfs = 1;
           if (OptNfs)
               CurrentLocalFile->sel_flags |= SELNFS;
           if (!i.i_fsptr || readrnode((KA_T)i.i_fsptr, &r)) {
           (void) snpf(NameChars, NameCharsLength, "can't read rnode (%s)",
               print_kptr((KA_T)i.i_fsptr, (char *)NULL, 0));
           enter_nm(NameChars);
           return;
           }

# if    defined(HASNCACHE)
           CurrentLocalFile->node_addr = (KA_T)i.i_fsptr;
# endif /* defined(HASNCACHE) */

       }
#endif    /* defined(HAS_NFS) */

    else {

        /*
         * Determine the node type from the inode file type.
         */
        switch (type) {
            case IFBLK:
                NodeType = N_BLK;
                break;
            case IFCHR:
                NodeType = N_CHR;
                break;
            case IFIFO:
                NodeType = N_FIFO;
                break;
            case IFMPB:
            case IFMPC:
                NodeType = N_MPC;
                break;
            case IFNAM:
                NodeType = N_NM;
                break;
        }
    }
/*
 * Obtain lock information.
 */
    if ((flf = (KA_T) i.i_filocks)) {
        flp = flf;
        do {
            if ((kread(flp, (char *) &fl, sizeof(fl))))
                break;
            if (fl.set.l_pid != (pid_t) CurrentLocalProc->pid)
                continue;
            if (fl.set.l_whence == (short) 0 && fl.set.l_start == (off_t) 0
                && fl.set.l_len == 0x7fffffff)
                l = 1;
            else
                l = 0;

#if    OSRV < 500
            if (i.i_flag & IXLOCKED)
#else	/* OSRV>=500 */
                if (fl.flags & F_XOUT)
#endif    /* OSRV<500 */

                CurrentLocalFile->lock = l ? 'X' : 'x';
            else if (fl.set.l_type == F_RDLCK)
                CurrentLocalFile->lock = l ? 'R' : 'r';
            else if (fl.set.l_type == F_WRLCK)
                CurrentLocalFile->lock = l ? 'W' : 'w';
            else if (fl.set.l_type == (F_RDLCK | F_WRLCK))
                CurrentLocalFile->lock = 'u';
            break;
        } while ((flp = (KA_T) fl.next) && flp != flf);
    }

#if    OSRV >= 500
    /*
     * See if a FIFO node is an HPPS node -- 3.2v5.0.0 and higher.
     */
        if (NodeType == N_FIFO && ity && strcasecmp(Fsinfo[ity-1], "HPPS") == 0)
        {
            hpps = 1;
            if (i.i_fsptr) {
            enter_dev_ch(print_kptr((KA_T)i.i_fsptr, (char )NULL, 0));
            if (kread((KA_T)i.i_fsptr, (char *)&pi, sizeof(pi)) == 0)
                hpps = 2;
            }
        }
#endif    /* OSRV>=500 */

/*
 * Determine the device.
 */
    switch (NodeType) {
        case N_BLK:
            CurrentLocalFile->dev = i.i_dev;
            CurrentLocalFile->rdev = i.i_rdev;
            CurrentLocalFile->dev_def = CurrentLocalFile->rdev_def = 1;
            break;
        case N_FIFO:
        case N_HSFS:
        case N_NM:
        case N_REGLR:

#if    OSRV >= 500
            if (hpps)
            break;
#endif    /* OSRV>=500 */

            CurrentLocalFile->dev = i.i_dev;
            CurrentLocalFile->dev_def = 1;
            break;
        case N_CHR:
            CurrentLocalFile->dev = i.i_dev;
            CurrentLocalFile->rdev = i.i_rdev;
            CurrentLocalFile->dev_def = CurrentLocalFile->rdev_def = 1;
            if (i.i_sptr) {

                /*
                 * NameChars may be:
                 *    /dev/* name if it exists for i.i_rdev;
                 *    cdevsw[].d_name if it exists for GET_MAJ_DEV(i.i_rdev);
                 *    "STR:" otherwise.
                 */
                (void) snpf(NameChars, NameCharsLength, "STR:");
                CurrentLocalFile->is_stream = 1;
                k = strlen(NameChars);
                cp = (char *) NULL;
                if ((dp = finddev(&CurrentLocalFile->dev, &CurrentLocalFile->rdev, 1))) {
                    (void) snpf(&NameChars[k], NameCharsLength - k, dp->name);
                    k += strlen(dp->name);
                    if ((cp = strrchr(dp->name, '/')))
                        cp++;
                } else if ((j = GET_MAJ_DEV(i.i_rdev))
                           < Cdevcnt && (cp = Cdevsw[j])) {
                    (void) snpf(NameChars, NameCharsLength, "%s", cp);
                    k += strlen(cp);
                }
                /*
                 * Get the module names of all queue elements of the stream's
                 * sd_wrq queue.  Skip module names that end in "head",
                 * match the last component of the /dev name, or match the
                 * cdevsw[].d_name.
                 */
                p = (KA_T) NULL;
                if (!kread((KA_T) i.i_sptr, (char *) &sd, sizeof(sd))) {
                    dl = sizeof(tbuf) - 1;
                    tbuf[dl] = '\0';
                    qp = (KA_T) sd.sd_wrq;
                    for (j = 0; qp && j < 20; j++, qp = (KA_T) q.q_next) {
                        if (kread(qp, (char *) &q, sizeof(q)))
                            break;
                        if (!(ka = (KA_T) q.q_qinfo)
                            || kread(ka, (char *) &qi, sizeof(qi)))
                            continue;
                        if (!(ka = (KA_T) qi.qi_minfo)
                            || kread(ka, (char *) &mi, sizeof(mi)))
                            continue;
                        if (!(ka = (KA_T) mi.mi_idname)
                            || kread(ka, tbuf, dl))
                            continue;
                        if ((l = strlen(tbuf)) < 1)
                            continue;
                        if (l >= 4 && strcmp(&tbuf[l - 4], "head") == 0)
                            continue;
                        if (cp && strcmp(cp, tbuf) == 0) {
                            if (q.q_ptr && pt < 0) {

                                /*
                                 * If this is a TCP or UDP module and the
                                 * queue structure has a private pointer in
                                 * q_ptr, save it as a PCB address.
                                 */
                                if (strcasecmp(cp, "tcp") == 0) {
                                    pt = 0;
                                    (void) snpf(CurrentLocalFile->iproto,
                                                sizeof(CurrentLocalFile->iproto), "TCP");
                                } else if (strcasecmp(cp, "udp") == 0) {
                                    pt = 1;
                                    (void) snpf(CurrentLocalFile->iproto,
                                                sizeof(CurrentLocalFile->iproto), "UDP");
                                }
                                if (pt >= 0)
                                    p = (KA_T) q.q_ptr;
                                else
                                    pt = -1;
                            }
                            continue;
                        }
                        if (k) {
                            if ((k + 2) > (NameCharsLength - 1))
                                break;
                            (void) snpf(&NameChars[k], NameCharsLength - k, "->");
                            k += 2;
                        }
                        if ((k + l) > (NameCharsLength - 1))
                            break;
                        (void) snpf(&NameChars[k], NameCharsLength - k, tbuf);
                        k += l;
                    }
                }
                if (p && pt >= 0) {

                    /*
                     * If the stream has a TCP or UDP module with a PCB pointer,
                     * print any associated local and foreign Internet addresses.
                     */
                    if (kread(p, (char *) &pcb, sizeof(pcb)))
                        break;
                    if (OptNetwork)
                        CurrentLocalFile->sel_flags |= SELNET;
                    if ((k + 1) > (NameCharsLength - 1))
                        break;
                    if (pt == 1 && pcb.inp_ppcb) {

                        /*
                         * If this is a UDP stream, get the udpdev structure at the
                         * PCB's per-protocol address.  It may contain addresses.
                         */
                        if (kread((KA_T) pcb.inp_ppcb, (char *) &udp, sizeof(udp))
                            == 0) {

#if    OSRV >= 500
                            if (udp.ud_lsin.sin_addr.s_addr != INADDR_ANY
                            ||  udp.ud_lsin.sin_port != 0)
                            udpsl = 1;
                            else
                            udpsl = 0;
#endif    /* OSRV>=500 */

                            if (udp.ud_fsin.sin_addr.s_addr != INADDR_ANY
                                || udp.ud_fsin.sin_port != 0)
                                udpsf = 1;
                            else
                                udpsf = 0;
                        }
                    } else
                        udpsf = udpsl = 0;
                    /*
                     * Enter the local address from the PCB.  If there is none,
                     * and if this is a 5.0.0 or greater UDP stream, and if it
                     * has a local address set, use it.
                     */
                    la = (unsigned char *) &pcb.inp_laddr;
                    lp = (int) ntohs(pcb.inp_lport);

#if    OSRV >= 500
                    if (((struct in_addr *)la)->s_addr == INADDR_ANY
                    &&  lp == 0 && udpsl) {
                    la = (unsigned char *)&udp.ud_lsin.sin_addr;
                    lp = (int)ntohs(udp.ud_lsin.sin_port);
                    }

#endif    /* OSRV>=500 */

                    /*
                     * Enter the foreign address from the PCB.  If there is
                     * none, and if this is a 5.0.0 or greater UDP stream, and
                     * if it has a local address set, use it.
                     */
                    if (pcb.inp_faddr.s_addr != INADDR_ANY || pcb.inp_fport != 0) {
                        fa = (unsigned char *) &pcb.inp_faddr;
                        fp = (int) ntohs(pcb.inp_fport);
                    } else if (udpsf) {
                        fa = (unsigned char *) &udp.ud_fsin.sin_addr;
                        fp = (int) ntohs(udp.ud_fsin.sin_port);
                        udptm = 1;
                    }
                    if (fa || la) {
                        (void) ent_inaddr(la, lp, fa, fp, AF_INET);
                        if (udptm && !CurrentLocalFile->name_append)
                            (void) udp_tm(udp.ud_ftime);
                    }
                    if (!i.i_number)
                        CurrentLocalFile->inp_ty = 2;
                }
            } else {
                if (ity) {
                    if (strcasecmp(Fsinfo[ity - 1], "COM") == 0)
                        NodeType = N_COM;
                    else
                        NodeType = N_CHR;
                } else {
                    NodeType = N_CHR;
                    if (!finddev(&i.i_dev, &i.i_rdev, 0)
                        && HaveEventMajor
                        && GET_MAJ_DEV(i.i_rdev) == EventMajor)
                        (void) snpf(NameChars, NameCharsLength,
                                    "clone %d:/dev/event", GET_MIN_DEV(i.i_rdev));
                }
            }
            break;

#if    defined(HAS_NFS)
        case N_NFS:

#if	OSRV<500
            CurrentLocalFile->dev = (dev_t)_makedev(~GET_MAJ_DEV(i.i_dev),
                          GET_MIN_DEV(i.i_dev));
            CurrentLocalFile->rdev = (dev_t)_makedev(~GET_MAJ_DEV(i.i_rdev),
                           GET_MIN_DEV(i.i_rdev));
#else	/* OSRV>=500 */
            CurrentLocalFile->dev = i.i_dev;
            CurrentLocalFile->rdev = i.i_rdev;
#endif	/* OSRV<500 */

            CurrentLocalFile->dev_def = CurrentLocalFile->rdev_def = 1;
            break;
#endif    /* defined(HAS_NFS) */

    }
/*
 * Determine the inode number.
 */
    switch (NodeType) {
        case N_HSFS:

#if    OSRV < 500
            /*
             * High Sierra inode numbers for versions below 5.0.0, as reported
             * by "ls -i" and stat(2), are the lower 16 bits of i_number.
             */
            if ((CurrentLocalFile->inode = (unsigned long) (i.i_number & 0xffff)))
#else	/* OSRV>=500 */
                if ((CurrentLocalFile->inode = (unsigned long)i.i_number))
#endif    /* OSRV<500 */

                CurrentLocalFile->inp_ty = 1;
            break;

#if    defined(HAS_NFS)
        case N_NFS:

#if	OSRV<500
            n = (unsigned short *)&r.r_fh.fh_pad[14];
            if ((CurrentLocalFile->inode = (unsigned long)ntohs(*n)))
            CurrentLocalFile->inp_ty = 1;
            else if ((CurrentLocalFile->inode = (unsigned long)r.r_fh.fh_u.fh_fgen_u))
#else	/* OSRV>=500 */
            n = (unsigned short *)&r.r_fh.fh_u.fh_fid_u[4];
            n1 = (unsigned short *)&r.r_fh.fh_u.fh_fid_u[2];
            if ((CurrentLocalFile->inode = (unsigned long)*n))
            CurrentLocalFile->inp_ty = 1;
            else if ((CurrentLocalFile->inode = (unsigned long)*n1))
#endif	/* OSRV<500 */

            CurrentLocalFile->inp_ty = 1;
            break;
#endif    /* defined(HAS_NFS) */

        case N_BLK:
        case N_CHR:
        case N_COM:
        case N_FIFO:
        case N_NM:
        case N_REGLR:

#if    OSRV >= 500
            /*
             * Inodes for some 5.0.x HPPS FIFOs have an i_number that is the same
             * as i_fsptr.  If it is, ignore it, because i_fsptr has already been
             * recorded for the DEVICE column.
             */
                if (hpps && i.i_fsptr && i.i_number
                && (unsigned long)i.i_fsptr == (unsigned long)i.i_number)
                break;
#endif    /* OSRV>=500 */

            if (i.i_number) {
                CurrentLocalFile->inode = (unsigned long) i.i_number;
                CurrentLocalFile->inp_ty = 1;
            }
            break;
    }
/*
 * Determine the file size.
 */
    if (OptOffset)
        CurrentLocalFile->off_def = 1;
    else {
        switch (NodeType) {
            case N_BLK:
                if (!OptSize)
                    CurrentLocalFile->off_def = 1;
                break;
            case N_CHR:
            case N_COM:
                if (!OptSize)
                    CurrentLocalFile->off_def = 1;
                break;
            case N_FIFO:

#if    OSRV >= 500
                if (hpps == 2) {
                    CurrentLocalFile->sz = (SZOFFTYPE)pi.count;
                    CurrentLocalFile->sz_def = 1;
                    break;
                }
#endif    /* OSRV>=500 */

                if (!OptSize)
                    CurrentLocalFile->off_def = 1;
                break;
            case N_HSFS:

#if    defined(HAS_NFS)
                case N_NFS:
                CurrentLocalFile->sz = (SZOFFTYPE)i.i_size;
                CurrentLocalFile->sz_def = 1;
                break;
#endif    /* defined(HAS_NFS) */

            case N_REGLR:
                if (type == IFREG || type == IFDIR) {
                    CurrentLocalFile->sz = (SZOFFTYPE) i.i_size;
                    CurrentLocalFile->sz_def = 1;
                }
                break;
        }
    }
/*
 * Record link count.
 */
    if (OptLinkCount) {
        CurrentLocalFile->nlink = (long) i.i_nlink;
        CurrentLocalFile->nlink_def = 1;
        if (LinkCountThreshold && (CurrentLocalFile->nlink < LinkCountThreshold))
            CurrentLocalFile->sel_flags |= SELNLINK;
    }
/*
 * Format the type name.
 */
    switch (type) {
        case IFDIR:
            tn = "DIR";
            break;
        case IFBLK:
            tn = "BLK";
            break;
        case IFCHR:
            tn = "CHR";
            break;
        case IFREG:
            tn = "REG";
            break;
        case IFMPC:
            tn = "MPC";
            break;
        case IFMPB:
            tn = "MPB";
            break;
        case IFNAM:
            if (i.i_rdev == S_INSEM)
                tn = "XSEM";
            else if (i.i_rdev == S_INSHD)
                tn = "XSD";
            else {
                tn = "XNAM";
                (void) snpf(NameChars, NameCharsLength,
                            "unknown Xenix special file type: %x", i.i_rdev);
            }
            break;
        case IFIFO:
            tn = "FIFO";
            break;

#if    defined(IFLNK)
        case IFLNK:
            tn = "LINK";
            break;
#endif    /* defined(IFLNK) */

        default:
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "%04o",
                        ((type >> 12) & 0xfff));
            tn = NULL;
    }
    if (tn)
        (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "%s", tn);
/*
 * Save the file system names.
 */
    switch (NodeType) {
        case N_BLK:
        case N_CHR:
        case N_FIFO:
        case N_HSFS:

#if    defined(HAS_NFS)
            case N_NFS:
#endif    /* defined(HAS_NFS) */

        case N_NM:
        case N_REGLR:
            if (CurrentLocalFile->dev_def) {

                /*
                 * Defer the local mount info table search until printname().
                 */
                CurrentLocalFile->lmi_srch = 1;
            }
            break;
    }
    CurrentLocalFile->ntype = NodeType;

#if     defined(HASBLKDEV)
    /*
     * If this is a IFBLK file and it's missing an inode number, try to
     * supply one.
     */
        if ((CurrentLocalFile->inp_ty == 0) && (type == IFBLK))
            find_bl_ino();
#endif  /* defined(HASBLKDEV) */

/*
 * If this is a IFCHR file and it's missing an inode number, try to
 * supply one.
 */
    if ((CurrentLocalFile->inp_ty == 0) && (type == IFCHR))
        find_ch_ino();
/*
 * Test for specified file.
 */
    if (SearchFileChain && is_file_named((char *) NULL,
                               ((type == IFCHR) || (type == IFBLK) || (type == IFNAM)) ? 1
                                                                                       : 0))
        CurrentLocalFile->sel_flags |= SELNM;

#if    OSRV >= 500
    /*
     * If this is an HPPS node and no other name characters have been
     * entered, enter HPPS as the name.
     */
        if (hpps && NameChars[0] == '\0')
            (void) snpf(NameChars, NameCharsLength, "HPPS");
#endif    /* OSRV>=500 */

/*
 * Enter name characters.
 */
    if (NameChars[0])
        enter_nm(NameChars);
}

/*
 * dsock.c -- Darwin socket processing functions for libproc-based lsof
 */


/*
 * Portions Copyright 2005 Apple Computer, Inc.  All rights reserved.
 *
 *
 * Written by Allan Nathanson, Apple Computer, Inc., and J. Menke.
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

#include "lsof.h"


/*
 * IPv6_2_IPv4()  -- macro to define the address of an IPv4 address contained
 *                 in an IPv6 address
 */

#define    IPv6_2_IPv4(v6)    (((uint8_t *)((struct in6_addr *)v6)->s6_addr)+12)


/*
 * process_socket() -- process socket file
 */

static void
process_socket_common(struct socket_fdinfo * si)
{
    unsigned char *fa = (unsigned char *) NULL;
    int fam, fp, lp, unl;
    unsigned char *la = (unsigned char *) NULL;

/*
 * Enter basic socket values.
 */
    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "sock");
    CurrentLocalFile->inp_ty = 2;
/*
 * Enter basic file information.
 */
    enter_file_info(&si->pfi);
/*
 * Enable size or offset display.
 */
    if (OptSize) {
        if (CurrentLocalFile->access == 'r')
            CurrentLocalFile->sz = (SZOFFTYPE) si->psi.soi_rcv.sbi_cc;
        else if (CurrentLocalFile->access == 'w')
            CurrentLocalFile->sz = (SZOFFTYPE) si->psi.soi_snd.sbi_cc;
        else
            CurrentLocalFile->sz = (SZOFFTYPE) (si->psi.soi_rcv.sbi_cc
                                  + si->psi.soi_snd.sbi_cc);
        CurrentLocalFile->sz_def = 1;
    } else
        CurrentLocalFile->off_def = 1;

#if    defined(HASTCPTPIQ)
    /*
     * Enter send and receive queue sizes.
     */
        CurrentLocalFile->lts.recv_queue = si->psi.soi_rcv.sbi_cc;
        CurrentLocalFile->lts.send_queue = si->psi.soi_snd.sbi_cc;
        CurrentLocalFile->lts.recv_queue_st = CurrentLocalFile->lts.send_queue_st = (unsigned char)1;
#endif    /* defined(HASTCPTPIQ) */

#if    defined(HASSOOPT)
    /*
     * Enter socket options.
     */
        CurrentLocalFile->lts.ltm = (unsigned int)(si->psi.soi_linger & 0xffff);
        CurrentLocalFile->lts.opt = (unsigned int)(si->psi.soi_options & 0xffff);
        CurrentLocalFile->lts.pqlen = (unsigned int)si->psi.soi_incqlen;
        CurrentLocalFile->lts.qlen = (unsigned int)si->psi.soi_qlen;
        CurrentLocalFile->lts.qlim = (unsigned int)si->psi.soi_qlimit;
        CurrentLocalFile->lts.rbsz = (unsigned long)si->psi.soi_rcv.sbi_mbmax;
        CurrentLocalFile->lts.sbsz = (unsigned long)si->psi.soi_snd.sbi_mbmax;
        CurrentLocalFile->lts.pqlens = CurrentLocalFile->lts.qlens = CurrentLocalFile->lts.qlims = CurrentLocalFile->lts.rbszs
                   = CurrentLocalFile->lts.sbszs = (unsigned char)1;
#endif    /* defined(HASSOOPT) */

#if    defined(HASSOSTATE)
    /*
     * Enter socket state.
     */
        CurrentLocalFile->lts.sock_state = (unsigned int)si->psi.soi_state;
#endif    /* defined(HASSOSTATE) */

/*
 * Process socket by its associated domain family.
 */
    switch ((fam = si->psi.soi_family)) {
        case AF_INET:
        case AF_INET6:

            /*
             * Process IPv[46] sockets.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type),
                        (fam == AF_INET) ? "IPv4" : "IPv6");
            if ((si->psi.soi_kind != SOCKINFO_IN) &&
                (si->psi.soi_kind != SOCKINFO_TCP)) {
                break;
            }
            /*
             * Process TCP state inclusions and exclusions, as required.
             */
            if ((si->psi.soi_kind == SOCKINFO_TCP) && (TcpStateExcludeCount || TcpStateIncludeCount)) {
                int tsnx = (int) si->psi.soi_proto.pri_tcp.tcpsi_state
                           + TcpStateOffset;

                if ((tsnx >= 0) && (tsnx < TcpNumStates)) {
                    if (TcpStateExcludeCount) {
                        if (TcpStateExclude[tsnx]) {
                            CurrentLocalFile->sel_flags |= SELEXCLF;
                            return;
                        }
                    }
                    if (TcpStateIncludeCount) {
                        if (TcpStateInclude[tsnx])
                            TcpStateInclude[tsnx] = 2;
                        else {
                            CurrentLocalFile->sel_flags |= SELEXCLF;
                            return;
                        }
                    }
                }
            }
            /*
             * Process an Internet domain socket.
             */
            if (OptNetwork) {
                if (!OptNetworkType
                    || ((OptNetworkType == 4) && (fam == AF_INET))
                    || ((OptNetworkType == 6) && (fam == AF_INET6))
                        )
                    CurrentLocalFile->sel_flags |= SELNET;
            }
            printiproto(si->psi.soi_protocol);
            if ((si->psi.soi_kind == SOCKINFO_TCP)
                && si->psi.soi_proto.pri_tcp.tcpsi_tp) {
                enter_dev_ch(print_kptr((KA_T) si->psi.soi_proto.pri_tcp.tcpsi_tp,
                                        (char *) NULL, 0));
            } else
                enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            if (fam == AF_INET) {

                /*
                 * Enter IPv4 address information.
                 */
                if (si->psi.soi_kind == SOCKINFO_TCP) {

                    /*
                     * Enter information for a TCP socket.
                     */
                    la = (unsigned char *) &si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_46.i46a_addr4;
                    lp = (int) ntohs(si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
                    fa = (unsigned char *) &si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_46.i46a_addr4;
                    fp = (int) ntohs(si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
                } else {

                    /*
                     * Enter information for a non-TCP socket.
                     */
                    la = (unsigned char *) &si->psi.soi_proto.pri_in.insi_laddr.ina_46.i46a_addr4;
                    lp = (int) ntohs(si->psi.soi_proto.pri_in.insi_lport);
                    fa = (unsigned char *) &si->psi.soi_proto.pri_in.insi_faddr.ina_46.i46a_addr4;
                    fp = (int) ntohs(si->psi.soi_proto.pri_in.insi_fport);
                }
                if ((fa && (*fa == INADDR_ANY)) && !fp) {
                    fa = (unsigned char *) NULL;
                    fp = 0;
                }
            } else {

                /*
                 * Enter IPv6 address information
                 */
                int v4mapped = 0;

                if (si->psi.soi_kind == SOCKINFO_TCP) {

                    /*
                     * Enter TCP socket information.
                     */
                    la = (unsigned char *) &si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_laddr.ina_6;
                    lp = (int) ntohs(si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
                    fa = (unsigned char *) &si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_faddr.ina_6;
                    fp = (int) ntohs(si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
                    if ((si->psi.soi_proto.pri_tcp.tcpsi_ini.insi_vflag & INI_IPV4) != 0)
                        v4mapped = 1;
                } else {

                    /*
                     * Enter non-TCP socket information.
                     */
                    la = (unsigned char *) &si->psi.soi_proto.pri_in.insi_laddr.ina_6;
                    lp = (int) ntohs(si->psi.soi_proto.pri_in.insi_lport);
                    fa = (unsigned char *) &si->psi.soi_proto.pri_in.insi_faddr.ina_6;
                    fp = (int) ntohs(si->psi.soi_proto.pri_in.insi_fport);
                    if ((si->psi.soi_proto.pri_in.insi_vflag & INI_IPV4) != 0)
                        v4mapped = 1;
                }
                if (IN6_IS_ADDR_UNSPECIFIED((struct in6_addr *) fa) && !fp) {
                    fa = (unsigned char *) NULL;
                    fp = 0;
                }
                if (v4mapped) {

                    /*
                     * Adjust IPv4 addresses mapped in IPv6 addresses.
                     */
                    fam = AF_INET;
                    if (la)
                        la = (unsigned char *) IPv6_2_IPv4(la);
                    if (fa)
                        fa = (unsigned char *) IPv6_2_IPv4(fa);
                }
            }
            /*
             * Enter local and remote addresses by address family.
             */
            if (fa || la)
                (void) ent_inaddr(la, lp, fa, fp, fam);
            if (si->psi.soi_kind == SOCKINFO_TCP) {

                /*
                 * Enter a TCP socket definition and its state.
                 */
                CurrentLocalFile->lts.type = 0;
                CurrentLocalFile->lts.state.val = (int) si->psi.soi_proto.pri_tcp.tcpsi_state;
                /*
                 * Enter TCP options.
                 */

#if    defined(HASSOOPT)
                CurrentLocalFile->lts.kai = (unsigned int)si->psi.soi_proto.pri_tcp.tcpsi_timer[TCPT_KEEP];
#endif    /* defined(HASSOOPT) */

#if    defined(HASTCPOPT)
                CurrentLocalFile->lts.mss = (unsigned long)si->psi.soi_proto.pri_tcp.tcpsi_mss;
                CurrentLocalFile->lts.msss = (unsigned char)1;
                CurrentLocalFile->lts.topt = (unsigned int)si->psi.soi_proto.pri_tcp.tcpsi_flags;
#endif    /* defined(HASTCPOPT) */

            }
            break;
        case AF_UNIX:

            /*
             * Process a UNIX domain socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "unix");
            if (si->psi.soi_kind != SOCKINFO_UN)
                break;
            if (OptUnixSocket)
                CurrentLocalFile->sel_flags |= SELUNX;
            enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            /*
             * Enter information on a UNIX domain socket that has no address bound
             * to it, although it may be connected to another UNIX domain socket
             * as a pipe.
             */
            if (si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_family != AF_UNIX) {
                if (si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_family
                    == AF_UNSPEC) {
                    if (si->psi.soi_proto.pri_un.unsi_conn_pcb) {
                        (void) snpf(NameChars, NameCharsLength, "->%s",
                                    print_kptr((KA_T) si->psi.soi_proto.pri_un.unsi_conn_pcb, (char *) NULL, 0));
                    } else
                        (void) snpf(NameChars, NameCharsLength, "->(none)");
                } else
                    (void) snpf(NameChars, NameCharsLength, "unknown sun_family (%d)",
                                si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_family);
                break;
            }
            if (si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path[0]) {
                unl = si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_len - offsetof(
                struct sockaddr_un, sun_path);
                if ((unl < 0) || (unl >= sizeof(si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path)))
                    unl = sizeof(si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path) - 1;
                si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path[unl] = '\0';
                if (si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path[0]
                    && SearchFileChain
                    && is_file_named(si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path, 0))
                    CurrentLocalFile->sel_flags |= SELNM;
                if (si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path[0]
                    && !NameChars[0])
                    (void) snpf(NameChars, NameCharsLength, "%s", si->psi.soi_proto.pri_un.unsi_addr.ua_sun.sun_path);
            } else
                (void) snpf(NameChars, NameCharsLength, "no address");
            break;
        case AF_ROUTE:

            /*
             * Process a ROUTE domain socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "rte");
            if (!OptSize)
                CurrentLocalFile->off_def = 1;
            break;
        case AF_NDRV:

            /*
             * Process an NDRV domain socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "ndrv");
            if (si->psi.soi_kind != SOCKINFO_NDRV)
                break;
            enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            si->psi.soi_proto.pri_ndrv.ndrvsi_if_name[sizeof(si->psi.soi_proto.pri_ndrv.ndrvsi_if_name) - 1] = '\0';
            (void) snpf(NameChars, NameCharsLength, "-> %s%d",
                        si->psi.soi_proto.pri_ndrv.ndrvsi_if_name,
                        si->psi.soi_proto.pri_ndrv.ndrvsi_if_unit);
            break;
        case pseudo_AF_KEY:

            /*
             * Process an [internal] key-management function socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "key");
            enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            break;
        case AF_SYSTEM:

            /*
             * Process a SYSTEM domain socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "systm");
            if (si->psi.soi_kind != SOCKINFO_KERN_EVENT)
                break;
            enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            (void) snpf(NameChars, NameCharsLength, "[%x:%x:%x]",
                        si->psi.soi_proto.pri_kern_event.kesi_vendor_code_filter,
                        si->psi.soi_proto.pri_kern_event.kesi_class_filter,
                        si->psi.soi_proto.pri_kern_event.kesi_subclass_filter);
            break;
        case AF_PPP:

            /*
             * Process a PPP domain socket.
             */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "ppp");
            enter_dev_ch(print_kptr((KA_T) si->psi.soi_pcb, (char *) NULL, 0));
            break;
        default:
            printunkaf(fam, 1);
    }
/*
 * If there are NAME column characters, enter them.
 */
    if (NameChars[0])
        enter_nm(NameChars);
}


void
process_socket(int pid, int32_t fd)
{
    int nb;
    struct socket_fdinfo si;
/*
 * Get socket information.
 */
    nb = proc_pidfdinfo(pid, fd, PROC_PIDFDSOCKETINFO, &si, sizeof(si));
    if (nb <= 0) {
        (void) err2nm("socket");
        return;
    } else if (nb < sizeof(si)) {
        (void) fprintf(stderr,
                       "%s: PID %d, FD %d: proc_pidfdinfo(PROC_PIDFDSOCKETINFO);\n",
                       ProgramName, pid, fd);
        (void) fprintf(stderr,
                       "      too few bytes; expected %ld, got %d\n",
                       sizeof(si), nb);
        Exit(1);
    }

    process_socket_common(&si);
}

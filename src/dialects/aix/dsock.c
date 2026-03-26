/*
 * dsock.c - AIX socket processing functions for lsof
 */


/*
 * Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
        "@(#) Copyright 1994 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: dsock.c,v 1.24 2008/10/21 16:14:18 abe Exp $";
#endif


#include "lsof.h"


/*
 * We include <sys/domain.h> here instead of "dlsof.h" for gcc's benefit.
 * Its loader can't handle the multiple CONST u_char arrays declared in
 * <net/net_globals.h> -- e.g., etherbroadcastaddr[].  (<sys/domain.h>
 * #include's <net/net_globals.h>.)
 */

#include <net/netopt.h>
#include <sys/domain.h>


/*
 * process_socket() - process socket file
 */

void
process_socket(sa)
        KA_T sa;            /* socket address in kernel */
{
    struct domain d;
    unsigned char *fa = (unsigned char *) NULL;
    int fam;
    int fp, lp, uo;
    struct gnode g;
    struct l_ino i;
    struct inpcb inp;
    int is = 0;
    unsigned char *la = (unsigned char *) NULL;
    struct protosw p;
    struct socket s;
    struct tcpcb t;
    int ts = 0;
    int tsn, tsnx;
    struct unpcb uc, unp;
    struct sockaddr_un *ua = (struct sockaddr_un *) NULL;
    struct sockaddr_un un;
    struct vnode v;
    struct mbuf mb;
/*
 * Set socket file variables.
 */
    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "sock");
    CurrentLocalFile->inp_ty = 2;
/*
 * Read socket and protocol switch structures.
 */
    if (!sa) {
        enter_nm("no socket address");
        return;
    }
    if (kread(sa, (char *) &s, sizeof(s))) {
        (void) snpf(NameChars, NameCharsLength, "can't read socket struct from %s",
                    print_kptr(sa, (char *) NULL, 0));
        enter_nm(NameChars);
        return;
    }
    if (!s.so_type) {
        enter_nm("no socket type");
        return;
    }
    if (!s.so_proto
        || kread((KA_T) s.so_proto, (char *) &p, sizeof(p))) {
        (void) snpf(NameChars, NameCharsLength, "can't read protocol switch from %s",
                    print_kptr((KA_T) s.so_proto, (char *) NULL, 0));
        enter_nm(NameChars);
        return;
    }
/*
 * Save size information.
 */
    if (OptSize) {
        if (CurrentLocalFile->access == 'r')
            CurrentLocalFile->sz = (SZOFFTYPE) s.so_rcv.sb_cc;
        else if (CurrentLocalFile->access == 'w')
            CurrentLocalFile->sz = (SZOFFTYPE) s.so_snd.sb_cc;
        else
            CurrentLocalFile->sz = (SZOFFTYPE) (s.so_rcv.sb_cc + s.so_snd.sb_cc);
        CurrentLocalFile->sz_def = 1;
    } else
        CurrentLocalFile->off_def = 1;

#if    defined(HASTCPTPIQ)
    CurrentLocalFile->lts.rq = s.so_rcv.sb_cc;
    CurrentLocalFile->lts.sq = s.so_snd.sb_cc;
    CurrentLocalFile->lts.rqs = CurrentLocalFile->lts.sqs = 1;
#endif    /* defined(HASTCPTPIQ) */

#if    defined(HASSOOPT)
    CurrentLocalFile->lts.ltm = (unsigned int)s.so_linger;
    CurrentLocalFile->lts.opt = (unsigned int)s.so_options;
    CurrentLocalFile->lts.pqlen = (unsigned int)s.so_q0len;
    CurrentLocalFile->lts.qlen = (unsigned int)s.so_qlen;
    CurrentLocalFile->lts.qlim = (unsigned int)s.so_qlimit;
    CurrentLocalFile->lts.rbsz = (unsigned long)s.so_rcv.sb_mbmax;
    CurrentLocalFile->lts.sbsz = (unsigned long)s.so_snd.sb_mbmax;
    CurrentLocalFile->lts.pqlens = CurrentLocalFile->lts.qlens = CurrentLocalFile->lts.qlims = CurrentLocalFile->lts.rbszs
               = CurrentLocalFile->lts.sbszs = (unsigned char)1;
#endif    /* defined(HASSOOPT) */

#if    defined(HASSOSTATE)
    CurrentLocalFile->lts.ss = (unsigned int)s.so_state;
#endif    /* defined(HASSOSTATE) */

/*
 * Process socket by the associated domain family.
 */
    if (!p.pr_domain
        || kread((KA_T) p.pr_domain, (char *) &d, sizeof(d))) {
        (void) snpf(NameChars, NameCharsLength, "can't read domain struct from %s",
                    print_kptr((KA_T) p.pr_domain, (char *) NULL, 0));
        enter_nm(NameChars);
        return;
    }
    switch ((fam = d.dom_family)) {
/*
 * Process an Internet domain socket.
 */
        case AF_INET:

#if    defined(HASIPv6)
            case AF_INET6:
#endif    /* defined(HASIPv6) */

            /*
             * Read protocol control block.
             */
            if (!s.so_pcb
                || kread((KA_T) s.so_pcb, (char *) &inp, sizeof(inp))) {
                if (!s.so_pcb) {
                    (void) snpf(NameChars, NameCharsLength, "no PCB%s%s",
                                (s.so_state & SS_CANTSENDMORE) ? ", CANTSENDMORE" : "",
                                (s.so_state & SS_CANTRCVMORE) ? ", CANTRCVMORE" : "");
                } else {
                    (void) snpf(NameChars, NameCharsLength, "can't read inpcb at %s",
                                print_kptr((KA_T) s.so_pcb, (char *) NULL, 0));
                }
                enter_nm(NameChars);
                return;
            }
            if (p.pr_protocol == IPPROTO_TCP) {

                /*
                 * If this is a TCP socket, read its control block.
                 */
                if (inp.inp_ppcb
                    && !kread((KA_T) inp.inp_ppcb, (char *) &t, sizeof(t))) {
                    ts = 1;
                    tsn = (int) t.t_state;
                    tsnx = tsn + TcpStateOffset;
                }
            }
            if (ts
                && (TcpStateIncludeCount || TcpStateExcludeCount)
                && (tsnx >= 0) && (tsnx < TcpNumStates)
                    ) {

                /*
                 * Check TCP state name inclusion and exclusions.
                 */
                if (TcpStateExcludeCount) {
                    if (TcpStateExclude[tsnx]) {
                        CurrentLocalFile->sf |= SELEXCLF;
                        return;
                    }
                }
                if (TcpStateIncludeCount) {
                    if (TcpStateInclude[tsnx])
                        TcpStateInclude[tsnx] = 2;
                    else {
                        CurrentLocalFile->sf |= SELEXCLF;
                        return;
                    }
                }
            }
            if (OptNetwork) {

                /*
                 * Set SELNET flag for the file, as requested.
                 */
                if (!OptNetworkType
                    || ((OptNetworkType == 4) && (fam == AF_INET))

#if    defined(HASIPv6)
                    ||  ((OptNetworkType == 6) && (fam == AF_INET6))
#endif    /* defined(HASIPv6) */
                        )

                    CurrentLocalFile->sf |= SELNET;
            }
            printiproto(p.pr_protocol);

#if    defined(HASIPv6)
        (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type),
            fam == AF_INET ? "IPv4" : "IPv6");
#else	/* !defined(HASIPv6) */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "inet");
#endif    /* defined(HASIPv6) */

            /*
             * Save Internet socket information.
             */
            enter_dev_ch(print_kptr((KA_T)(inp.inp_ppcb ? inp.inp_ppcb
                                                        : s.so_pcb),
                                    (char *) NULL, 0));

#if    defined(HASIPv6)
        /*
         * If this is an IPv6 (AF_INET6) socket and IPv4 compatibility
         * mode is enabled, use the IPv4 address, change the family
         * indicator from AF_INET6 to AF_INET.  Otherwise, use the IPv6
         * address.  Don't ignore empty addresses.
         */
            if (fam == AF_INET6) {
            if (inp.inp_flags & INP_COMPATV4) {
                fam = AF_INET;
                la = (unsigned char *)&inp.inp_laddr;
            } else
                la = (unsigned char *)&inp.inp_laddr6;
            } else
#endif    /* defined(HASIPv6) */

            la = (unsigned char *) &inp.inp_laddr;
            lp = (int) ntohs(inp.inp_lport);
            if (fam == AF_INET
                && (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport != 0)) {
                fa = (unsigned char *) &inp.inp_faddr;
                fp = (int) ntohs(inp.inp_fport);
            }

#if    defined(HASIPv6)
        else if (fam == AF_INET6) {

        /*
         * If this is an IPv6 (AF_INET6) socket and IPv4 compatibility
         * mode is enabled, use the IPv4 address, change the family
         * indicator from AF_INET6 to AF_INET.  Otherwise, use the IPv6
         * address.  Ignore empty addresses.
         */
        if (inp.inp_flags & INP_COMPATV4) {
            fam = AF_INET;
            if (inp.inp_faddr.s_addr != INADDR_ANY
            || inp.inp_fport != 0)
            {
            fa = (unsigned char *)&inp.inp_faddr;
            fp = (int)ntohs(inp.inp_fport);
            }
        } else {
            if (!IN6_IS_ADDR_UNSPECIFIED(&inp.inp_faddr6)) {
            fa = (unsigned char *)&inp.inp_faddr6;
            fp = (int)ntohs(inp.inp_fport);
            }
        }
        }
#endif    /* defined(HASIPv6) */

            if (fa || la)
                (void) ent_inaddr(la, lp, fa, fp, fam);
            if (ts) {
                CurrentLocalFile->lts.type = 0;
                CurrentLocalFile->lts.state.i = tsn;

#if    defined(HASSOOPT)
                CurrentLocalFile->lts.kai = (unsigned int)t.t_timer[TCPT_KEEP];
#endif    /* defined(HASSOOPT) */

#if    defined(HASTCPOPT)
                CurrentLocalFile->lts.mss = (unsigned long)t.t_maxseg;
                CurrentLocalFile->lts.msss = (unsigned char)1;
                CurrentLocalFile->lts.topt = (unsigned int)t.t_flags;
#endif    /* defined(HASTCPOPT) */

            }
            break;
/*
 * Process a ROUTE domain socket.
 */
        case AF_ROUTE:
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "rte");
            if (s.so_pcb)
                enter_dev_ch(print_kptr((KA_T)(s.so_pcb), (char *) NULL, 0));
            else
                (void) snpf(NameChars, NameCharsLength, "no protocol control block");
            if (!OptSize)
                CurrentLocalFile->off_def = 1;
            break;
/*
 * Process a Unix domain socket.
 */
        case AF_UNIX:
            if (OptUnixSocket)
                CurrentLocalFile->sf |= SELUNX;
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "unix");
            /*
             * Read Unix protocol control block and the Unix address structure.
             */
            enter_dev_ch(print_kptr(sa, (char *) NULL, 0));
            if (kread((KA_T) s.so_pcb, (char *) &unp, sizeof(unp))) {
                (void) snpf(NameChars, NameCharsLength, "can't read unpcb at %s",
                            print_kptr((KA_T) s.so_pcb, (char *) NULL, 0));
                break;
            }
            if ((struct socket *) sa != unp.unp_socket) {
                (void) snpf(NameChars, NameCharsLength, "unp_socket (%s) mismatch",
                            print_kptr((KA_T) unp.unp_socket, (char *) NULL, 0));
                break;
            }
            if (unp.unp_addr) {
                if (kread((KA_T) unp.unp_addr, (char *) &mb, sizeof(mb))) {
                    (void) snpf(NameChars, NameCharsLength, "can't read unp_addr at %s",
                                print_kptr((KA_T) unp.unp_addr, (char *) NULL, 0));
                    break;
                }

#if    AIXV >= 3200
                uo = (int)(mb.m_hdr.mh_data - (caddr_t)unp.unp_addr);
                if ((uo + sizeof(struct sockaddr)) <= sizeof(mb))
                    ua = (struct sockaddr_un *)((char *)&mb + uo);
                else {
                    if (mb.m_hdr.mh_data
                    &&  !kread((KA_T)mb.m_hdr.mh_data, (char *)&un, sizeof(un))
                    ) {
                    ua = &un;
                    }
                }
#else	/* AIXV<3200 */
                ua = (struct sockaddr_un *) (((char *) &mb) + mb.m_off);
#endif    /* AIXV>=3200 */

            }
            if (!ua) {
                ua = &un;
                (void) bzero((char *) ua, sizeof(un));
                ua->sun_family = AF_UNSPEC;
            }
            /*
             * Print information on Unix socket that has no address bound
             * to it, although it may be connected to another Unix domain
             * socket as a pipe.
             */
            if (ua->sun_family != AF_UNIX) {
                if (ua->sun_family == AF_UNSPEC) {
                    if (unp.unp_conn) {
                        if (kread((KA_T) unp.unp_conn, (char *) &uc, sizeof(uc)))
                            (void) snpf(NameChars, NameCharsLength,
                                        "can't read unp_conn at %s",
                                        print_kptr((KA_T) unp.unp_conn, (char *) NULL, 0));
                        else
                            (void) snpf(NameChars, NameCharsLength, "->%s",
                                        print_kptr((KA_T) uc.unp_socket, (char *) NULL, 0));
                    } else
                        (void) snpf(NameChars, NameCharsLength, "->(none)");
                } else
                    (void) snpf(NameChars, NameCharsLength, "unknown sun_family (%d)",
                                ua->sun_family);
                break;
            }
            /*
             * Read any associated vnode and then read its gnode and inode.
             */
            g.gn_type = VSOCK;
            if (unp.unp_vnode
                && !readvnode((KA_T) unp.unp_vnode, &v)) {
                if (v.v_gnode
                    && !readgnode((KA_T) v.v_gnode, &g)) {
                    CurrentLocalFile->lock = isglocked(&g);
                    if (g.gn_type == VSOCK && g.gn_data
                        && !readlino(&g, &i))
                        is = 1;
                }
            }
            /*
             * Print Unix socket information.
             */
            if (is) {
                CurrentLocalFile->dev = i.dev;
                CurrentLocalFile->dev_def = i.dev_def;
                if (CurrentLocalFile->dev_ch) {
                    (void) free((FREE_P *) CurrentLocalFile->dev_ch);
                    CurrentLocalFile->dev_ch = (char *) NULL;
                }
                CurrentLocalFile->inode = (INODETYPE) i.number;
                CurrentLocalFile->inp_ty = i.number_def;
            }
            if (ua->sun_path[0]) {
                if (mb.m_len > sizeof(struct sockaddr_un))
                    mb.m_len = sizeof(struct sockaddr_un);
                *((char *) ua + mb.m_len - 1) = '\0';
                if (SearchFileChain && is_file_named(ua->sun_path, VSOCK, 0, 0))
                    CurrentLocalFile->sf |= SELNM;
                if (!NameChars[0])
                    (void) snpf(NameChars, NameCharsLength, "%s", ua->sun_path);
            } else
                (void) snpf(NameChars, NameCharsLength, "no address");
            break;

        default:
            printunkaf(fam, 1);
    }
    if (NameChars[0])
        enter_nm(NameChars);
}

/*
 * dsock.c - Darwin socket processing functions for /dev/kmem-based lsof
 */

/*
 * Special Darwin socket info: Justin Walker, 000927
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
static char *rcsid = "$Id: dsock.c,v 1.11 2005/11/01 20:24:51 abe Exp $";
#endif


#include "lsof.h"


#if    defined(HASIPv6)
/*
 * IPv6_2_IPv4()  -- macro to define the address of an IPv4 address contained
 *                 in an IPv6 address
 */

#define	IPv6_2_IPv4(v6)	(((uint8_t *)((struct in6_addr *)v6)->s6_addr)+12)
#endif    /* defined(HASIPv6) */


/*
 * process_socket() - process socket
 */

void
process_socket(sa)
        KA_T sa;            /* socket address in kernel */
{
    struct domain d;
    unsigned char *fa = (unsigned char *) NULL;
    int fam, lp;
    int fp = 0;
    struct inpcb inp;
    unsigned char *la = (unsigned char *) NULL;
    struct protosw p;
    struct socket s;
    struct tcpcb t;
    KA_T ta = (KA_T) NULL;
    struct unpcb uc, unp;
    struct sockaddr_un *ua = NULL;
    struct sockaddr_un un;
    int unl;

#if    defined(HASIPv6)
    struct in6pcb in6p;
#endif    /* defined(HASIPv6) */

#if    defined(AF_SYSTEM)
    struct kern_event_pcb kev_cb;
#endif    /* defined(AF_SYSTEM) */

#if    defined(AF_NDRV)
    char buf[IFNAMSIZ];
    struct ndrv_cb ndrv_cb;
    struct ifnet ifnet;
#endif    /* defined(AF_NDRV) */

    (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "sock");
    CurrentLocalFile->inp_ty = 2;
/*
 * Read the socket, protocol, and domain structures.
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
    if (!p.pr_domain
        || kread((KA_T) p.pr_domain, (char *) &d, sizeof(d))) {
        (void) snpf(NameChars, NameCharsLength, "can't read domain struct from %s",
                    print_kptr((KA_T) p.pr_domain, (char *) NULL, 0));
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
    CurrentLocalFile->lts.ltm = (unsigned int)(s.so_linger & 0xffff);
    CurrentLocalFile->lts.opt = (unsigned int)(s.so_options & 0xffff);
    CurrentLocalFile->lts.pqlen = (unsigned int)s.so_incqlen;
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
    switch ((fam = d.dom_family)) {
/*
 * Process an Internet domain socket.
 */
        case AF_INET:

#if    defined(HASIPv6)
            case AF_INET6:
#endif    /* defined(HASIPv6) */

            if (OptNetwork) {
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
        (fam == AF_INET) ? "IPv4" : "IPv6");
#else	/* !defined(HASIPv6) */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "inet");
#endif    /* defined(HASIPv6) */

#if    defined(HASIPv6)
        if (fam == AF_INET6) {

        /*
         * Read IPv6 protocol control block.
         */
        if (!s.so_pcb
        ||  kread((KA_T)s.so_pcb, (char *)&in6p, sizeof(in6p)))
        {
            (void) snpf(NameChars, NameCharsLength, "can't read in6pcb at %s",
            print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
            }
        /*
         * Save IPv6 address information.
         */
        enter_dev_ch(print_kptr((KA_T)(in6p.in6p_ppcb ? in6p.in6p_ppcb
                                  : s.so_pcb),
                           (char *)NULL, 0));
        if (p.pr_protocol == IPPROTO_TCP)
            ta = (KA_T)in6p.in6p_ppcb;
        la = (unsigned char *)&in6p.in6p_laddr;
        lp = (int)ntohs(in6p.in6p_lport);
        if (!IN6_IS_ADDR_UNSPECIFIED(&in6p.in6p_faddr)
        ||  in6p.in6p_fport)
        {
            fa = (unsigned char *)&in6p.in6p_faddr;
            fp = (int)ntohs(in6p.in6p_fport);
        }
        } else
#endif    /* defined(HASIPv6) */

            {

                /*
                 * Read IPv4 protocol control block.
                 */
                if (!s.so_pcb
                    || kread((KA_T) s.so_pcb, (char *) &inp, sizeof(inp))) {
                    if (!s.so_pcb) {
                        (void) snpf(NameChars, NameCharsLength, "no PCB%s%s",
                                    (s.so_state & SS_CANTSENDMORE) ? ", CANTSENDMORE"
                                                                   : "",
                                    (s.so_state & SS_CANTRCVMORE) ? ", CANTRCVMORE"
                                                                  : "");
                    } else {
                        (void) snpf(NameChars, NameCharsLength, "can't read inpcb at %s",
                                    print_kptr((KA_T) s.so_pcb, (char *) NULL, 0));
                    }
                    enter_nm(NameChars);
                    return;
                }
                /*
                 * Print Internet socket information.
                 */
                enter_dev_ch(print_kptr((KA_T)(inp.inp_ppcb ? inp.inp_ppcb
                                                            : s.so_pcb),
                                        (char *) NULL, 0));
                /*
                 * Save IPv4 address information.
                 */
                if (p.pr_protocol == IPPROTO_TCP)
                    ta = (KA_T) inp.inp_ppcb;
                la = (unsigned char *) &inp.inp_laddr;
                lp = (int) ntohs(inp.inp_lport);
                if (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport) {
                    fa = (unsigned char *) &inp.inp_faddr;
                    fp = (int) ntohs(inp.inp_fport);
                }
            }

#if    defined(HASIPv6)
        if ((fam == AF_INET6)
        &&  ((la && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)la))
        ||  ((fa && IN6_IS_ADDR_V4MAPPED((struct in6_addr *)fa))))) {

        /*
         * Adjust for IPv4 addresses mapped in IPv6 addresses.
         */
        if (la)
            la = (unsigned char *)IPv6_2_IPv4(la);
        if (fa)
            fa = (unsigned char *)IPv6_2_IPv4(fa);
        fam = AF_INET;
        }
#endif    /* defined(HASIPv6) */

            /*
             * Enter local and remote addresses by address family.
             */
            if (fa || la)
                (void) ent_inaddr(la, lp, fa, fp, fam);
            if (ta && !kread(ta, (char *) &t, sizeof(t))) {
                CurrentLocalFile->lts.type = 0;
                CurrentLocalFile->lts.state.i = (int) t.t_state;

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

#if    defined(AF_NDRV)
        /*
         * Process an NDRV domain socket.
         */
            case AF_NDRV:
            {
                (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "ndrv");
            /*
             * Read protocol control block.
             */
                if (!s.so_pcb
                ||  kread((KA_T)s.so_pcb, (char *)&ndrv_cb, sizeof(ndrv_cb))) {
                (void) snpf(NameChars, NameCharsLength, "can't read ndrv_cb at %s",
                    print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
                }
            /*
             * Print NDRV socket information.
             */
                enter_dev_ch(print_kptr((KA_T)(s.so_pcb), (char *)NULL, 0));
            /*
             * Print device name, if bound
             */
                if (!ndrv_cb.nd_if
                ||  kread((KA_T)ndrv_cb.nd_if, (char *)&ifnet, sizeof(ifnet))) {
                (void) snpf(NameChars, NameCharsLength, "can't read ifnet at %s",
                        print_kptr((KA_T)ndrv_cb.nd_if, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
                }
                if (!ifnet.if_name
                ||  kread((KA_T)ifnet.if_name, buf, sizeof(buf))) {
                (void) snpf(NameChars, NameCharsLength, "can't read ifnet.if_name at %s",
                        print_kptr((KA_T)ifnet.if_name, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
                }
                (void) snpf(NameChars, NameCharsLength, "-> %s%d", buf, ifnet.if_unit);
            }
            break;
#endif    /* defined(AF_NDRV) */

#if    defined(pseudo_AF_KEY)
        /*
         * Process an [internal] key-management function socket
         */
            case pseudo_AF_KEY:
                (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "key");
                break;
#endif    /* defined(pseudo_AF_KEY) */

#if    defined(AF_SYSTEM)
        /*
         * Process a SYSTEM domain socket
         */
            case AF_SYSTEM:
                (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "systm");
            /*
             * Read protocol control block.
             */
                if (!s.so_pcb
                ||  kread((KA_T)s.so_pcb, (char *)&kev_cb, sizeof(kev_cb))) {
                (void) snpf(NameChars, NameCharsLength, "can't read kev_cb at %s",
                    print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
                }
            /*
             * Print SYSTEM socket information.
             */
                enter_dev_ch(print_kptr((KA_T)(s.so_pcb), (char *)NULL, 0));
                (void) snpf(NameChars, NameCharsLength, "[%lx:%lx:%lx]",
                    kev_cb.vendor_code_filter,
                    kev_cb.class_filter, kev_cb.subclass_filter);
                break;
#endif    /* defined(AF_SYSTEM) */

#if    defined(AF_PPP)
        /*
         * Process a PPP domain socket
         */
            case AF_PPP:
                (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "ppp");
                break;
#endif    /* defined(AF_PPP) */

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
            if (!s.so_pcb
                || kread((KA_T) s.so_pcb, (char *) &unp, sizeof(unp))) {
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
                if (kread((KA_T) unp.unp_addr, (char *) &un, sizeof(un))) {
                    (void) snpf(NameChars, NameCharsLength, "can't read unp_addr at %s",
                                print_kptr((KA_T) unp.unp_addr, (char *) NULL, 0));
                    break;
                }
                ua = &un;
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
            if (ua->sun_path[0]) {
                unl = ua->sun_len - offsetof(
                struct sockaddr_un, sun_path);
                if ((unl < 0) || (unl >= sizeof(ua->sun_path)))
                    unl = sizeof(ua->sun_path) - 1;
                ua->sun_path[unl] = '\0';
                if (ua->sun_path[0] && SearchFileChain && is_file_named(ua->sun_path, 0))
                    CurrentLocalFile->sf |= SELNM;
                if (ua->sun_path[0] && !NameChars[0])
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

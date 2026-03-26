/*
 * dsock.c - FreeBSD socket processing functions for lsof
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
static char *rcsid = "$Id: dsock.c,v 1.28 2009/03/25 19:23:06 abe Exp $";
#endif


#include "lsof.h"


#if    defined(HASIPv6)

/*
 * IPv6_2_IPv4()  -- macro to define the address of an IPv4 address contained
 *		     in an IPv6 address
 */

#define IPv6_2_IPv4(v6)	(((uint8_t *)((struct in6_addr *)v6)->s6_addr)+12)

# if	defined(HAS_NO_6PORT)
/*
 * If the in_pcb structure no longer has the KAME accommodations of
 * in6p_[fl]port, redefine them to inp_[fl]port.
 */

#define	in6p_fport	inp_fport
#define	in6p_lport	inp_lport
# endif	/* defined(HAS_NO_6PORT) */

# if	defined(HAS_NO_6PPCB)
/*
 * If the in_pcb structure no longer has the KAME accommodation of in6p_pcb,
 * redefine it to inp_ppcb.
 */

#define	in6p_ppcb	inp_ppcb
# endif	/* defined(HAS_NO_6PPCB) */
#endif    /* defined(HASIPv6) */


/*
 * Local function prototypes
 */

_PROTOTYPE(static int ckstate, (KA_T
        ta,
        struct tcpcb *t,
        int fam));


/*
 * ckstate() -- read TCP control block and check TCP state for inclusion
 *		or exclusion
 * return: -1 == no TCP CB available
 *	    0 == TCP DB available; continue processing file
 *	    1 == stop processing file
 */

static int
ckstate(ta, t, fam)
        KA_T ta;            /* TCP control block address */
        struct tcpcb *t;        /* TCP control block receptor */
        int fam;            /* protocol family */
{
    int tsnx;
/*
 * Read TCP control block.
 */
    if (kread(ta, (char *) t, sizeof(struct tcpcb)))
        return (-1);
    if (TcpStateExcludeCount || TcpStateIncludeCount) {

        /*
         * If there are TCP state inclusions or exclusions, check them.
         */
        tsnx = (int) t->t_state + TcpStateOffset;
        if (TcpStateExcludeCount) {
            if (TcpStateExclude[tsnx]) {
                CurrentLocalFile->sf &= ~SELNET;
                CurrentLocalFile->sf |= SELEXCLF;
                return (1);
            }
        }
        if (TcpStateIncludeCount) {
            if (TcpStateInclude[tsnx]) {
                TcpStateInclude[tsnx] = 2;
                CurrentLocalFile->sf |= SELNET;
            } else {
                CurrentLocalFile->sf &= ~SELNET;
                CurrentLocalFile->sf |= SELEXCLF;
                return (1);
            }
        }
    }
    if (!(CurrentLocalFile->sf & SELNET) && !TcpStateIncludeCount) {

        /*
         * See if this TCP file should be selected.
         */
        if (OptNetwork) {
            if (!OptNetworkType
                || (OptNetworkType == 4) && (fam == AF_INET)

#if    defined(HASIPv6)
                ||  (OptNetworkType == 6) && (fam == AF_INET6)
#endif    /* defined(HASIPv6) */

                    ) {
                CurrentLocalFile->sf |= SELNET;
            }
        }
    }
    return (0);
}


/*
 * process_socket() - process socket
 */

void
process_socket(sa)
        KA_T sa;            /* socket address in kernel */
{
    struct domain d;
    unsigned char *fa = (unsigned char *) NULL;
    int fam;
    int fp, lp;
    struct inpcb inp;
    unsigned char *la = (unsigned char *) NULL;
    struct protosw p;
    struct socket s;
    struct tcpcb t;
    int ts = -1;
    struct unpcb uc, unp;
    struct sockaddr_un *ua = NULL;
    struct sockaddr_un un;

#if    FREEBSDV < 4050
    struct mbuf mb;
#else	/* FREEBSDV>=4050 */
    int unl;
#endif    /* FREEBSDV<4050 */

#if    defined(HASIPv6) && !defined(HASINRIAIPv6)
    struct in6pcb in6p;
#endif    /* defined(HASIPv6) && !defined(HASINRIAIPv6) */

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
    CurrentLocalFile->lts.ltm = (unsigned int)s.so_linger;
    CurrentLocalFile->lts.opt = (unsigned int)s.so_options;
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
# if	defined(HASSBSTATE)
    CurrentLocalFile->lts.sbs_rcv = s.so_rcv.sb_state;
    CurrentLocalFile->lts.sbs_snd = s.so_snd.sb_state;
# endif	/* defined(HASSBSTATE) */
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

                        ) {
                    if (!TcpStateIncludeCount && !UdpStateIncludeCount)
                        CurrentLocalFile->sf |= SELNET;
                }
            }
            printiproto(p.pr_protocol);

#if    defined(HASIPv6)
        (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type),
        (fam == AF_INET) ? "IPv4" : "IPv6");
#else	/* !defined(HASIPv6) */
            (void) snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "inet");
#endif    /* defined(HASIPv6) */

#if    defined(HASIPv6) && !defined(HASINRIAIPv6)
        if (fam == AF_INET6) {

        /*
         * Read IPv6 protocol control block.
         */
        if (!s.so_pcb
        ||  kread((KA_T)s.so_pcb, (char *)&in6p, sizeof(in6p))) {
            (void) snpf(NameChars, NameCharsLength, "can't read in6pcb at %s",
            print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
            enter_nm(NameChars);
            return;
        }
        /*
         * Save IPv6 address information.
         */
        if (p.pr_protocol == IPPROTO_TCP) {
            if (in6p.in6p_ppcb) {
            if ((ts = ckstate((KA_T)in6p.in6p_ppcb, &t, fam)) == 1)
                return;
            }
        }
        enter_dev_ch(print_kptr((KA_T)(in6p.in6p_ppcb ? in6p.in6p_ppcb
                                  : s.so_pcb),
                           (char *)NULL, 0));
            la = (unsigned char *)&in6p.in6p_laddr;
            lp = (int)ntohs(in6p.in6p_lport);
        if (!IN6_IS_ADDR_UNSPECIFIED(&in6p.in6p_faddr)
        ||  in6p.in6p_fport)
        {
            fa = (unsigned char *)&in6p.in6p_faddr;
            fp = (int)ntohs(in6p.in6p_fport);
        }
        } else
#endif    /* defined(HASIPv6) && !defined(HASINRIAIPv6) */

            {

                /*
                 * Read Ipv4 or IPv6 (INRIA) protocol control block.
                 */
                if (!s.so_pcb
                    || kread((KA_T) s.so_pcb, (char *) &inp, sizeof(inp))) {
                    if (!s.so_pcb) {
                        (void) snpf(NameChars, NameCharsLength, "no PCB%s%s",

#if    defined(HASSBSTATE)
                                (s.so_snd.sb_state & SBS_CANTSENDMORE) ?
#else	/* !defined(HASSBSTATE) */
                                    (s.so_state & SS_CANTSENDMORE) ?
                                    #endif    /* defined(HASSBSTATE) */

                                    ", CANTSENDMORE" : "",
#if    defined(HASSBSTATE)
                                (s.so_rcv.sb_state & SBS_CANTRCVMORE) ?
#else	/* !defined(HASSBSTATE) */
                                    (s.so_state & SS_CANTRCVMORE) ?
                                    #endif    /* defined(HASSBSTATE) */

                                    ", CANTRCVMORE" : "");
                    } else {
                        (void) snpf(NameChars, NameCharsLength, "can't read inpcb at %s",
                                    print_kptr((KA_T) s.so_pcb, (char *) NULL, 0));
                    }
                    enter_nm(NameChars);
                    return;
                }
                if (p.pr_protocol == IPPROTO_TCP) {
                    if (inp.inp_ppcb) {
                        if ((ts = ckstate((KA_T) inp.inp_ppcb, &t, fam)) == 1)
                            return;
                    }
                }
                enter_dev_ch(print_kptr((KA_T)(inp.inp_ppcb ? inp.inp_ppcb
                                                            : s.so_pcb),
                                        (char *) NULL, 0));
                lp = (int) ntohs(inp.inp_lport);
                if (fam == AF_INET) {

                    /*
                     * Save IPv4 address information.
                     */
                    la = (unsigned char *) &inp.inp_laddr;
                    if (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport) {
                        fa = (unsigned char *) &inp.inp_faddr;
                        fp = (int) ntohs(inp.inp_fport);
                    }
                }

#if    defined(HASIPv6) && defined(HASINRIAIPv6)
                else {
                    la = (unsigned char *)&inp.inp_laddr6;
                    if (!IN6_IS_ADDR_UNSPECIFIED(&inp.inp_faddr6)
                    ||  inp.inp_fport)
                    {
                    fa = (unsigned char *)&inp.inp_faddr6;
                    fp = (int)ntohs(inp.inp_fport);
                    }
                }
#endif    /* defined(HASIPv6) && defined(HASINRIAIPv6) */

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
            if (ts == 0) {
                CurrentLocalFile->lts.type = 0;
                CurrentLocalFile->lts.state.i = (int) t.t_state;

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

#if    FREEBSDV < 4050
                if (kread((KA_T) unp.unp_addr, (char *) &mb, sizeof(mb)))
#else	/* FREEBSDV>=4050 */
                    if (kread((KA_T)unp.unp_addr, (char *)&un, sizeof(un)))
#endif    /* FREEBSDV<4050 */

                {
                    (void) snpf(NameChars, NameCharsLength, "can't read unp_addr at %s",
                                print_kptr((KA_T) unp.unp_addr, (char *) NULL, 0));
                    break;
                }

#if    FREEBSDV < 4050
                if (mb.m_hdr.mh_len == sizeof(struct sockaddr_un))
                    ua = (struct sockaddr_un *) ((char *) &mb
                                                 + (mb.m_hdr.mh_data - (caddr_t) unp.unp_addr));
#else	/* FREEBSDV>=4050 */
                ua = &un;
#endif    /* FREEBSDV<4050 */

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

#if    FREEBSDV < 4050
                if (mb.m_len >= sizeof(struct sockaddr_un))
                    mb.m_len = sizeof(struct sockaddr_un) - 1;
                *((char *) ua + mb.m_len) = '\0';
#else	/* FREEBSDV>=4050 */
# if	FREEBSDV>4060
                unl = ua->sun_len - offsetof(struct sockaddr_un, sun_path);
# else	/* FREEBSDV<4060 */
                unl = sizeof(ua->sun_path) - 1;
# endif	/* FREEBSDV>4060 */
                if ((unl < 0) || (unl >= sizeof(ua->sun_path)))
                    unl = sizeof(ua->sun_path) - 1;
                ua->sun_path[unl] = '\0';
#endif    /* FREEBSDV<4050 */

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

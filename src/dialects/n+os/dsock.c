/*
 * dsock.c - NEXTSTEP and OPENSTEP socket processing functions for lsof
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
 * process_socket() - process socket
 */

void process_socket(KA_T sa) {
    struct domain d;
    char *ep;
    unsigned char *fa = (unsigned char *)NULL;
    int fam;
    int fp, lp;
    struct inpcb inp;
    unsigned char *la = (unsigned char *)NULL;
    struct mbuf mb;
    struct protosw p;
    struct rawcb raw;
    struct socket s;
    size_t sz;
    struct tcpcb t;
    struct unpcb uc, unp;
    struct sockaddr_un *ua = NULL;
    struct sockaddr_un un;

    (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "sock");
    CurrentLocalFile->inp_ty = 2;
    /*
 * Read socket structure.
 */
    if (!sa) {
        enter_nm("no socket address");
        return;
    }
    if (kread((KA_T)sa, (char *)&s, sizeof(s))) {
        (void)snpf(NameChars, NameCharsLength, "can't read socket struct from %#x", sa);
        enter_nm(NameChars);
        return;
    }
    if (!s.so_type) {
        enter_nm("no socket type");
        return;
    }
    /*
 * Read protocol switch and domain structures.
 */
    if (!s.so_proto || kread((KA_T)s.so_proto, (char *)&p, sizeof(p))) {
        (void)snpf(NameChars, NameCharsLength, "no protocol switch");
        enter_nm(NameChars);
        return;
    }
    if (kread((KA_T)p.pr_domain, (char *)&d, sizeof(d))) {
        (void)snpf(NameChars, NameCharsLength, "can't read domain struct from %s",
                   print_kptr((KA_T)p.pr_domain, (char *)NULL, 0));
        enter_nm(NameChars);
        return;
    }
    /*
 * Save size information.
 */
    if (OptSize) {
        if (CurrentLocalFile->access == 'r')
            CurrentLocalFile->sz = (SZOFFTYPE)s.so_rcv.sb_cc;
        else if (CurrentLocalFile->access == 'w')
            CurrentLocalFile->sz = (SZOFFTYPE)s.so_snd.sb_cc;
        else
            CurrentLocalFile->sz = (SZOFFTYPE)(s.so_rcv.sb_cc + s.so_snd.sb_cc);
        CurrentLocalFile->sz_def = 1;
    } else
        CurrentLocalFile->off_def = 1;

#if defined(HASTCPTPIQ)
    CurrentLocalFile->lts.recv_queue = (unsigned long)s.so_rcv.sb_cc;
    CurrentLocalFile->lts.send_queue = (unsigned long)s.so_snd.sb_cc;
    CurrentLocalFile->lts.recv_queue_st = CurrentLocalFile->lts.send_queue_st = 1;
#endif /* defined(HASTCPTPIQ) */

#if defined(HASSOOPT)
    CurrentLocalFile->lts.ltm = (unsigned int)s.so_linger;
    CurrentLocalFile->lts.opt = (unsigned int)s.so_options;
    CurrentLocalFile->lts.pqlen = (unsigned int)s.so_q0len;
    CurrentLocalFile->lts.qlen = (unsigned int)s.so_qlen;
    CurrentLocalFile->lts.qlim = (unsigned int)s.so_qlimit;
    CurrentLocalFile->lts.rbsz = (unsigned long)s.so_rcv.sb_mbmax;
    CurrentLocalFile->lts.sbsz = (unsigned long)s.so_snd.sb_mbmax;
    CurrentLocalFile->lts.pqlens = CurrentLocalFile->lts.qlens = CurrentLocalFile->lts.qlims =
        CurrentLocalFile->lts.rbszs = CurrentLocalFile->lts.sbszs = (unsigned char)1;
#endif /* defined(HASSOOPT) */

#if defined(HASSOSTATE)
    CurrentLocalFile->lts.sock_state = (unsigned int)s.so_state;
#endif /* defined(HASSOSTATE) */

    /*
 * Process socket by the associated domain family.
 */
    switch ((fam = d.dom_family)) {
        /*
 * Process an Internet domain socket.
 */
    case AF_INET:
        if (OptNetwork)
            CurrentLocalFile->sel_flags |= SELNET;
        (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "inet");
        printiproto(p.pr_protocol);
        /*
             * Read protocol control block.
             */
        if (!s.so_pcb) {
            (void)snpf(NameChars, NameCharsLength, "no PCB%s%s",
                       (s.so_state & SS_CANTSENDMORE) ? ", CANTSENDMORE" : "",
                       (s.so_state & SS_CANTRCVMORE) ? ", CANTRCVMORE" : "");
            enter_nm(NameChars);
            return;
        }
        if (s.so_type == SOCK_RAW) {

            /*
                 * Print raw socket information.
                 */
            if (kread((KA_T)s.so_pcb, (char *)&raw, sizeof(raw)) ||
                (struct socket *)sa != raw.rcb_socket) {
                (void)snpf(NameChars, NameCharsLength, "can't read rawcb at %s",
                           print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
            }
            enter_dev_ch(print_kptr((KA_T)(raw.rcb_pcb ? raw.rcb_pcb : s.so_pcb), (char *)NULL, 0));
            if (raw.rcb_laddr.sa_family == AF_INET)
                la = (unsigned char *)&raw.rcb_laddr.sa_data[2];
            else if (raw.rcb_laddr.sa_family)
                printrawaddr(&raw.rcb_laddr);
            if (raw.rcb_faddr.sa_family == AF_INET)
                fa = (unsigned char *)&raw.rcb_faddr.sa_data[2];
            else if (raw.rcb_faddr.sa_family) {
                ep = endnm(&sz);
                (void)snpf(ep, sz, "->");
                printrawaddr(&raw.rcb_faddr);
            }
            if (fa || la)
                (void)ent_inaddr(la, -1, fa, -1, AF_INET);
        } else {

            /*
                 * Print Internet socket information.
                 */
            if (kread((KA_T)s.so_pcb, (char *)&inp, sizeof(inp)) ||
                (struct socket *)sa != inp.inp_socket) {
                (void)snpf(NameChars, NameCharsLength, "can't read inpcb at %s",
                           print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
                enter_nm(NameChars);
                return;
            }
            enter_dev_ch(
                print_kptr((KA_T)(inp.inp_ppcb ? inp.inp_ppcb : s.so_pcb), (char *)NULL, 0));
            /*
                 * If the protocol is TCP, try to read the TCP protocol
                 * control block to record its state.
                 */
            if (p.pr_protocol == IPPROTO_TCP && inp.inp_ppcb &&
                kread((KA_T)inp.inp_ppcb, (char *)&t, sizeof(t)) == 0) {
                CurrentLocalFile->lts.type = 0;
                CurrentLocalFile->lts.state.val = (int)t.t_state;

#if defined(HASTCPOPT)
                CurrentLocalFile->lts.mss = (unsigned long)t.t_maxseg;
                CurrentLocalFile->lts.msss = (unsigned char)1;
                CurrentLocalFile->lts.topt = (unsigned int)t.t_flags;
#endif /* defined(HASTCPOPT) */
            }
            /*
                 * Process the local and foreign addresses from the Internet
                 * control block.
                 */
            la = (unsigned char *)&inp.inp_laddr;
            lp = (int)ntohs(inp.inp_lport);
            if (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport != 0) {
                fa = (unsigned char *)&inp.inp_faddr;
                fp = (int)ntohs(inp.inp_fport);
            }
            if (fa || la)
                (void)ent_inaddr(la, lp, fa, fp, AF_INET);
        }
        break;
        /*
 * Process a Unix domain socket.
 */
    case AF_UNIX:
        if (OptUnixSocket)
            CurrentLocalFile->sel_flags |= SELUNX;
        (void)snpf(CurrentLocalFile->type, sizeof(CurrentLocalFile->type), "unix");
        /*
             * Read Unix protocol control block and the Unix address structure.
             */
        enter_dev_ch(print_kptr(sa, (char *)NULL, 0));
        if (kread((KA_T)s.so_pcb, (char *)&unp, sizeof(unp))) {
            (void)snpf(NameChars, NameCharsLength, "can't read unpcb at %s",
                       print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
            break;
        }
        if ((struct socket *)sa != unp.unp_socket) {
            (void)snpf(NameChars, NameCharsLength, "unp_socket (%s) mismatch",
                       print_kptr((KA_T)unp.unp_socket, (char *)NULL, 0));
            break;
        }
        if (unp.unp_addr) {
            if (kread((KA_T)unp.unp_addr, (char *)&mb, sizeof(mb))) {
                (void)snpf(NameChars, NameCharsLength, "can't read unp_addr at %s",
                           print_kptr((KA_T)unp.unp_addr, (char *)NULL, 0));
                break;
            }
            ua = (struct sockaddr_un *)(((char *)&mb) + mb.m_off);
        }
        if (!ua) {
            ua = &un;
            (void)bzero((char *)ua, sizeof(un));
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
                    if (kread((KA_T)unp.unp_conn, (char *)&uc, sizeof(uc))) {
                        (void)snpf(NameChars, NameCharsLength, "can't read unp_conn at %s",
                                   print_kptr((KA_T)unp.unp_conn, (char *)NULL, 0));
                    } else {
                        (void)snpf(NameChars, NameCharsLength, "->%s",
                                   print_kptr((KA_T)uc.unp_socket, (char *)NULL, 0));
                    }
                } else
                    (void)snpf(NameChars, NameCharsLength, "->(none)");
            } else
                (void)snpf(NameChars, NameCharsLength, "unknown sun_family (%d)", ua->sun_family);
            break;
        }
        if (ua->sun_path[0]) {
            if (mb.m_len >= sizeof(struct sockaddr_un))
                mb.m_len = sizeof(struct sockaddr_un) - 1;
            *((char *)ua + mb.m_len) = '\0';
            if (SearchFileChain && is_file_named(ua->sun_path, 0))
                CurrentLocalFile->sel_flags |= SELNM;
            if (!NameChars[0])
                (void)snpf(NameChars, NameCharsLength, "%s", ua->sun_path);
        } else
            (void)snpf(NameChars, NameCharsLength, "no address");
        break;
    default:
        printunkaf(fam, 1);
    }
    if (NameChars[0])
        enter_nm(NameChars);
}

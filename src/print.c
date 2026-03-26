/*
 * print.c - common print support functions for lsof
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

#ifndef lint
static char copyright[] =
        "@(#) Copyright 1994 lsof contributors.\nAll rights reserved.\n";
#endif


#include "lsof.h"


/*
 * Local definitions, structures and function prototypes
 */

#define HCINC        64        /* host cache size increase chunk */
#define PORTHASHBUCKETS    128        /* port hash bucket count
					 * !!MUST BE A POWER OF 2!! */
#define    PORTTABTHRESH    10        /* threshold at which we will switch
					 * from using getservbyport() to
					 * getservent() -- see lkup_port()
					 * and fill_porttab() */

struct hostcache {
    unsigned char a[MAX_AF_ADDR];    /* numeric address */
    int af;                /* address family -- e.g., AF_INET
					 * or AF_INET6 */
    char *name;            /* name */
};

struct porttab {
    int port;
    MALLOC_S nl;            /* name length (excluding '\0') */
    int ss;                /* service name status, 0 = lookup not
					 * yet performed */
    char *name;
    struct porttab *next;
};


#if    defined(HASNORPC_H)
static struct porttab **Pth[2] = { NULL, NULL };
                        /* port hash buckets:
                         * Pth[0] for TCP service names
                         * Pth[1] for UDP service names
                         */
#else	/* !defined(HASNORPC_H) */
static struct porttab **Pth[4] = {NULL, NULL, NULL, NULL};
/* port hash buckets:
 * Pth[0] for TCP service names
 * Pth[1] for UDP service names
 * Pth[2] for TCP portmap info
 * Pth[3] for UDP portmap info
 */
#endif    /* defined(HASNORPC_H) */

#define HASHPORT(p)    (((((int)(p)) * 31415) >> 3) & (PORTHASHBUCKETS - 1))


#if    !defined(HASNORPC_H)

_PROTOTYPE(static void fill_portmap, (void));

_PROTOTYPE(static void update_portmap, (struct porttab *pt, char *pn));

#endif    /* !defined(HASNORPC_H) */

_PROTOTYPE(static void fill_porttab, (void));

_PROTOTYPE(static char *lkup_port, (int port, int protocol, int src));

_PROTOTYPE(static char *lkup_svcnam, (int hash_idx, int port, int protocol, int svc_status));

_PROTOTYPE(static int printinaddr, (void));


/*
 * endnm() - locate end of NameChars
 */

char *
endnm(remaining_size)
        size_t *remaining_size;            /* returned remaining size */
{
    size_t len = strlen(NameChars);
    *remaining_size = NameCharsLength - len;
    return (NameChars + len);
}


#if !defined(HASNORPC_H)
/*
 * fill_portmap() -- fill the RPC portmap program name table via a conversation
 *		     with the portmapper
 *
 * The following copyright notice acknowledges that this function was adapted
 * from getrpcportnam() of the source code of the OpenBSD netstat program.
 */

/*
* Copyright (c) 1983, 1988, 1993
*      The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by the University of
*      California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

static void
fill_portmap() {
    char buf[128], *char_ptr, *name;
    CLIENT *client;
    int hash_idx, port, protocol;
    MALLOC_S name_len;
    struct pmaplist *p = (struct pmaplist *) NULL;
    struct porttab *port_entry;
    struct rpcent *rpc_entry;
    struct TIMEVAL_LSOF timeout;

#if    !defined(CAN_USE_CLNT_CREATE)
    struct hostent *host_entry;
    struct sockaddr_in sock_addr;
    int sock = RPC_ANYSOCK;
#endif    /* !defined(CAN_USE_CLNT_CREATE) */

/*
 * Construct structures for communicating with the portmapper.
 */

#if    !defined(CAN_USE_CLNT_CREATE)
    zeromem(&sock_addr, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    if ((host_entry = gethostbyname("localhost")))
        MEMMOVE((caddr_t) & sock_addr.sin_addr, host_entry->h_addr, host_entry->h_length);
    sock_addr.sin_port = htons(PMAPPORT);
#endif    /* !defined(CAN_USE_CLNT_CREATE) */

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
/*
 * Get an RPC client handle.  Then ask for a dump of the port map.
 */

#if    defined(CAN_USE_CLNT_CREATE)
    if (!(client = clnt_create("localhost", PMAPPROG, PMAPVERS, "tcp")))
#else	/* !defined(CAN_USE_CLNT_CREATE) */
    if (!(client = clnttcp_create(&sock_addr, PMAPPROG, PMAPVERS, &sock, 0, 0)))
#endif    /* defined(CAN_USE_CLNT_CREATE) */

        return;
    if (clnt_call(client, PMAPPROC_DUMP, XDR_VOID, NULL, XDR_PMAPLIST,
                  (caddr_t) & p, timeout)
        != RPC_SUCCESS) {
        clnt_destroy(client);
        return;
    }
/*
 * Loop through the port map dump, creating portmap table entries from TCP
 * and UDP members.
 */
    for (; p; p = p->pml_next) {

        /*
         * Determine the port map entry's protocol; ignore all but TCP and UDP.
         */
        if (p->pml_map.pm_prot == IPPROTO_TCP)
            protocol = 2;
        else if (p->pml_map.pm_prot == IPPROTO_UDP)
            protocol = 3;
        else
            continue;
        /*
         * See if there's already a portmap entry for this port.  If there is,
         * ignore this entry.
         */
        hash_idx = HASHPORT((port = (int) p->pml_map.pm_port));
        for (port_entry = Pth[protocol][hash_idx]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port)
                break;
        }
        if (port_entry)
            continue;
        /*
         * Save the registration name or number.
         */
        char_ptr = (char *) NULL;
        if ((rpc_entry = (struct rpcent *) getrpcbynumber(p->pml_map.pm_prog))) {
            if (rpc_entry->r_name && strlen(rpc_entry->r_name))
                char_ptr = rpc_entry->r_name;
        }
        if (!char_ptr) {
            (void) snpf(buf, sizeof(buf), "%lu",
                        (unsigned long) p->pml_map.pm_prog);
            char_ptr = buf;
        }
        if (!strlen(char_ptr))
            continue;
        /*
         * Allocate space for the portmap name entry and copy it there.
         */
        if (!(name = mkstrcpy(char_ptr, &name_len))) {
            (void) fprintf(stderr,
                           "%s: can't allocate space for portmap entry: ", ProgramName);
            safestrprt(char_ptr, stderr, 1);
            Exit(1);
        }
        if (!name_len) {
            (void) free((FREE_P *) name);
            continue;
        }
        /*
         * Allocate and fill a porttab struct entry for the portmap table.
         * Link it to the head of its hash bucket, and make it the new head.
         */
        if (!(port_entry = (struct porttab *) malloc(sizeof(struct porttab)))) {
            (void) fprintf(stderr,
                           "%s: can't allocate porttab entry for portmap: ", ProgramName);
            safestrprt(name, stderr, 1);
            (void) free((FREE_P *) name);
            Exit(1);
        }
        port_entry->name = name;
        port_entry->nl = name_len;
        port_entry->port = port;
        port_entry->next = Pth[protocol][hash_idx];
        port_entry->ss = 0;
        Pth[protocol][hash_idx] = port_entry;
    }
    clnt_destroy(client);
}

#endif    /* !defined(HASNORPC_H) */


/*
 * fill_porttab() -- fill the TCP and UDP service name port table with a
 *		     getservent() scan
 */

static void
fill_porttab() {
    int hash_idx, port, protocol;
    MALLOC_S name_len;
    char *name;
    struct porttab *port_entry;
    struct servent *serv_entry;

    (void) endservent();
/*
 * Scan the services data base for TCP and UDP entries that have a non-null
 * name associated with them.
 */
    (void) setservent(1);
    while ((serv_entry = getservent())) {
        if (!serv_entry->s_name || !serv_entry->s_proto)
            continue;
        if (strcasecmp(serv_entry->s_proto, "TCP") == 0)
            protocol = 0;
        else if (strcasecmp(serv_entry->s_proto, "UDP") == 0)
            protocol = 1;
        else
            continue;
        if (!serv_entry->s_name || !strlen(serv_entry->s_name))
            continue;
        port = ntohs(serv_entry->s_port);
        /*
         * See if a port->service entry is already cached for this port and
         * prototcol.  If it is, leave it alone.
         */
        hash_idx = HASHPORT(port);
        for (port_entry = Pth[protocol][hash_idx]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port == port)
                break;
        }
        if (port_entry)
            continue;
        /*
         * Add a new entry to the cache for this port and protocol.
         */
        if (!(name = mkstrcpy(serv_entry->s_name, &name_len))) {
            (void) fprintf(stderr,
                           "%s: can't allocate %d bytes for port %d name: %s\n",
                           ProgramName, (int) (name_len + 1), port, serv_entry->s_name);
            Exit(1);
        }
        if (!name_len) {
            (void) free((FREE_P *) name);
            continue;
        }
        if (!(port_entry = (struct porttab *) malloc(sizeof(struct porttab)))) {
            (void) fprintf(stderr,
                           "%s: can't allocate porttab entry for port %d: %s\n",
                           ProgramName, port, serv_entry->s_name);
            (void) free((FREE_P *) name);
            Exit(1);
        }
        port_entry->name = name;
        port_entry->nl = name_len - 1;
        port_entry->port = port;
        port_entry->next = Pth[protocol][hash_idx];
        port_entry->ss = 0;
        Pth[protocol][hash_idx] = port_entry;
    }
    (void) endservent();
}


/*
 * gethostnm() - get host name
 */

char *
gethostnm(inet_addr, addr_family)
        unsigned char *inet_addr;        /* Internet address */
        int addr_family;        /* address family -- e.g., AF_INET
					 * or AF_INET6 */
{
    int addr_len = MIN_AF_ADDR;
    char hbuf[256];
    static struct hostcache *hc = (struct hostcache *) NULL;
    static int hcx = 0;
    char *hn, *np;
    struct hostent *host_entry = (struct hostent *) NULL;
    int i, j;
    MALLOC_S len;
    static int nhc = 0;
/*
 * Search cache.
 */

#if    defined(HASIPv6)
    if (addr_family == AF_INET6)
        addr_len = MAX_AF_ADDR;
#endif    /* defined(HASIPv6) */

    for (i = 0; i < hcx; i++) {
        if (addr_family != hc[i].af)
            continue;
        for (j = 0; j < addr_len; j++) {
            if (inet_addr[j] != hc[i].a[j])
                break;
        }
        if (j >= addr_len)
            return (hc[i].name);
    }
/*
 * If -n has been specified, construct a numeric address.  Otherwise, look up
 * host name by address.  If that fails, or if there is no name in the returned
 * hostent structure, construct a numeric version of the address.
 */
    if (OptHostLookup)
        host_entry = gethostbyaddr((char *) inet_addr, addr_len, addr_family);
    if (!host_entry || !host_entry->h_name) {

#if    defined(HASIPv6)
        if (addr_family == AF_INET6) {

        /*
         * Since IPv6 numeric addresses use `:' as a separator, enclose
         * them in brackets.
         */
        hbuf[0] = '[';
        if (!inet_ntop(addr_family, inet_addr, hbuf + 1, sizeof(hbuf) - 3)) {
            (void) snpf(&hbuf[1], (sizeof(hbuf) - 1),
            "can't format IPv6 address]");
        } else {
            len = strlen(hbuf);
            (void) snpf(&hbuf[len], sizeof(hbuf) - len, "]");
        }
        } else
#endif    /* defined(HASIPv6) */

        if (addr_family == AF_INET)
            (void) snpf(hbuf, sizeof(hbuf), "%u.%u.%u.%u", inet_addr[0], inet_addr[1],
                        inet_addr[2], inet_addr[3]);
        else
            (void) snpf(hbuf, sizeof(hbuf), "(unknown AF value: %d)", addr_family);
        hn = hbuf;
    } else
        hn = (char *) host_entry->h_name;
/*
 * Allocate space for name and copy name to it.
 */
    if (!(np = mkstrcpy(hn, (MALLOC_S *) NULL))) {
        (void) fprintf(stderr, "%s: no space for host name: ", ProgramName);
        safestrprt(hn, stderr, 1);
        Exit(1);
    }
/*
 * Add address/name entry to cache.  Allocate cache space in HCINC chunks.
 */
    if (hcx >= nhc) {
        nhc += HCINC;
        len = (MALLOC_S)(nhc * sizeof(struct hostcache));
        if (!hc)
            hc = (struct hostcache *) malloc(len);
        else {
            struct hostcache *tmp;
            tmp = (struct hostcache *) realloc((MALLOC_P *) hc, len);
            if (!tmp) {
                (void) free((FREE_P *) hc);
                hc = (struct hostcache *) NULL;
            } else
                hc = tmp;
        }
        if (!hc) {
            (void) fprintf(stderr, "%s: no space for host cache\n", ProgramName);
            Exit(1);
        }
    }
    hc[hcx].af = addr_family;
    for (i = 0; i < addr_len; i++) {
        hc[hcx].a[i] = inet_addr[i];
    }
    hc[hcx++].name = np;
    return (np);
}


/*
 * lkup_port() - look up port for protocol
 */

static char *
lkup_port(port, protocol, src)
        int port;            /* port number */
        int protocol;            /* protocol index: 0 = tcp, 1 = udp */
        int src;            /* port source: 0 = local
					 *		1 = foreign */
{
    int hash_idx, i, nh;
    MALLOC_S name_len;
    char *name, *pn;
    static char pb[128];
    static int pm = 0;
    struct porttab *port_entry;
/*
 * If the hash buckets haven't been allocated, do so.
 */
    if (!Pth[0]) {

#if    defined(HASNORPC_H)
        nh = 2;
#else	/* !defined(HASNORPC_H) */
        nh = OptPortMap ? 4 : 2;
#endif    /* defined(HASNORPC_H) */

        for (hash_idx = 0; hash_idx < nh; hash_idx++) {
            if (!(Pth[hash_idx] = (struct porttab **) calloc(PORTHASHBUCKETS,
                                                      sizeof(struct porttab *)))) {
                (void) fprintf(stderr,
                               "%s: can't allocate %d bytes for %s %s hash buckets\n",
                               ProgramName,
                               (int) (2 * (PORTHASHBUCKETS * sizeof(struct porttab *))),
                               (hash_idx & 1) ? "UDP" : "TCP",
                               (hash_idx > 1) ? "portmap" : "port");
                for (i = 0; i < hash_idx; i++) {
                    (void) free((FREE_P *) Pth[i]);
                    Pth[i] = (struct porttab **) NULL;
                }
                Exit(1);
            }
        }
    }

#if    !defined(HASNORPC_H)
/*
 * If we're looking up program names for portmapped ports, make sure the
 * portmap table has been loaded.
 */
    if (OptPortMap && !pm) {
        (void) fill_portmap();
        pm++;
    }
#endif    /* !defined(HASNORPC_H) */

/*
 * Hash the port and see if its name has been cached.  Look for a local
 * port first in the portmap, if portmap searching is enabled.
 */
    hash_idx = HASHPORT(port);

#if    !defined(HASNORPC_H)
    if (!src && OptPortMap) {
        for (port_entry = Pth[protocol + 2][hash_idx]; port_entry; port_entry = port_entry->next) {
            if (port_entry->port != port)
                continue;
            if (!port_entry->ss) {
                pn = OptPortLookup ? lkup_svcnam(hash_idx, port, protocol, 0) : (char *) NULL;
                if (!pn) {
                    (void) snpf(pb, sizeof(pb), "%d", port);
                    pn = pb;
                }
                (void) update_portmap(port_entry, pn);
            }
            return (port_entry->name);
        }
    }
#endif    /* !defined(HASNORPC_H) */

    for (port_entry = Pth[protocol][hash_idx]; port_entry; port_entry = port_entry->next) {
        if (port_entry->port == port)
            return (port_entry->name);
    }
/*
 * Search for a possible service name, unless the -P option has been specified.
 *
 * If there is no service name, return a %d conversion.
 *
 * Don't cache %d conversions; a zero port number is a %d conversion that
 * is represented by "*".
 */
    pn = OptPortLookup ? lkup_svcnam(hash_idx, port, protocol, 1) : (char *) NULL;
    if (!pn || !strlen(pn)) {
        if (port) {
        /*
         * Fast integer-to-string for port numbers (avoids snpf overhead).
         */
            char *ep = &pb[sizeof(pb) - 1];
            int value = port;
            *ep = '\0';
            do { *--ep = '0' + (value % 10); value /= 10; } while (value);
            /* shift to start of pb for stable return pointer */
            {
                int dlen = (int)(&pb[sizeof(pb) - 1] - ep);
                if (ep != pb) {
                    int k;
                    for (k = 0; k < dlen; k++) pb[k] = ep[k];
                    pb[dlen] = '\0';
                }
            }
            return (pb);
        } else
            return ("*");
    }
/*
 * Allocate a new porttab entry for the TCP or UDP service name.
 */
    if (!(port_entry = (struct porttab *) malloc(sizeof(struct porttab)))) {
        (void) fprintf(stderr,
                       "%s: can't allocate porttab entry for port %d\n", ProgramName, port);
        Exit(1);
    }
/*
 * Allocate space for the name; copy it to the porttab entry; and link the
 * porttab entry to its hash bucket.
 *
 * Return a pointer to the name.
 */
    if (!(name = mkstrcpy(pn, &name_len))) {
        (void) fprintf(stderr,
                       "%s: can't allocate space for port name: ", ProgramName);
        safestrprt(pn, stderr, 1);
        Exit(1);
    }
    port_entry->name = name;
    port_entry->nl = name_len;
    port_entry->port = port;
    port_entry->next = Pth[protocol][hash_idx];
    port_entry->ss = 0;
    Pth[protocol][hash_idx] = port_entry;
    return (name);
}


/*
 * lkup_svcnam() - look up service name for port
 */

static char *
lkup_svcnam(hash_idx, port, protocol, svc_status)
        int hash_idx;            /* porttab hash index */
        int port;            /* port number */
        int protocol;            /* protocol: 0 = TCP, 1 = UDP */
        int svc_status;            /* search status: 1 = Pth[protocol][hash_idx]
					 *		  already searched */
{
    static int fl[PORTTABTHRESH];
    static int fln = 0;
    static int gsbp = 0;
    int i;
    struct porttab *port_entry;
    static int ptf = 0;
    struct servent *serv_entry;
/*
 * Do nothing if -P has been specified.
 */
    if (!OptPortLookup)
        return ((char *) NULL);

    for (;;) {

        /*
         * Search service name cache, if it hasn't already been done.
         * Return the name of a match.
         */
        if (!svc_status) {
            for (port_entry = Pth[protocol][hash_idx]; port_entry; port_entry = port_entry->next) {
                if (port_entry->port == port)
                    return (port_entry->name);
            }
        }
/*
 * If fill_porttab() has been called, there is no service name.
 *
 * Do PORTTABTHRES getservbport() calls, remembering the failures, so they
 * won't be repeated.
 *
 * After PORTABTHRESH getservbyport() calls, call fill_porttab() once,
 */
        if (ptf)
            break;
        if (gsbp < PORTTABTHRESH) {
            for (i = 0; i < fln; i++) {
                if (fl[i] == port)
                    return ((char *) NULL);
            }
            gsbp++;
            if ((serv_entry = getservbyport(htons(port), protocol ? "udp" : "tcp")))
                return (serv_entry->s_name);
            if (fln < PORTTABTHRESH)
                fl[fln++] = port;
            return ((char *) NULL);
        }
        (void) fill_porttab();
        ptf++;
        svc_status = 0;
    }
    return ((char *) NULL);
}


/*
 * print_file() - print file
 */

void
print_file() {
    char buf[128];
    char *char_ptr = (char *) NULL;
    dev_t dev;
    int devs, len;

    if (PrintPass && !HeaderPrinted) {

        /*
         * Print the header line if this is the second pass and the
         * header hasn't already been printed.
         */
        (void) printf("%-*.*s %*s", CommandColWidth, CommandColWidth, CMDTTL, PidColWidth,
                      PIDTTL);

#if    defined(HASTASKS)
        if (TaskPrintFlag)
        (void) printf(" %*s", TidColWidth, TIDTTL);
#endif    /* defined(HASTASKS) */

#if	defined(HASZONES)
	    if (OptZone)
		(void) printf(" %-*s", ZoneColWidth, ZONETTL);
#endif	/* defined(HASZONES) */

#if    defined(HASSELINUX)
        if (OptSecContext)
        (void) printf(" %-*s", ContextColWidth, CNTXTTL);
#endif /* defined(HASSELINUX) */

#if    defined(HASPPID)
        if (OptParentPid)
         (void) printf(" %*s", PpidColWidth, PPIDTTL);
#endif    /* defined(HASPPID) */

        if (OptProcessGroup)
            (void) printf(" %*s", PgidColWidth, PGIDTTL);
        (void) printf(" %*s %*s   %*s",
                      UserColWidth, USERTTL,
                      FileDescColWidth - 2, FDTTL,
                      TypeColWidth, TYPETTL);

#if    defined(HASFSTRUCT)
        if (OptFileStructValues) {

# if	!defined(HASNOFSADDR)
        if (OptFileStructValues & FSV_FILE_ADDR)
            (void) printf(" %*s", FileStructAddrColWidth, FSTTL);
# endif	/* !defined(HASNOFSADDR) */

# if	!defined(HASNOFSCOUNT)
        if (OptFileStructValues & FSV_FILE_COUNT)
            (void) printf(" %*s", FileCountColWidth, FCTTL);
# endif	/* !defined(HASNOFSCOUNT) */

# if	!defined(HASNOFSFLAGS)
        if (OptFileStructValues & FSV_FILE_FLAGS)
            (void) printf(" %*s", FileFlagColWidth, FGTTL);
# endif	/* !defined(HASNOFSFLAGS) */

# if	!defined(HASNOFSNADDR)
        if (OptFileStructValues & FSV_NODE_ID)
            (void) printf(" %*s", NodeIdColWidth, NodeIdTitle);
# endif	/* !defined(HASNOFSNADDR) */

        }
#endif    /* defined(HASFSTRUCT) */

        (void) printf(" %*s", DeviceColWidth, DEVTTL);
        if (OptOffset)
            (void) printf(" %*s", SizeOffColWidth, OFFTTL);
        else if (OptSize)
            (void) printf(" %*s", SizeOffColWidth, SZTTL);
        else
            (void) printf(" %*s", SizeOffColWidth, SZOFFTTL);
        if (OptLinkCount)
            (void) printf(" %*s", LinkCountColWidth, NLTTL);
        (void) printf(" %*s %s\n", NodeColWidth, NODETTL, NMTTL);
        HeaderPrinted++;
    }
/*
 * Size or print the command.
 */
    char_ptr = (CurrentLocalProc->cmd && *CurrentLocalProc->cmd != '\0') ? CurrentLocalProc->cmd : "(unknown)";
    if (!PrintPass) {
        len = safestrlen(char_ptr, 2);
        if (CommandColLimit && (len > CommandColLimit))
            len = CommandColLimit;
        if (len > CommandColWidth)
            CommandColWidth = len;
    } else
        safestrprtn(char_ptr, CommandColWidth, stdout, 2);
/*
 * Size or print the process ID.
 */
    if (!PrintPass) {
        (void) snpf(buf, sizeof(buf), "%d", CurrentLocalProc->pid);
        if ((len = strlen(buf)) > PidColWidth)
            PidColWidth = len;
    } else
        (void) printf(" %*d", PidColWidth, CurrentLocalProc->pid);

#if    defined(HASTASKS)
    /*
     * Size or print task ID.
     */
        if (!PrintPass) {
            if (CurrentLocalProc->tid) {
            (void) snpf(buf, sizeof(buf), "%d", CurrentLocalProc->tid);
            if ((len = strlen(buf)) > TidColWidth)
                TidColWidth = len;
            TaskPrintFlag = 1;
            }
        } else if (TaskPrintFlag) {
            if (CurrentLocalProc->tid)
            (void) printf(" %*d", TidColWidth, CurrentLocalProc->tid);
            else
            (void) printf(" %*s", TidColWidth, "");
        }
#endif    /* defined(HASTASKS) */

#if    defined(HASZONES)
    /*
     * Size or print the zone.
     */
        if (OptZone) {
            if (!PrintPass) {
            if (CurrentLocalProc->zn) {
                if ((len = strlen(CurrentLocalProc->zn)) > ZoneColWidth)
                ZoneColWidth = len;
            }
            } else
            (void) printf(" %-*s", ZoneColWidth, CurrentLocalProc->zn ? CurrentLocalProc->zn : "");
        }
#endif    /* defined(HASZONES) */

#if    defined(HASSELINUX)
    /*
     * Size or print the context.
     */
        if (OptSecContext) {
            if (!PrintPass) {
            if (CurrentLocalProc->cntx) {
                if ((len = strlen(CurrentLocalProc->cntx)) > ContextColWidth)
                ContextColWidth = len;
            }
            } else
            (void) printf(" %-*s", ContextColWidth, CurrentLocalProc->cntx ? CurrentLocalProc->cntx : "");
        }
#endif    /* defined(HASSELINUX) */

#if    defined(HASPPID)
    if (OptParentPid) {

    /*
     * Size or print the parent process ID.
     */
        if (!PrintPass) {
        (void) snpf(buf, sizeof(buf), "%d", CurrentLocalProc->ppid);
        if ((len = strlen(buf)) > PpidColWidth)
            PpidColWidth = len;
        } else
        (void) printf(" %*d", PpidColWidth, CurrentLocalProc->ppid);
    }
#endif    /* defined(HASPPID) */

    if (OptProcessGroup) {

        /*
         * Size or print the process group ID.
         */
        if (!PrintPass) {
            (void) snpf(buf, sizeof(buf), "%d", CurrentLocalProc->pgid);
            if ((len = strlen(buf)) > PgidColWidth)
                PgidColWidth = len;
        } else
            (void) printf(" %*d", PgidColWidth, CurrentLocalProc->pgid);
    }
/*
 * Size or print the user ID or login name.
 */
    if (!PrintPass) {
        if ((len = strlen(printuid((UID_ARG) CurrentLocalProc->uid, NULL))) > UserColWidth)
            UserColWidth = len;
    } else
        (void) printf(" %*.*s", UserColWidth, UserColWidth,
                      printuid((UID_ARG) CurrentLocalProc->uid, NULL));
/*
 * Size or print the file descriptor, access mode and lock status.
 */
    if (!PrintPass) {
        (void) snpf(buf, sizeof(buf), "%s%c%c",
                    CurrentLocalFile->fd,
                    (CurrentLocalFile->lock == ' ') ? CurrentLocalFile->access
                                      : (CurrentLocalFile->access == ' ') ? '-'
                                                            : CurrentLocalFile->access,
                    CurrentLocalFile->lock);
        if ((len = strlen(buf)) > FileDescColWidth)
            FileDescColWidth = len;
    } else
        (void) printf(" %*.*s%c%c", FileDescColWidth - 2, FileDescColWidth - 2, CurrentLocalFile->fd,
                      (CurrentLocalFile->lock == ' ') ? CurrentLocalFile->access
                                        : (CurrentLocalFile->access == ' ') ? '-'
                                                              : CurrentLocalFile->access,
                      CurrentLocalFile->lock);
/*
 * Size or print the type.
 */
    if (!PrintPass) {
        if ((len = strlen(CurrentLocalFile->type)) > TypeColWidth)
            TypeColWidth = len;
    } else
        (void) printf(" %*.*s", TypeColWidth, TypeColWidth, CurrentLocalFile->type);

#if    defined(HASFSTRUCT)
    /*
     * Size or print the file structure address, file usage count, and node
     * ID (address).
     */

        if (OptFileStructValues) {

# if	!defined(HASNOFSADDR)
            if (OptFileStructValues & FSV_FILE_ADDR) {
            char_ptr =  (CurrentLocalFile->fsv & FSV_FILE_ADDR) ? print_kptr(CurrentLocalFile->fsa, buf, sizeof(buf))
                         : "";
            if (!PrintPass) {
                if ((len = strlen(char_ptr)) > FileStructAddrColWidth)
                FileStructAddrColWidth = len;
            } else
                (void) printf(" %*.*s", FileStructAddrColWidth, FileStructAddrColWidth, char_ptr);

            }
# endif	/* !defined(HASNOFSADDR) */

# if	!defined(HASNOFSCOUNT)
            if (OptFileStructValues & FSV_FILE_COUNT) {
            if (CurrentLocalFile->fsv & FSV_FILE_COUNT) {
                (void) snpf(buf, sizeof(buf), "%ld", CurrentLocalFile->fct);
                char_ptr = buf;
            } else
                char_ptr = "";
            if (!PrintPass) {
                if ((len = strlen(char_ptr)) > FileCountColWidth)
                FileCountColWidth = len;
            } else
                (void) printf(" %*.*s", FileCountColWidth, FileCountColWidth, char_ptr);
            }
# endif	/* !defined(HASNOFSCOUNT) */

# if	!defined(HASNOFSFLAGS)
            if (OptFileStructValues & FSV_FILE_FLAGS) {
            if ((CurrentLocalFile->fsv & FSV_FILE_FLAGS) && (OptFileStructFlagHex || CurrentLocalFile->ffg || CurrentLocalFile->pof))
                char_ptr = print_fflags(CurrentLocalFile->ffg, CurrentLocalFile->pof);
            else
                char_ptr = "";
            if (!PrintPass) {
                if ((len = strlen(char_ptr)) > FileFlagColWidth)
                FileFlagColWidth = len;
            } else
                (void) printf(" %*.*s", FileFlagColWidth, FileFlagColWidth, char_ptr);
            }
# endif	/* !defined(HASNOFSFLAGS) */

# if	!defined(HASNOFSNADDR)
            if (OptFileStructValues & FSV_NODE_ID) {
            char_ptr = (CurrentLocalFile->fsv & FSV_NODE_ID) ? print_kptr(CurrentLocalFile->fna, buf, sizeof(buf))
                        : "";
            if (!PrintPass) {
                if ((len = strlen(char_ptr)) > NodeIdColWidth)
                NodeIdColWidth = len;
            } else
                (void) printf(" %*.*s", NodeIdColWidth, NodeIdColWidth, char_ptr);
            }
# endif	/* !defined(HASNOFSNADDR) */

        }
#endif    /* defined(HASFSTRUCT) */

/*
 * Size or print the device information.
 */

    if (CurrentLocalFile->rdev_def) {
        dev = CurrentLocalFile->rdev;
        devs = 1;
    } else if (CurrentLocalFile->dev_def) {
        dev = CurrentLocalFile->dev;
        devs = 1;
    } else
        devs = 0;
    if (devs) {

#if    defined(HASPRINTDEV)
        char_ptr = HASPRINTDEV(CurrentLocalFile, &dev);
#else	/* !defined(HASPRINTDEV) */
        (void) snpf(buf, sizeof(buf), "%u,%u", GET_MAJ_DEV(dev),
                    GET_MIN_DEV(dev));
        char_ptr = buf;
#endif    /* defined(HASPRINTDEV) */

    }

    if (!PrintPass) {
        if (devs)
            len = strlen(char_ptr);
        else if (CurrentLocalFile->dev_ch)
            len = strlen(CurrentLocalFile->dev_ch);
        else
            len = 0;
        if (len > DeviceColWidth)
            DeviceColWidth = len;
    } else {
        if (devs)
            (void) printf(" %*.*s", DeviceColWidth, DeviceColWidth, char_ptr);
        else {
            if (CurrentLocalFile->dev_ch)
                (void) printf(" %*.*s", DeviceColWidth, DeviceColWidth, CurrentLocalFile->dev_ch);
            else
                (void) printf(" %*.*s", DeviceColWidth, DeviceColWidth, "");
        }
    }
/*
 * Size or print the size or offset.
 */
    if (!PrintPass) {
        if (CurrentLocalFile->sz_def) {

#if    defined(HASPRINTSZ)
            char_ptr = HASPRINTSZ(CurrentLocalFile);
#else	/* !defined(HASPRINTSZ) */
            (void) snpf(buf, sizeof(buf), SizeOffFormatD, CurrentLocalFile->sz);
            char_ptr = buf;
#endif    /* defined(HASPRINTSZ) */

            len = strlen(char_ptr);
        } else if (CurrentLocalFile->off_def) {

#if    defined(HASPRINTOFF)
            char_ptr = HASPRINTOFF(CurrentLocalFile, 0);
#else	/* !defined(HASPRINTOFF) */
            (void) snpf(buf, sizeof(buf), SizeOffFormat0t, CurrentLocalFile->off);
            char_ptr = buf;
#endif    /* defined(HASPRINTOFF) */

            len = strlen(char_ptr);
            if (OffsetDecDigitLimit && len > (OffsetDecDigitLimit + 2)) {

#if    defined(HASPRINTOFF)
                char_ptr = HASPRINTOFF(CurrentLocalFile, 1);
#else	/* !defined(HASPRINTOFF) */
                (void) snpf(buf, sizeof(buf), SizeOffFormatX, CurrentLocalFile->off);
                char_ptr = buf;
#endif    /* defined(HASPRINTOFF) */

                len = strlen(char_ptr);
            }
        } else
            len = 0;
        if (len > SizeOffColWidth)
            SizeOffColWidth = len;
    } else {
        putchar(' ');
        if (CurrentLocalFile->sz_def)

#if    defined(HASPRINTSZ)
            (void) printf("%*.*s", SizeOffColWidth, SizeOffColWidth, HASPRINTSZ(CurrentLocalFile));
#else	/* !defined(HASPRINTSZ) */
            (void) printf(SizeOffFormatDv, SizeOffColWidth, CurrentLocalFile->sz);
#endif    /* defined(HASPRINTSZ) */

        else if (CurrentLocalFile->off_def) {

#if    defined(HASPRINTOFF)
            char_ptr = HASPRINTOFF(CurrentLocalFile, 0);
#else	/* !defined(HASPRINTOFF) */
            (void) snpf(buf, sizeof(buf), SizeOffFormat0t, CurrentLocalFile->off);
            char_ptr = buf;
#endif    /* defined(HASPRINTOFF) */

            if (OffsetDecDigitLimit && (int) strlen(char_ptr) > (OffsetDecDigitLimit + 2)) {

#if    defined(HASPRINTOFF)
                char_ptr = HASPRINTOFF(CurrentLocalFile, 1);
#else	/* !defined(HASPRINTOFF) */
                (void) snpf(buf, sizeof(buf), SizeOffFormatX, CurrentLocalFile->off);
                char_ptr = buf;
#endif    /* defined(HASPRINTOFF) */

            }
            (void) printf("%*.*s", SizeOffColWidth, SizeOffColWidth, char_ptr);
        } else
            (void) printf("%*.*s", SizeOffColWidth, SizeOffColWidth, "");
    }
/*
 * Size or print the link count.
 */
    if (OptLinkCount) {
        if (CurrentLocalFile->nlink_def) {
            (void) snpf(buf, sizeof(buf), " %ld", CurrentLocalFile->nlink);
            char_ptr = buf;
        } else
            char_ptr = "";
        if (!PrintPass) {
            if ((len = strlen(char_ptr)) > LinkCountColWidth)
                LinkCountColWidth = len;
        } else
            (void) printf(" %*s", LinkCountColWidth, char_ptr);
    }
/*
 * Size or print the inode information.
 */
    switch (CurrentLocalFile->inp_ty) {
        case 1:

#if    defined(HASPRINTINO)
            char_ptr = HASPRINTINO(CurrentLocalFile);
#else	/* !defined(HASPRINTINO) */
            (void) snpf(buf, sizeof(buf), InodeFormatDecimal, CurrentLocalFile->inode);
            char_ptr = buf;
#endif    /* defined(HASPRINTINO) */

            break;
        case 2:
            if (CurrentLocalFile->iproto[0])
                char_ptr = CurrentLocalFile->iproto;
            else
                char_ptr = "";
            break;
        case 3:
            (void) snpf(buf, sizeof(buf), InodeFormatHex, CurrentLocalFile->inode);
            char_ptr = buf;
            break;
        default:
            char_ptr = "";
    }
    if (!PrintPass) {
        if ((len = strlen(char_ptr)) > NodeColWidth)
            NodeColWidth = len;
    } else {
        (void) printf(" %*.*s", NodeColWidth, NodeColWidth, char_ptr);
    }
/*
 * If this is the second pass, print the name column.  (It doesn't need
 * to be sized.)
 */
    if (PrintPass) {
        putchar(' ');

#if    defined(HASPRINTNM)
        HASPRINTNM(CurrentLocalFile);
#else	/* !defined(HASPRINTNM) */
        printname(1);
#endif    /* defined(HASPRINTNM) */

    }
}


/*
 * printinaddr() - print Internet addresses
 */

static int
printinaddr() {
    int i, len, src;
    char *host, *port;
    int name_len = NameCharsLength - 1;
    char *np = NameChars;
    char pbuf[32];
/*
 * Process local network address first.  If there's a foreign address,
 * separate it from the local address with "->".
 */
    for (i = 0, *np = '\0'; i < 2; i++) {
        if (!CurrentLocalFile->li[i].af)
            continue;
        host = port = (char *) NULL;
        if (i) {

            /*
             * If this is the foreign address, insert the separator.
             */
            if (name_len < 2)

                addr_too_long:

                {
                    (void) snpf(NameChars, NameCharsLength,
                                "network addresses too long");
                    return (1);
                }
            *np++ = '-';
            *np++ = '>';
            name_len -= 2;
        }
        /*
         * Convert the address to a host name.
         */

#if    defined(HASIPv6)
        if ((CurrentLocalFile->li[i].af == AF_INET6
        &&   IN6_IS_ADDR_UNSPECIFIED(&CurrentLocalFile->li[i].ia.a6))
        ||  (CurrentLocalFile->li[i].af == AF_INET
        &&    CurrentLocalFile->li[i].ia.a4.s_addr == INADDR_ANY))
        host ="*";
        else
        host = gethostnm((unsigned char *)&CurrentLocalFile->li[i].ia, CurrentLocalFile->li[i].af);
#else /* !defined(HASIPv6) */
        if (CurrentLocalFile->li[i].ia.a4.s_addr == INADDR_ANY)
            host = "*";
        else
            host = gethostnm((unsigned char *) &CurrentLocalFile->li[i].ia, CurrentLocalFile->li[i].af);
#endif    /* defined(HASIPv6) */

        /*
         * Process the port number.
         */
        if (CurrentLocalFile->li[i].p > 0) {

            if (OptPortLookup

                #if    !defined(HASNORPC_H)
                || OptPortMap
#endif    /* defined(HASNORPC_H) */

                    ) {

                /*
                 * If converting port numbers to service names, or looking
                 * up portmap program names and numbers, do so by protocol.
                 *
                 * Identify the port source as local if: 1) it comes from the
                 * local entry (0) of the file's Internet address array; or
                 * 2) it comes from  the foreign entry (1), and the foreign
                 * Internet address matches the local one; or 3) it is the
                 * loopback address 127.0.0.1.  (Test 2 may not always work
                 * -- e.g., on hosts with multiple interfaces.)
                 */
#if    !defined(HASNORPC_H)
                if ((src = i) && OptPortMap) {

# if    defined(HASIPv6)
                    if (CurrentLocalFile->li[0].af == AF_INET6) {
                        if (IN6_IS_ADDR_LOOPBACK(&CurrentLocalFile->li[i].ia.a6)
                        ||  IN6_ARE_ADDR_EQUAL(&CurrentLocalFile->li[0].ia.a6,
                                   &CurrentLocalFile->li[1].ia.a6)
                        )
                        src = 0;
                    } else
# endif    /* defined(HASIPv6) */

                    if (CurrentLocalFile->li[0].af == AF_INET) {
                        if (CurrentLocalFile->li[i].ia.a4.s_addr == htonl(INADDR_LOOPBACK)
                            || CurrentLocalFile->li[0].ia.a4.s_addr == CurrentLocalFile->li[1].ia.a4.s_addr
                                )
                            src = 0;
                    }
                }
#endif    /* !defined(HASNORPC_H) */

                if (strcasecmp(CurrentLocalFile->iproto, "TCP") == 0)
                    port = lkup_port(CurrentLocalFile->li[i].p, 0, src);
                else if (strcasecmp(CurrentLocalFile->iproto, "UDP") == 0)
                    port = lkup_port(CurrentLocalFile->li[i].p, 1, src);
            }
            if (!port) {
                (void) snpf(pbuf, sizeof(pbuf), "%d", CurrentLocalFile->li[i].p);
                port = pbuf;
            }
        } else if (CurrentLocalFile->li[i].p == 0)
            port = "*";
        /*
         * Enter the host name.
         */
        if (host) {
            if ((len = strlen(host)) > name_len)
                goto addr_too_long;
            if (len) {
                (void) snpf(np, name_len, "%s", host);
                np += len;
                name_len -= len;
            }
        }
        /*
         * Enter the port number, preceded by a colon.
         */
        if (port) {
            if (((len = strlen(port)) + 1) >= name_len)
                goto addr_too_long;
            (void) snpf(np, name_len, ":%s", port);
            np += len + 1;
            name_len -= len - 1;
        }
    }
    if (NameChars[0]) {
        safestrprt(NameChars, stdout, 0);
        return (1);
    }
    return (0);
}


/*
 * print_init() - initialize for printing
 */

void
print_init() {
    PrintPass = (OptFieldOutput || OptTerse) ? 1 : 0;
    CommandColWidth = strlen(CMDTTL);
    DeviceColWidth = strlen(DEVTTL);
    FileDescColWidth = strlen(FDTTL);
    if (OptLinkCount)
        LinkCountColWidth = strlen(NLTTL);
    NameColWidth = strlen(NMTTL);
    NodeColWidth = strlen(NODETTL);
    PgidColWidth = strlen(PGIDTTL);
    PidColWidth = strlen(PIDTTL);
    PpidColWidth = strlen(PPIDTTL);
    if (OptSize)
        SizeOffColWidth = strlen(SZTTL);
    else if (OptOffset)
        SizeOffColWidth = strlen(OFFTTL);
    else
        SizeOffColWidth = strlen(SZOFFTTL);
    TaskPrintFlag = 0;

#if    defined(HASTASKS)
    TidColWidth = strlen(TIDTTL);
#endif    /* defined(HASTASKS) */

    TypeColWidth = strlen(TYPETTL);
    UserColWidth = strlen(USERTTL);

#if    defined(HASFSTRUCT)

# if	!defined(HASNOFSADDR)
    FileStructAddrColWidth = strlen(FSTTL);
# endif	/* !defined(HASNOFSADDR) */

# if	!defined(HASNOFSCOUNT)
    FileCountColWidth = strlen(FCTTL);
# endif	/* !defined(HASNOFSCOUNT) */

# if	!defined(HASNOFSFLAGS)
    FileFlagColWidth = strlen(FGTTL);
# endif	/* !defined(HASNOFSFLAGS) */

# if	!defined(HASNOFSNADDR)
    NodeIdColWidth = strlen(NodeIdTitle);
# endif	/* !defined(HASNOFSNADDR) */
#endif    /* defined(HASFSTRUCT) */

#if    defined(HASSELINUX)
    if (OptSecContext)
        ContextColWidth = strlen(CNTXTTL);
#endif    /* defined(HASSELINUX) */

#if    defined(HASZONES)
    if (OptZone)
        ZoneColWidth = strlen(ZONETTL);
#endif    /* defined(HASZONES) */

}


#if    !defined(HASPRIVPRIPP)
/*
 * printiproto() - print Internet protocol name
 */

void
printiproto(proto)
        int proto;                /* protocol number */
{
    int i;
    static int m = -1;
    char *s;

    switch (proto) {

#if    defined(IPPROTO_TCP)
        case IPPROTO_TCP:
            s = "TCP";
            break;
#endif    /* defined(IPPROTO_TCP) */

#if    defined(IPPROTO_UDP)
        case IPPROTO_UDP:
            s = "UDP";
            break;
#endif    /* defined(IPPROTO_UDP) */

#if    defined(IPPROTO_IP)
# if	!defined(IPPROTO_HOPOPTS) || IPPROTO_IP!=IPPROTO_HOPOPTS
        case IPPROTO_IP:
            s = "IP";
            break;
# endif	/* !defined(IPPROTO_HOPOPTS) || IPPROTO_IP!=IPPROTO_HOPOPTS */
#endif    /* defined(IPPROTO_IP) */

#if    defined(IPPROTO_ICMP)
        case IPPROTO_ICMP:
            s = "ICMP";
            break;
#endif    /* defined(IPPROTO_ICMP) */

#if    defined(IPPROTO_ICMPV6)
        case IPPROTO_ICMPV6:
            s = "ICMPV6";
            break;
#endif    /* defined(IPPROTO_ICMPV6) */

#if    defined(IPPROTO_IGMP)
        case IPPROTO_IGMP:
            s = "IGMP";
            break;
#endif    /* defined(IPPROTO_IGMP) */

#if    defined(IPPROTO_GGP)
        case IPPROTO_GGP:
            s = "GGP";
            break;
#endif    /* defined(IPPROTO_GGP) */

#if    defined(IPPROTO_EGP)
        case IPPROTO_EGP:
            s = "EGP";
            break;
#endif    /* defined(IPPROTO_EGP) */

#if    defined(IPPROTO_PUP)
        case IPPROTO_PUP:
            s = "PUP";
            break;
#endif    /* defined(IPPROTO_PUP) */

#if    defined(IPPROTO_IDP)
        case IPPROTO_IDP:
            s = "IDP";
            break;
#endif    /* defined(IPPROTO_IDP) */

#if    defined(IPPROTO_ND)
        case IPPROTO_ND:
            s = "ND";
            break;
#endif    /* defined(IPPROTO_ND) */

#if    defined(IPPROTO_RAW)
        case IPPROTO_RAW:
            s = "RAW";
            break;
#endif    /* defined(IPPROTO_RAW) */

#if    defined(IPPROTO_HELLO)
        case IPPROTO_HELLO:
            s = "HELLO";
            break;
#endif    /* defined(IPPROTO_HELLO) */

#if    defined(IPPROTO_PXP)
        case IPPROTO_PXP:
            s = "PXP";
            break;
#endif    /* defined(IPPROTO_PXP) */

#if    defined(IPPROTO_RAWIP)
        case IPPROTO_RAWIP:
            s = "RAWIP";
            break;
#endif    /* defined(IPPROTO_RAWIP) */

#if    defined(IPPROTO_RAWIF)
        case IPPROTO_RAWIF:
            s = "RAWIF";
            break;
#endif    /* defined(IPPROTO_RAWIF) */

#if    defined(IPPROTO_HOPOPTS)
        case IPPROTO_HOPOPTS:
            s = "HOPOPTS";
            break;
#endif    /* defined(IPPROTO_HOPOPTS) */

#if    defined(IPPROTO_IPIP)
        case IPPROTO_IPIP:
            s = "IPIP";
            break;
#endif    /* defined(IPPROTO_IPIP) */

#if    defined(IPPROTO_ST)
        case IPPROTO_ST:
            s = "ST";
            break;
#endif    /* defined(IPPROTO_ST) */

#if    defined(IPPROTO_PIGP)
        case IPPROTO_PIGP:
            s = "PIGP";
            break;
#endif    /* defined(IPPROTO_PIGP) */

#if    defined(IPPROTO_RCCMON)
        case IPPROTO_RCCMON:
            s = "RCCMON";
            break;
#endif    /* defined(IPPROTO_RCCMON) */

#if    defined(IPPROTO_NVPII)
        case IPPROTO_NVPII:
            s = "NVPII";
            break;
#endif    /* defined(IPPROTO_NVPII) */

#if    defined(IPPROTO_ARGUS)
        case IPPROTO_ARGUS:
            s = "ARGUS";
            break;
#endif    /* defined(IPPROTO_ARGUS) */

#if    defined(IPPROTO_EMCON)
        case IPPROTO_EMCON:
            s = "EMCON";
            break;
#endif    /* defined(IPPROTO_EMCON) */

#if    defined(IPPROTO_XNET)
        case IPPROTO_XNET:
            s = "XNET";
            break;
#endif    /* defined(IPPROTO_XNET) */

#if    defined(IPPROTO_CHAOS)
        case IPPROTO_CHAOS:
            s = "CHAOS";
            break;
#endif    /* defined(IPPROTO_CHAOS) */

#if    defined(IPPROTO_MUX)
        case IPPROTO_MUX:
            s = "MUX";
            break;
#endif    /* defined(IPPROTO_MUX) */

#if    defined(IPPROTO_MEAS)
        case IPPROTO_MEAS:
            s = "MEAS";
            break;
#endif    /* defined(IPPROTO_MEAS) */

#if    defined(IPPROTO_HMP)
        case IPPROTO_HMP:
            s = "HMP";
            break;
#endif    /* defined(IPPROTO_HMP) */

#if    defined(IPPROTO_PRM)
        case IPPROTO_PRM:
            s = "PRM";
            break;
#endif    /* defined(IPPROTO_PRM) */

#if    defined(IPPROTO_TRUNK1)
        case IPPROTO_TRUNK1:
            s = "TRUNK1";
            break;
#endif    /* defined(IPPROTO_TRUNK1) */

#if    defined(IPPROTO_TRUNK2)
        case IPPROTO_TRUNK2:
            s = "TRUNK2";
            break;
#endif    /* defined(IPPROTO_TRUNK2) */

#if    defined(IPPROTO_LEAF1)
        case IPPROTO_LEAF1:
            s = "LEAF1";
            break;
#endif    /* defined(IPPROTO_LEAF1) */

#if    defined(IPPROTO_LEAF2)
        case IPPROTO_LEAF2:
            s = "LEAF2";
            break;
#endif    /* defined(IPPROTO_LEAF2) */

#if    defined(IPPROTO_RDP)
        case IPPROTO_RDP:
            s = "RDP";
            break;
#endif    /* defined(IPPROTO_RDP) */

#if    defined(IPPROTO_IRTP)
        case IPPROTO_IRTP:
            s = "IRTP";
            break;
#endif    /* defined(IPPROTO_IRTP) */

#if    defined(IPPROTO_TP)
        case IPPROTO_TP:
            s = "TP";
            break;
#endif    /* defined(IPPROTO_TP) */

#if    defined(IPPROTO_BLT)
        case IPPROTO_BLT:
            s = "BLT";
            break;
#endif    /* defined(IPPROTO_BLT) */

#if    defined(IPPROTO_NSP)
        case IPPROTO_NSP:
            s = "NSP";
            break;
#endif    /* defined(IPPROTO_NSP) */

#if    defined(IPPROTO_INP)
        case IPPROTO_INP:
            s = "INP";
            break;
#endif    /* defined(IPPROTO_INP) */

#if    defined(IPPROTO_SEP)
        case IPPROTO_SEP:
            s = "SEP";
            break;
#endif    /* defined(IPPROTO_SEP) */

#if    defined(IPPROTO_3PC)
        case IPPROTO_3PC:
            s = "3PC";
            break;
#endif    /* defined(IPPROTO_3PC) */

#if    defined(IPPROTO_IDPR)
        case IPPROTO_IDPR:
            s = "IDPR";
            break;
#endif    /* defined(IPPROTO_IDPR) */

#if    defined(IPPROTO_XTP)
        case IPPROTO_XTP:
            s = "XTP";
            break;
#endif    /* defined(IPPROTO_XTP) */

#if    defined(IPPROTO_DDP)
        case IPPROTO_DDP:
            s = "DDP";
            break;
#endif    /* defined(IPPROTO_DDP) */

#if    defined(IPPROTO_CMTP)
        case IPPROTO_CMTP:
            s = "CMTP";
            break;
#endif    /* defined(IPPROTO_CMTP) */

#if    defined(IPPROTO_TPXX)
        case IPPROTO_TPXX:
            s = "TPXX";
            break;
#endif    /* defined(IPPROTO_TPXX) */

#if    defined(IPPROTO_IL)
        case IPPROTO_IL:
            s = "IL";
            break;
#endif    /* defined(IPPROTO_IL) */

#if    defined(IPPROTO_IPV6)
        case IPPROTO_IPV6:
            s = "IPV6";
            break;
#endif    /* defined(IPPROTO_IPV6) */

#if    defined(IPPROTO_SDRP)
        case IPPROTO_SDRP:
            s = "SDRP";
            break;
#endif    /* defined(IPPROTO_SDRP) */

#if    defined(IPPROTO_ROUTING)
        case IPPROTO_ROUTING:
            s = "ROUTING";
            break;
#endif    /* defined(IPPROTO_ROUTING) */

#if    defined(IPPROTO_FRAGMENT)
        case IPPROTO_FRAGMENT:
            s = "FRAGMNT";
            break;
#endif    /* defined(IPPROTO_FRAGMENT) */

#if    defined(IPPROTO_IDRP)
        case IPPROTO_IDRP:
            s = "IDRP";
            break;
#endif    /* defined(IPPROTO_IDRP) */

#if    defined(IPPROTO_RSVP)
        case IPPROTO_RSVP:
            s = "RSVP";
            break;
#endif    /* defined(IPPROTO_RSVP) */

#if    defined(IPPROTO_GRE)
        case IPPROTO_GRE:
            s = "GRE";
            break;
#endif    /* defined(IPPROTO_GRE) */

#if    defined(IPPROTO_MHRP)
        case IPPROTO_MHRP:
            s = "MHRP";
            break;
#endif    /* defined(IPPROTO_MHRP) */

#if    defined(IPPROTO_BHA)
        case IPPROTO_BHA:
            s = "BHA";
            break;
#endif    /* defined(IPPROTO_BHA) */

#if    defined(IPPROTO_ESP)
        case IPPROTO_ESP:
            s = "ESP";
            break;
#endif    /* defined(IPPROTO_ESP) */

#if    defined(IPPROTO_AH)
        case IPPROTO_AH:
            s = "AH";
            break;
#endif    /* defined(IPPROTO_AH) */

#if    defined(IPPROTO_INLSP)
        case IPPROTO_INLSP:
            s = "INLSP";
            break;
#endif    /* defined(IPPROTO_INLSP) */

#if    defined(IPPROTO_SWIPE)
        case IPPROTO_SWIPE:
            s = "SWIPE";
            break;
#endif    /* defined(IPPROTO_SWIPE) */

#if    defined(IPPROTO_NHRP)
        case IPPROTO_NHRP:
            s = "NHRP";
            break;
#endif    /* defined(IPPROTO_NHRP) */

#if    defined(IPPROTO_NONE)
        case IPPROTO_NONE:
            s = "NONE";
            break;
#endif    /* defined(IPPROTO_NONE) */

#if    defined(IPPROTO_DSTOPTS)
        case IPPROTO_DSTOPTS:
            s = "DSTOPTS";
            break;
#endif    /* defined(IPPROTO_DSTOPTS) */

#if    defined(IPPROTO_AHIP)
        case IPPROTO_AHIP:
            s = "AHIP";
            break;
#endif    /* defined(IPPROTO_AHIP) */

#if    defined(IPPROTO_CFTP)
        case IPPROTO_CFTP:
            s = "CFTP";
            break;
#endif    /* defined(IPPROTO_CFTP) */

#if    defined(IPPROTO_SATEXPAK)
        case IPPROTO_SATEXPAK:
            s = "SATEXPK";
            break;
#endif    /* defined(IPPROTO_SATEXPAK) */

#if    defined(IPPROTO_KRYPTOLAN)
        case IPPROTO_KRYPTOLAN:
            s = "KRYPTOL";
            break;
#endif    /* defined(IPPROTO_KRYPTOLAN) */

#if    defined(IPPROTO_RVD)
        case IPPROTO_RVD:
            s = "RVD";
            break;
#endif    /* defined(IPPROTO_RVD) */

#if    defined(IPPROTO_IPPC)
        case IPPROTO_IPPC:
            s = "IPPC";
            break;
#endif    /* defined(IPPROTO_IPPC) */

#if    defined(IPPROTO_ADFS)
        case IPPROTO_ADFS:
            s = "ADFS";
            break;
#endif    /* defined(IPPROTO_ADFS) */

#if    defined(IPPROTO_SATMON)
        case IPPROTO_SATMON:
            s = "SATMON";
            break;
#endif    /* defined(IPPROTO_SATMON) */

#if    defined(IPPROTO_VISA)
        case IPPROTO_VISA:
            s = "VISA";
            break;
#endif    /* defined(IPPROTO_VISA) */

#if    defined(IPPROTO_IPCV)
        case IPPROTO_IPCV:
            s = "IPCV";
            break;
#endif    /* defined(IPPROTO_IPCV) */

#if    defined(IPPROTO_CPNX)
        case IPPROTO_CPNX:
            s = "CPNX";
            break;
#endif    /* defined(IPPROTO_CPNX) */

#if    defined(IPPROTO_CPHB)
        case IPPROTO_CPHB:
            s = "CPHB";
            break;
#endif    /* defined(IPPROTO_CPHB) */

#if    defined(IPPROTO_WSN)
        case IPPROTO_WSN:
            s = "WSN";
            break;
#endif    /* defined(IPPROTO_WSN) */

#if    defined(IPPROTO_PVP)
        case IPPROTO_PVP:
            s = "PVP";
            break;
#endif    /* defined(IPPROTO_PVP) */

#if    defined(IPPROTO_BRSATMON)
        case IPPROTO_BRSATMON:
            s = "BRSATMN";
            break;
#endif    /* defined(IPPROTO_BRSATMON) */

#if    defined(IPPROTO_WBMON)
        case IPPROTO_WBMON:
            s = "WBMON";
            break;
#endif    /* defined(IPPROTO_WBMON) */

#if    defined(IPPROTO_WBEXPAK)
        case IPPROTO_WBEXPAK:
            s = "WBEXPAK";
            break;
#endif    /* defined(IPPROTO_WBEXPAK) */

#if    defined(IPPROTO_EON)
        case IPPROTO_EON:
            s = "EON";
            break;
#endif    /* defined(IPPROTO_EON) */

#if    defined(IPPROTO_VMTP)
        case IPPROTO_VMTP:
            s = "VMTP";
            break;
#endif    /* defined(IPPROTO_VMTP) */

#if    defined(IPPROTO_SVMTP)
        case IPPROTO_SVMTP:
            s = "SVMTP";
            break;
#endif    /* defined(IPPROTO_SVMTP) */

#if    defined(IPPROTO_VINES)
        case IPPROTO_VINES:
            s = "VINES";
            break;
#endif    /* defined(IPPROTO_VINES) */

#if    defined(IPPROTO_TTP)
        case IPPROTO_TTP:
            s = "TTP";
            break;
#endif    /* defined(IPPROTO_TTP) */

#if    defined(IPPROTO_IGP)
        case IPPROTO_IGP:
            s = "IGP";
            break;
#endif    /* defined(IPPROTO_IGP) */

#if    defined(IPPROTO_DGP)
        case IPPROTO_DGP:
            s = "DGP";
            break;
#endif    /* defined(IPPROTO_DGP) */

#if    defined(IPPROTO_TCF)
        case IPPROTO_TCF:
            s = "TCF";
            break;
#endif    /* defined(IPPROTO_TCF) */

#if    defined(IPPROTO_IGRP)
        case IPPROTO_IGRP:
            s = "IGRP";
            break;
#endif    /* defined(IPPROTO_IGRP) */

#if    defined(IPPROTO_OSPFIGP)
        case IPPROTO_OSPFIGP:
            s = "OSPFIGP";
            break;
#endif    /* defined(IPPROTO_OSPFIGP) */

#if    defined(IPPROTO_SRPC)
        case IPPROTO_SRPC:
            s = "SRPC";
            break;
#endif    /* defined(IPPROTO_SRPC) */

#if    defined(IPPROTO_LARP)
        case IPPROTO_LARP:
            s = "LARP";
            break;
#endif    /* defined(IPPROTO_LARP) */

#if    defined(IPPROTO_MTP)
        case IPPROTO_MTP:
            s = "MTP";
            break;
#endif    /* defined(IPPROTO_MTP) */

#if    defined(IPPROTO_AX25)
        case IPPROTO_AX25:
            s = "AX25";
            break;
#endif    /* defined(IPPROTO_AX25) */

#if    defined(IPPROTO_IPEIP)
        case IPPROTO_IPEIP:
            s = "IPEIP";
            break;
#endif    /* defined(IPPROTO_IPEIP) */

#if    defined(IPPROTO_MICP)
        case IPPROTO_MICP:
            s = "MICP";
            break;
#endif    /* defined(IPPROTO_MICP) */

#if    defined(IPPROTO_SCCSP)
        case IPPROTO_SCCSP:
            s = "SCCSP";
            break;
#endif    /* defined(IPPROTO_SCCSP) */

#if    defined(IPPROTO_ETHERIP)
        case IPPROTO_ETHERIP:
            s = "ETHERIP";
            break;
#endif    /* defined(IPPROTO_ETHERIP) */

#if    defined(IPPROTO_ENCAP)
# if	!defined(IPPROTO_IPIP) || IPPROTO_IPIP!=IPPROTO_ENCAP
        case IPPROTO_ENCAP:
            s = "ENCAP";
            break;
# endif	/* !defined(IPPROTO_IPIP) || IPPROTO_IPIP!=IPPROTO_ENCAP */
#endif    /* defined(IPPROTO_ENCAP) */

#if    defined(IPPROTO_APES)
        case IPPROTO_APES:
            s = "APES";
            break;
#endif    /* defined(IPPROTO_APES) */

#if    defined(IPPROTO_GMTP)
        case IPPROTO_GMTP:
            s = "GMTP";
            break;
#endif    /* defined(IPPROTO_GMTP) */

#if    defined(IPPROTO_DIVERT)
        case IPPROTO_DIVERT:
            s = "DIVERT";
            break;
#endif    /* defined(IPPROTO_DIVERT) */

        default:
            s = (char *) NULL;
    }
    if (s)
        (void) snpf(CurrentLocalFile->iproto, sizeof(CurrentLocalFile->iproto), "%.*s", IPROTOL - 1, s);
    else {
        if (m < 0) {
            for (i = 0, m = 1; i < IPROTOL - 2; i++)
                m *= 10;
        }
        if (m > proto)
            (void) snpf(CurrentLocalFile->iproto, sizeof(CurrentLocalFile->iproto), "%d?", proto);
        else
            (void) snpf(CurrentLocalFile->iproto, sizeof(CurrentLocalFile->iproto), "*%d?", proto % (m / 10));
    }
}
#endif    /* !defined(HASPRIVPRIPP) */


/*
 * printname() - print output name field
 */

void
printname(newline)
        int newline;                /* NL status */
{

#if    defined(HASNCACHE)
    char buf[MAXPATHLEN];
    char *char_ptr;
    int full_path;
#endif    /* defined(HASNCACHE) */

    int print_status = 0;

    if (CurrentLocalFile->nm && CurrentLocalFile->nm[0]) {

        /*
         * Print the name characters, if there are some.
         */
        safestrprt(CurrentLocalFile->nm, stdout, 0);
        print_status++;
        if (!CurrentLocalFile->li[0].af && !CurrentLocalFile->li[1].af)
            goto print_nma;
    }
    if (CurrentLocalFile->li[0].af || CurrentLocalFile->li[1].af) {
        if (print_status)
            putchar(' ');
        /*
         * If the file has Internet addresses, print them.
         */
        if (printinaddr())
            print_status++;
        goto print_nma;
    }
    if (((CurrentLocalFile->ntype == N_BLK) || (CurrentLocalFile->ntype == N_CHR))
        && CurrentLocalFile->dev_def && CurrentLocalFile->rdev_def
        && printdevname(&CurrentLocalFile->dev, &CurrentLocalFile->rdev, 0, CurrentLocalFile->ntype)) {

        /*
         * If this is a block or character device and it has a name, print it.
         */
        print_status++;
        goto print_nma;
    }
    if (CurrentLocalFile->is_com) {

        /*
         * If this is a common node, print that fact.
         */
        (void) fputs("COMMON: ", stdout);
        print_status++;
        goto print_nma;
    }

#if    defined(HASPRIVNMCACHE)
    if (HASPRIVNMCACHE(CurrentLocalFile)) {
        print_status++;
        goto print_nma;
    }
#endif    /* defined(HASPRIVNMCACHE) */

    if (CurrentLocalFile->lmi_srch) {
        struct mounts *mp;
        /*
         * Do a deferred local mount info table search for the file system
         * (mounted) directory name and inode number, and mounted device name.
         */
        for (mp = readmnt(); mp; mp = mp->next) {
            if (CurrentLocalFile->dev == mp->dev) {
                CurrentLocalFile->fsdir = mp->dir;
                CurrentLocalFile->fsdev = mp->fsname;

#if    defined(HASFSINO)
                CurrentLocalFile->fs_ino = mp->inode;
#endif    /* defined(HASFSINO) */

                break;
            }
        }
        CurrentLocalFile->lmi_srch = 0;
    }
    if (CurrentLocalFile->fsdir || CurrentLocalFile->fsdev) {

        /*
         * Print the file system directory name, device name, and
         * possible path name components.
         */

#if    !defined(HASNCACHE) || HASNCACHE < 2
        if (CurrentLocalFile->fsdir) {
            safestrprt(CurrentLocalFile->fsdir, stdout, 0);
            print_status++;
        }
#endif    /* !defined(HASNCACHE) || HASNCACHE<2 */

#if    defined(HASNCACHE)

# if	HASNCACHE<2
        if (CurrentLocalFile->na) {
        if (NameCacheReload) {

#  if	defined(NCACHELDPFX)
            NCACHELDPFX
#  endif	/* defined(NCACHELDPFX) */

            (void) ncache_load();

#  if	defined(NCACHELDSFX)
            NCACHELDSFX
#  endif	/* defined(NCACHELDSFX) */

            NameCacheReload = 0;
        }
        if ((char_ptr = ncache_lookup(buf, sizeof(buf), &full_path))) {
            char *cp1;

            if (*char_ptr == '\0')
            goto print_nma;
            if (full_path && CurrentLocalFile->fsdir) {
            if (*char_ptr != '/') {
                cp1 = strrchr(CurrentLocalFile->fsdir, '/');
                if (cp1 == (char *)NULL ||  *(cp1 + 1) != '\0')
                putchar('/');
                }
            } else
            (void) fputs(" -- ", stdout);
            safestrprt(char_ptr, stdout, 0);
            print_status++;
            goto print_nma;
        }
        }
# else	/* HASNCACHE>1 */
        if (NameCacheReload) {

#  if	defined(NCACHELDPFX)
            NCACHELDPFX
#  endif	/* defined(NCACHELDPFX) */

        (void) ncache_load();

#  if	defined(NCACHELDSFX)
            NCACHELDSFX
#  endif	/* defined(NCACHELDSFX) */

        NameCacheReload = 0;
        }
        if ((char_ptr = ncache_lookup(buf, sizeof(buf), &full_path))) {
        if (full_path) {
            safestrprt(char_ptr, stdout, 0);
            print_status++;
        } else {
            if (CurrentLocalFile->fsdir) {
            safestrprt(CurrentLocalFile->fsdir, stdout, 0);
            print_status++;
            }
            if (*char_ptr) {
            (void) fputs(" -- ", stdout);
            safestrprt(char_ptr, stdout, 0);
            print_status++;
            }
        }
        goto print_nma;
        }
        if (CurrentLocalFile->fsdir) {
        safestrprt(CurrentLocalFile->fsdir, stdout, 0);
        print_status++;
        }
# endif	/* HASNCACHE<2 */
#endif    /* defined(HASNCACHE) */

        if (CurrentLocalFile->fsdev) {
            if (CurrentLocalFile->fsdir)
                (void) fputs(" (", stdout);
            else
                (void) putchar('(');
            safestrprt(CurrentLocalFile->fsdev, stdout, 0);
            (void) putchar(')');
            print_status++;
        }
    }
/*
 * Print the NAME column addition, if there is one.  If there isn't
 * make sure a NL is printed, as requested.
 */

    print_nma:

    if (CurrentLocalFile->nma) {
        if (print_status)
            putchar(' ');
        safestrprt(CurrentLocalFile->nma, stdout, 0);
        print_status++;
    }
/*
 * If this file has TCP/IP state information, print it.
 */
    if (!OptFieldOutput && OptTcpTpiInfo
        && (CurrentLocalFile->lts.type >= 0

#if    defined(HASTCPTPIQ)
                ||   ((OptTcpTpiInfo & TCPTPI_QUEUES) && (CurrentLocalFile->lts.rqs || CurrentLocalFile->lts.sqs))
#endif    /* defined(HASTCPTPIQ) */

#if    defined(HASTCPTPIW)
                ||   ((OptTcpTpiInfo & TCPTPI_WINDOWS) && (CurrentLocalFile->lts.rws || CurrentLocalFile->lts.wws))
#endif    /* defined(HASTCPTPIW) */

        )) {
        if (print_status)
            putchar(' ');
        (void) print_tcptpi(0);
    }
    if (newline)
        putchar('\n');
}


/*
 * printrawaddr() - print raw socket address
 */

void
printrawaddr(sock_addr)
        struct sockaddr *sock_addr;        /* socket address */
{
    char *ep;
    size_t sz;

    ep = endnm(&sz);
    (void) snpf(ep, sz, "%u/%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                sock_addr->sa_family,
                (unsigned char) sock_addr->sa_data[0],
                (unsigned char) sock_addr->sa_data[1],
                (unsigned char) sock_addr->sa_data[2],
                (unsigned char) sock_addr->sa_data[3],
                (unsigned char) sock_addr->sa_data[4],
                (unsigned char) sock_addr->sa_data[5],
                (unsigned char) sock_addr->sa_data[6],
                (unsigned char) sock_addr->sa_data[7],
                (unsigned char) sock_addr->sa_data[8],
                (unsigned char) sock_addr->sa_data[9],
                (unsigned char) sock_addr->sa_data[10],
                (unsigned char) sock_addr->sa_data[11],
                (unsigned char) sock_addr->sa_data[12],
                (unsigned char) sock_addr->sa_data[13]);
}


/*
 * printsockty() - print socket type
 */

char *
printsockty(type)
        int type;            /* socket type -- e.g., from so_type */
{
    static char buf[64];
    char *char_ptr;

    switch (type) {

#if    defined(SOCK_STREAM)
        case SOCK_STREAM:
            char_ptr = "STREAM";
            break;
#endif    /* defined(SOCK_STREAM) */

#if    defined(SOCK_STREAM)
        case SOCK_DGRAM:
            char_ptr = "DGRAM";
            break;
#endif    /* defined(SOCK_DGRAM) */

#if    defined(SOCK_RAW)
        case SOCK_RAW:
            char_ptr = "RAW";
            break;
#endif    /* defined(SOCK_RAW) */

#if    defined(SOCK_RDM)
        case SOCK_RDM:
            char_ptr = "RDM";
            break;
#endif    /* defined(SOCK_RDM) */

#if    defined(SOCK_SEQPACKET)
        case SOCK_SEQPACKET:
            char_ptr = "SEQPACKET";
            break;
#endif    /* defined(SOCK_SEQPACKET) */

        default:
            (void) snpf(buf, sizeof(buf), "SOCK_%#x", type);
            return (buf);
    }
    (void) snpf(buf, sizeof(buf), "SOCK_%s", char_ptr);
    return (buf);
}


/*
 * printuid() - print User ID or login name
 */

char *
printuid(uid, type)
        UID_ARG uid;            /* User IDentification number */
        int *type;            /* returned UID type pointer (NULL
					 * (if none wanted).  If non-NULL
					 * then: *type = 0 = login name
					 *	       = 1 = UID number */
{
    int i;
    struct passwd *pw;
    struct stat sb;
    static struct stat sbs;
    static struct uidcache {
        uid_t uid;
        char nm[LOGINML + 1];
        struct uidcache *next;
    } **uc = (struct uidcache **) NULL;
    struct uidcache *up, *upn;
    static char user[USERPRTL + 1];

    if (OptUserToLogin) {
        if (CheckPasswdChange) {

            /*
             * Get the mtime and ctime of /etc/passwd, as required.
             */
            if (stat("/etc/passwd", &sb) != 0) {
                (void) fprintf(stderr, "%s: can't stat(/etc/passwd): %s\n",
                               ProgramName, strerror(errno));
                Exit(1);
            }
        }
        /*
         * Define the UID cache, if necessary.
         */
        if (!uc) {
            if (!(uc = (struct uidcache **) calloc(UIDCACHEL,
                                                   sizeof(struct uidcache *)))) {
                (void) fprintf(stderr,
                               "%s: no space for %d byte UID cache hash buckets\n",
                               ProgramName, (int) (UIDCACHEL * (sizeof(struct uidcache *))));
                Exit(1);
            }
            if (CheckPasswdChange) {
                sbs = sb;
                CheckPasswdChange = 0;
            }
        }
        /*
         * If it's time to check /etc/passwd and if its the mtime/ctime has
         * changed, destroy the existing UID cache.
         */
        if (CheckPasswdChange) {
            if (sbs.st_mtime != sb.st_mtime || sbs.st_ctime != sb.st_ctime) {
                for (i = 0; i < UIDCACHEL; i++) {
                    if ((up = uc[i])) {
                        do {
                            upn = up->next;
                            (void) free((FREE_P *) up);
                        } while ((up = upn) != (struct uidcache *) NULL);
                        uc[i] = (struct uidcache *) NULL;
                    }
                }
                sbs = sb;
            }
            CheckPasswdChange = 0;
        }
        /*
         * Search the UID cache.
         */
        i = (int) ((((unsigned long) uid * 31415L) >> 7) & (UIDCACHEL - 1));
        for (up = uc[i]; up; up = up->next) {
            if (up->uid == (uid_t) uid) {
                if (type)
                    *type = 0;
                return (up->nm);
            }
        }
        /*
         * The UID is not in the cache.
         *
         * Look up the login name from the UID for a new cache entry.
         */
        if (!(pw = getpwuid((uid_t) uid))) {
            if (!OptWarnings) {
                (void) fprintf(stderr, "%s: no pwd entry for UID %lu\n",
                               ProgramName, (unsigned long) uid);
            }
        } else {

            /*
             * Allocate and fill a new cache entry.  Link it to its hash bucket.
             */
            if (!(upn = (struct uidcache *) malloc(sizeof(struct uidcache)))) {
                (void) fprintf(stderr,
                               "%s: no space for UID cache entry for: %lu, %s)\n",
                               ProgramName, (unsigned long) uid, pw->pw_name);
                Exit(1);
            }
            (void) strncpy(upn->nm, pw->pw_name, LOGINML);
            upn->nm[LOGINML] = '\0';
            upn->uid = (uid_t) uid;
            upn->next = uc[i];
            uc[i] = upn;
            if (type)
                *type = 0;
            return (upn->nm);
        }
    }
/*
 * Produce a numeric conversion of the UID.
 */
    (void) snpf(user, sizeof(user), "%*lu", USERPRTL, (unsigned long) uid);
    if (type)
        *type = 1;
    return (user);
}


/*
 * printunkaf() - print unknown address family
 */

void
printunkaf(fam, type)
        int fam;            /* unknown address family */
        int type;            /* output type: 0 = terse; 1 = full */
{
    char *p, *s;

    p = "";
    switch (fam) {

#if    defined(AF_UNSPEC)
        case AF_UNSPEC:
            s = "UNSPEC";
            break;
#endif    /* defined(AF_UNSPEC) */

#if    defined(AF_UNIX)
        case AF_UNIX:
            s = "UNIX";
            break;
#endif    /* defined(AF_UNIX) */

#if    defined(AF_INET)
        case AF_INET:
            s = "INET";
            break;
#endif    /* defined(AF_INET) */

#if    defined(AF_INET6)
        case AF_INET6:
            s = "INET6";
            break;
#endif    /* defined(AF_INET6) */

#if    defined(AF_IMPLINK)
        case AF_IMPLINK:
            s = "IMPLINK";
            break;
#endif    /* defined(AF_IMPLINK) */

#if    defined(AF_PUP)
        case AF_PUP:
            s = "PUP";
            break;
#endif    /* defined(AF_PUP) */

#if    defined(AF_CHAOS)
        case AF_CHAOS:
            s = "CHAOS";
            break;
#endif    /* defined(AF_CHAOS) */

#if    defined(AF_NS)
        case AF_NS:
            s = "NS";
            break;
#endif    /* defined(AF_NS) */

#if    defined(AF_ISO)
        case AF_ISO:
            s = "ISO";
            break;
#endif    /* defined(AF_ISO) */

#if    defined(AF_NBS)
# if	!defined(AF_ISO) || AF_NBS!=AF_ISO
        case AF_NBS:
            s = "NBS";
            break;
# endif	/* !defined(AF_ISO) || AF_NBS!=AF_ISO */
#endif    /* defined(AF_NBS) */

#if    defined(AF_ECMA)
        case AF_ECMA:
            s = "ECMA";
            break;
#endif    /* defined(AF_ECMA) */

#if    defined(AF_DATAKIT)
        case AF_DATAKIT:
            s = "DATAKIT";
            break;
#endif    /* defined(AF_DATAKIT) */

#if    defined(AF_CCITT)
        case AF_CCITT:
            s = "CCITT";
            break;
#endif    /* defined(AF_CCITT) */

#if    defined(AF_SNA)
        case AF_SNA:
            s = "SNA";
            break;
#endif    /* defined(AF_SNA) */

#if    defined(AF_DECnet)
        case AF_DECnet:
            s = "DECnet";
            break;
#endif    /* defined(AF_DECnet) */

#if    defined(AF_DLI)
        case AF_DLI:
            s = "DLI";
            break;
#endif    /* defined(AF_DLI) */

#if    defined(AF_LAT)
        case AF_LAT:
            s = "LAT";
            break;
#endif    /* defined(AF_LAT) */

#if    defined(AF_HYLINK)
        case AF_HYLINK:
            s = "HYLINK";
            break;
#endif    /* defined(AF_HYLINK) */

#if    defined(AF_APPLETALK)
        case AF_APPLETALK:
            s = "APPLETALK";
            break;
#endif    /* defined(AF_APPLETALK) */

#if    defined(AF_BSC)
        case AF_BSC:
            s = "BSC";
            break;
#endif    /* defined(AF_BSC) */

#if    defined(AF_DSS)
        case AF_DSS:
            s = "DSS";
            break;
#endif    /* defined(AF_DSS) */

#if    defined(AF_ROUTE)
        case AF_ROUTE:
            s = "ROUTE";
            break;
#endif    /* defined(AF_ROUTE) */

#if    defined(AF_RAW)
        case AF_RAW:
            s = "RAW";
            break;
#endif    /* defined(AF_RAW) */

#if    defined(AF_LINK)
        case AF_LINK:
            s = "LINK";
            break;
#endif    /* defined(AF_LINK) */

#if    defined(pseudo_AF_XTP)
        case pseudo_AF_XTP:
            p = "pseudo_";
            s = "XTP";
            break;
#endif    /* defined(pseudo_AF_XTP) */

#if    defined(AF_RMP)
        case AF_RMP:
            s = "RMP";
            break;
#endif    /* defined(AF_RMP) */

#if    defined(AF_COIP)
        case AF_COIP:
            s = "COIP";
            break;
#endif    /* defined(AF_COIP) */

#if    defined(AF_CNT)
        case AF_CNT:
            s = "CNT";
            break;
#endif    /* defined(AF_CNT) */

#if    defined(pseudo_AF_RTIP)
        case pseudo_AF_RTIP:
            p = "pseudo_";
            s = "RTIP";
            break;
#endif    /* defined(pseudo_AF_RTIP) */

#if    defined(AF_NETMAN)
        case AF_NETMAN:
            s = "NETMAN";
            break;
#endif    /* defined(AF_NETMAN) */

#if    defined(AF_INTF)
        case AF_INTF:
            s = "INTF";
            break;
#endif    /* defined(AF_INTF) */

#if    defined(AF_NETWARE)
        case AF_NETWARE:
            s = "NETWARE";
            break;
#endif    /* defined(AF_NETWARE) */

#if    defined(AF_NDD)
        case AF_NDD:
            s = "NDD";
            break;
#endif    /* defined(AF_NDD) */

#if    defined(AF_NIT)
# if	!defined(AF_ROUTE) || AF_ROUTE!=AF_NIT
        case AF_NIT:
            s = "NIT";
            break;
# endif	/* !defined(AF_ROUTE) || AF_ROUTE!=AF_NIT */
#endif    /* defined(AF_NIT) */

#if    defined(AF_802)
# if	!defined(AF_RAW) || AF_RAW!=AF_802
        case AF_802:
            s = "802";
            break;
# endif	/* !defined(AF_RAW) || AF_RAW!=AF_802 */
#endif    /* defined(AF_802) */

#if    defined(AF_X25)
        case AF_X25:
            s = "X25";
            break;
#endif    /* defined(AF_X25) */

#if    defined(AF_CTF)
        case AF_CTF:
            s = "CTF";
            break;
#endif    /* defined(AF_CTF) */

#if    defined(AF_WAN)
        case AF_WAN:
            s = "WAN";
            break;
#endif    /* defined(AF_WAN) */

#if    defined(AF_OSINET)
# if	defined(AF_INET) && AF_INET!=AF_OSINET
        case AF_OSINET:
            s = "OSINET";
            break;
# endif	/* defined(AF_INET) && AF_INET!=AF_OSINET */
#endif    /* defined(AF_OSINET) */

#if    defined(AF_GOSIP)
        case AF_GOSIP:
            s = "GOSIP";
            break;
#endif    /* defined(AF_GOSIP) */

#if    defined(AF_SDL)
        case AF_SDL:
            s = "SDL";
            break;
#endif    /* defined(AF_SDL) */

#if    defined(AF_IPX)
        case AF_IPX:
            s = "IPX";
            break;
#endif    /* defined(AF_IPX) */

#if    defined(AF_SIP)
        case AF_SIP:
            s = "SIP";
            break;
#endif    /* defined(AF_SIP) */

#if    defined(psuedo_AF_PIP)
        case psuedo_AF_PIP:
            p = "pseudo_";
            s = "PIP";
            break;
#endif    /* defined(psuedo_AF_PIP) */

#if    defined(AF_OTS)
        case AF_OTS:
            s = "OTS";
            break;
#endif    /* defined(AF_OTS) */

#if    defined(pseudo_AF_BLUE)
        case pseudo_AF_BLUE:	/* packets for Blue box */
            p = "pseudo_";
            s = "BLUE";
            break;
#endif    /* defined(pseudo_AF_BLUE) */

#if    defined(AF_NDRV)    /* network driver raw access */
        case AF_NDRV:
            s = "NDRV";
            break;
#endif    /* defined(AF_NDRV) */

#if    defined(AF_SYSTEM)    /* kernel event messages */
        case AF_SYSTEM:
            s = "SYSTEM";
            break;
#endif    /* defined(AF_SYSTEM) */

#if    defined(AF_USER)
        case AF_USER:
            s = "USER";
            break;
#endif    /* defined(AF_USER) */

#if    defined(pseudo_AF_KEY)
        case pseudo_AF_KEY:
            p = "pseudo_";
            s = "KEY";
            break;
#endif    /* defined(pseudo_AF_KEY) */

#if    defined(AF_KEY)        /* Security Association DB socket */
        case AF_KEY:
            s = "KEY";
            break;
#endif    /* defined(AF_KEY) */

#if    defined(AF_NCA)        /* NCA socket */
        case AF_NCA:
            s = "NCA";
            break;
#endif    /* defined(AF_NCA) */

#if    defined(AF_POLICY)        /* Security Policy DB socket */
        case AF_POLICY:
            s = "POLICY";
            break;
#endif    /* defined(AF_POLICY) */

#if    defined(AF_PPP)        /* PPP socket */
        case AF_PPP:
            s = "PPP";
            break;
#endif    /* defined(AF_PPP) */

        default:
            if (!type)
                (void) snpf(NameChars, NameCharsLength, "%#x", fam);
            else
                (void) snpf(NameChars, NameCharsLength,
                            "no further information on family %#x", fam);
            return;
    }
    if (!type)
        (void) snpf(NameChars, NameCharsLength, "%sAF_%s", p, s);
    else
        (void) snpf(NameChars, NameCharsLength, "no further information on %sAF_%s",
                    p, s);
    return;
}


#if    !defined(HASNORPC_H)
/*
 * update_portmap() - update a portmap entry with its port number or service
 *		      name
 */

static void
update_portmap(port_entry, prog_name)
        struct porttab *port_entry;        /* porttab entry */
        char *prog_name;            /* port name */
{
    MALLOC_S pn_len, name_len;
    char *char_ptr;

    if (port_entry->ss)
        return;
    if (!(pn_len = strlen(prog_name))) {
        port_entry->ss = 1;
        return;
    }
    name_len = pn_len + port_entry->nl + 2;
    if (!(char_ptr = (char *) malloc(name_len + 1))) {
        (void) fprintf(stderr,
                       "%s: can't allocate %d bytes for portmap name: %s[%s]\n",
                       ProgramName, (int) (name_len + 1), prog_name, port_entry->name);
        Exit(1);
    }
    (void) snpf(char_ptr, name_len + 1, "%s[%s]", prog_name, port_entry->name);
    (void) free((FREE_P *) port_entry->name);
    port_entry->name = char_ptr;
    port_entry->nl = name_len;
    port_entry->ss = 1;
}
#endif    /* !defined(HASNORPC_H) */

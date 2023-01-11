/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

/*  $OpenBSD: mbuf.h,v 1.255 2022/08/15 16:15:37 bluhm Exp $  */
/*  $NetBSD: mbuf.h,v 1.19 1996/02/09 18:25:14 christos Exp $  */

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 *  @(#)mbuf.h  8.5 (Berkeley) 2/19/95
 */

#ifndef _SYS_MBUF_H_
#define _SYS_MBUF_H_

#include "ng_defs.h"
#include "ng_queue.h"
#include "ng_msg.h"

typedef struct mbuf     ng_mbuf_s;
typedef struct m_hdr    ng_mhdr_s;
typedef struct pkthdr   ng_pkthdr_s;
typedef struct mbuf_ext ng_mext_s;
typedef struct m_tag    ng_mtag_s;

#define  MSIZE_MIN       1024               /* minimal size of an mbuf */

#define  MAXMCLBYTES    (1024 << 3)         /* largest cluster from the stack */
#define  MCLSHIFT        11                 /* convert bytes to m_buf clusters */

/* 2K cluster can hold Ether frame */
#define  MCLBYTES       (1 << MCLSHIFT)     /* size of a m_buf cluster */
#define  MCLOFSET       (MCLBYTES - 1)

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than PAGE_SIZE (the software page size) and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define  MSIZE    MSIZE_MIN    /* size of an mbuf */

/*
 * Mbufs are of a single size, MSIZE, which includes overhead.  An mbuf may
 * add a single "mbuf cluster" of size MCLBYTES, which has no additional
 * overhead and is used instead of the internal data area; this is done when
 * at least MINCLSIZE of data must be stored.
 */

#define  MLEN    (MSIZE - sizeof(ng_mhdr_s))    /* normal data len   */
#define  MHLEN   (MLEN  - sizeof(ng_pkthdr_s))  /* data len w/pkthdr */

#define  MINCLSIZE      (MHLEN + MLEN + 1)   /* smallest amount to put in cluster */
#define  M_MAXCOMPRESS  (MHLEN >> 1)         /* max amount to copy for compression */

/* Packet tags structure */
struct m_tag {
  ng_list_s   m_tag_link;  /* List of packet tags */
  uint16_t    m_tag_id;  /* Tag ID */
  uint16_t    m_tag_len;  /* Length of data */
};

/*
 * Macros for type conversion
 * mtod(m,t) -  convert mbuf pointer to data pointer of correct type
 */
#define mtod(m,t)  ((t)((m)->m_data))

/* header at beginning of each mbuf: */
struct m_hdr {
  ng_msg_s   mh_msg;       /* next chain in queue/record */
  uint32_t   mh_len;       /* amount of data in this mbuf */
  caddr_t    mh_data;      /* location of data */
  ng_mbuf_s *mh_next;      /* next buffer in chain */
  uint16_t   mh_type;      /* type of data in this mbuf */
  uint16_t   mh_flags;     /* flags; see below */
  uint32_t   mh_pad;
};

#define IFQ_NQUEUES  8
#define IFQ_MINPRIO  0
#define IFQ_MAXPRIO  IFQ_NQUEUES - 1
#define IFQ_DEFPRIO  3
#define IFQ_PRIO2TOS(_p) ((_p) << 5)
#define IFQ_TOS2PRIO(_t) ((_t) >> 5)
struct pkthdr_pf {
  void     *statekey;   /* pf stackside statekey */
  void     *inp;        /* connected pcb for outgoing packet */
  uint32_t  qid;        /* queue id */
  uint16_t  tag;        /* tag id */
  uint16_t  delay;      /* delay packet by X ms */
  uint8_t   flags;
  uint8_t   routed;
  uint8_t   prio;
  uint8_t   pad[1];
};
typedef struct pkthdr_pf ng_pkthdr_pf_s;

/* pkthdr_pf.flags */
#define  PF_TAG_GENERATED    0x01
#define  PF_TAG_SYNCOOKIE_RECREATED  0x02
#define  PF_TAG_TRANSLATE_LOCALHOST  0x04
#define  PF_TAG_DIVERTED      0x08
#define  PF_TAG_DIVERTED_PACKET    0x10
#define  PF_TAG_REROUTE      0x20
#define  PF_TAG_REFRAGMENTED    0x40  /* refragmented ipv6 packet */
#define  PF_TAG_PROCESSED    0x80  /* packet was checked by pf */

#define MPF_BITS \
    ("\20\1GENERATED\2SYNCOOKIE_RECREATED\3TRANSLATE_LOCALHOST\4DIVERTED" \
    "\5DIVERTED_PACKET\6REROUTE\7REFRAGMENTED\10PROCESSED")

/* record/packet header in first mbuf of chain; valid if M_PKTHDR set */
struct  pkthdr {
  void      *ph_cookie;    /* additional data */
  ng_list_s  ph_tags;      /* list of packet tags */
  uint64_t   ph_timestamp; /* packet timestamp */
  uint32_t   len;          /* total packet length */
  uint16_t   ph_tagsset;   /* mtags attached */
  uint16_t   ph_flowid;    /* pseudo unique flow id */
  uint16_t   csum_flags;   /* checksum flags */
  uint16_t   ether_vtag;   /* Ethernet 802.1p+Q vlan tag */
  uint16_t   ph_rtableid;  /* routing table id */
  uint16_t   ph_ifidx;     /* rcv interface index */
  uint8_t    ph_loopcnt;   /* mbuf is looping in kernel */
  uint8_t    ph_family;    /* af, used when queueing */
  ng_pkthdr_pf_s   pf;
};

/* description of external storage mapped into mbuf, valid if M_EXT set */
struct mbuf_ext {
  caddr_t    ext_buf;      /* start of buffer */
  void      *ext_arg;
  u_int      ext_free_fn;  /* index of free function */
  u_int      ext_size;     /* size of buffer, for ext_free_fn */
};

struct mbuf {
  ng_mhdr_s m_hdr;
  union {
    struct {
      ng_pkthdr_s MH_pkthdr;   /* M_PKTHDR set */
      union {
        ng_mext_s MH_ext;      /* M_EXT set */
        char MH_databuf[MHLEN];
      } MH_dat;
    } MH;
    char M_databuf[MLEN];      /* !M_PKTHDR, !M_EXT */
  } M_dat;
};
#define  m_next     m_hdr.mh_next
#define  m_len      m_hdr.mh_len
#define  m_data     m_hdr.mh_data
#define  m_type     m_hdr.mh_type
#define  m_flags    m_hdr.mh_flags
#define  m_link     m_hdr.mh_msg.link
#define  m_nextpkt  m_hdr.mh_msg.link.next
#define  m_pad      m_hdr.mh_pad
#define  m_pktlen   M_dat.MH.MH_pkthdr.len
#define  m_pkthdr   M_dat.MH.MH_pkthdr
#define  m_ext      M_dat.MH.MH_dat.MH_ext
#define  m_pktdat   M_dat.MH.MH_dat.MH_databuf
#define  m_dat      M_dat.M_databuf

/* mbuf flags */
#define M_EXT     0x0001  /* has associated external storage */
#define M_PKTHDR  0x0002  /* start of record */
#define M_EOR     0x0004  /* end of record */
#define M_EXTWR   0x0008  /* external storage is writable */
#define M_PROTO1  0x0010  /* protocol-specific */

/* mbuf pkthdr flags, also in m_flags */
#define M_VLANTAG 0x0020  /* ether_vtag is valid */
#define M_LOOP    0x0040  /* packet has been sent from local machine */
#define M_BCAST   0x0100  /* sent/received as link-level broadcast */
#define M_MCAST   0x0200  /* sent/received as link-level multicast */
#define M_CONF    0x0400  /* payload was encrypted (ESP-transport) */
#define M_AUTH    0x0800  /* payload was authenticated (AH or ESP auth) */
#define M_TUNNEL  0x1000  /* IP-in-IP added by tunnel mode IPsec */
#define M_ZEROIZE 0x2000  /* Zeroize data part on free */
#define M_COMP    0x4000  /* header was decompressed */
#define M_LINK0   0x8000  /* link layer specific flag */

#define M_BITS \
    ("\20\1M_EXT\2M_PKTHDR\3M_EOR\4M_EXTWR\5M_PROTO1\6M_VLANTAG\7M_LOOP" \
    "\11M_BCAST\12M_MCAST\13M_CONF\14M_AUTH\15M_TUNNEL" \
    "\16M_ZEROIZE\17M_COMP\20M_LINK0")

/* flags copied when copying m_pkthdr */
#define  M_COPYFLAGS  (M_PKTHDR|M_EOR|M_PROTO1|M_BCAST|M_MCAST|M_CONF|M_COMP|\
       M_AUTH|M_LOOP|M_TUNNEL|M_LINK0|M_VLANTAG|M_ZEROIZE)

/* mbuf types */
#define  MT_FREE    0  /* should be on free list */
#define  MT_DATA    1  /* dynamic (data) allocation */
#define  MT_HEADER  2  /* packet header */
#define  MT_SONAME  3  /* socket name */
#define  MT_SOOPTS  4  /* socket options */
#define  MT_FTABLE  5  /* fragment reassembly header */
#define  MT_CONTROL 6  /* extra-data protocol message */
#define  MT_OOBDATA 7  /* expedited data  */
#define  MT_NTYPES  8

/* Checksumming flags */
#define  M_IPV4_CSUM_OUT    0x0001  /* IPv4 checksum needed */
#define  M_TCP_CSUM_OUT    0x0002  /* TCP checksum needed */
#define  M_UDP_CSUM_OUT    0x0004  /* UDP checksum needed */
#define  M_IPV4_CSUM_IN_OK  0x0008  /* IPv4 checksum verified */
#define  M_IPV4_CSUM_IN_BAD  0x0010  /* IPv4 checksum bad */
#define  M_TCP_CSUM_IN_OK  0x0020  /* TCP checksum verified */
#define  M_TCP_CSUM_IN_BAD  0x0040  /* TCP checksum bad */
#define  M_UDP_CSUM_IN_OK  0x0080  /* UDP checksum verified */
#define  M_UDP_CSUM_IN_BAD  0x0100  /* UDP checksum bad */
#define  M_ICMP_CSUM_OUT    0x0200  /* ICMP/ICMPv6 checksum needed */
#define  M_ICMP_CSUM_IN_OK  0x0400  /* ICMP/ICMPv6 checksum verified */
#define  M_ICMP_CSUM_IN_BAD  0x0800  /* ICMP/ICMPv6 checksum bad */
#define  M_IPV6_DF_OUT    0x1000  /* don't fragment outgoing IPv6 */
#define  M_TIMESTAMP    0x2000  /* ph_timestamp is set */
#define  M_FLOWID    0x4000  /* ph_flowid is set */

#define MCS_BITS \
    ("\20\1IPV4_CSUM_OUT\2TCP_CSUM_OUT\3UDP_CSUM_OUT\4IPV4_CSUM_IN_OK" \
    "\5IPV4_CSUM_IN_BAD\6TCP_CSUM_IN_OK\7TCP_CSUM_IN_BAD\10UDP_CSUM_IN_OK" \
    "\11UDP_CSUM_IN_BAD\12ICMP_CSUM_OUT\13ICMP_CSUM_IN_OK\14ICMP_CSUM_IN_BAD" \
    "\15IPV6_NODF_OUT" "\16TIMESTAMP" "\17FLOWID")

/*
 * mbuf allocation/deallocation macros:
 *
 *  MGET(ng_mbuf_s *m, int how, int type)
 * allocates an mbuf and initializes it to contain internal data.
 *
 *  MGETHDR(ng_mbuf_s *m, int how, int type)
 * allocates an mbuf and initializes it to contain a packet header
 * and internal data.
 */
#define MGET(m, how, type)      m = m_get((how), (type))
#define MGETHDR(m, how, type)   m = m_gethdr((how), (type))

/*
 * Macros for mbuf external storage.
 *
 * MEXTADD adds pre-allocated external storage to
 * a normal mbuf; the flag M_EXT is set.
 *
 * MCLGET allocates and adds an mbuf cluster to a normal mbuf;
 * the flag M_EXT is set upon success.
 */
#define  MEXTADD(m, buf, mflags) do {      \
  (m)->m_data = (m)->m_ext.ext_buf = (caddr_t)(buf);    \
  (m)->m_flags |= M_EXT | (mflags & M_EXTWR);      \
} while (/* CONSTCOND */ 0)

#define MCLGET(m, how) (void) m_clget((m), (how), MCLBYTES)
#define MCLGETL(m, how, l) m_clget((m), (how), (l))

/*
 * Move just m_pkthdr from from to to,
 * remove M_PKTHDR and clean flags/tags for from.
 */
#define M_MOVE_HDR(to, from) do {          \
  (to)->m_pkthdr = (from)->m_pkthdr;        \
  (from)->m_flags &= ~M_PKTHDR;          \
  __list_poison_entry(&(from)->m_pkthdr.ph_tags);        \
} while (/* CONSTCOND */ 0)

/*
 * MOVE mbuf pkthdr from from to to.
 * from must have M_PKTHDR set, and to must be empty.
 */
#define  M_MOVE_PKTHDR(to, from) do {          \
  (to)->m_flags = ((to)->m_flags & (M_EXT | M_EXTWR));    \
  (to)->m_flags |= (from)->m_flags & M_COPYFLAGS;      \
  M_MOVE_HDR((to), (from));          \
  if (((to)->m_flags & M_EXT) == 0)        \
    (to)->m_data = (to)->m_pktdat;        \
} while (/* CONSTCOND */ 0)

/*
 * Determine if an mbuf's data area is read-only. This is true for
 * non-cluster external storage and for clusters that are being
 * referenced by more than one mbuf.
 */
#define  M_READONLY(m)              \
  (((m)->m_flags & M_EXT) != 0 &&          \
    (((m)->m_flags & M_EXTWR) == 0))

/*
 * Arrange to prepend space of size plen to mbuf m.
 * If a new mbuf must be allocated, how specifies whether to wait.
 * If how is M_DONTWAIT and allocation fails, the original mbuf chain
 * is freed and m is set to NULL.
 */
#define  M_PREPEND(m, plen, how) \
    (m) = m_prepend((m), (plen), (how))

/* length to m_copy to copy all */
#define  M_COPYALL  1000000000

/*
 * Mbuf statistics.
 * For statistics related to mbuf and cluster allocations, see also the
 * pool headers (mbpool and mclpool).
 */
struct mbstat {
  u_long  m_drops;  /* times failed to find space */
  u_long  m_wait;    /* times waited for space */
  u_long  m_drain;  /* times drained protocols for space */
  u_short  m_mtypes[256];  /* type specific mbuf allocations */
};

#define MBSTAT_TYPES           MT_NTYPES
#define MBSTAT_DROPS           (MBSTAT_TYPES + 0)
#define MBSTAT_WAIT            (MBSTAT_TYPES + 1)
#define MBSTAT_DRAIN           (MBSTAT_TYPES + 2)
#define MBSTAT_COUNT           (MBSTAT_TYPES + 3)

/* Packet tag types */
#define PACKET_TAG_IPSEC_IN_DONE  0x0001  /* IPsec applied, in */
#define PACKET_TAG_IPSEC_OUT_DONE  0x0002  /* IPsec applied, out */
#define PACKET_TAG_IPSEC_FLOWINFO  0x0004  /* IPsec flowinfo */
#define PACKET_TAG_WIREGUARD    0x0040  /* WireGuard data */
#define PACKET_TAG_GRE      0x0080  /* GRE processing done */
#define PACKET_TAG_DLT      0x0100 /* data link layer type */
#define PACKET_TAG_PF_DIVERT    0x0200 /* pf(4) diverted packet */
#define PACKET_TAG_PF_REASSEMBLED  0x0800 /* pf reassembled ipv6 packet */
#define PACKET_TAG_SRCROUTE    0x1000 /* IPv4 source routing options */
#define PACKET_TAG_TUNNEL    0x2000  /* Tunnel endpoint address */
#define PACKET_TAG_CARP_BAL_IP    0x4000  /* carp(4) ip balanced marker */
#define PACKET_TAG_IP6_OFFNXT    0x8000  /* IPv6 offset and next proto */

#define MTAG_BITS \
    ("\20\1IPSEC_IN_DONE\2IPSEC_OUT_DONE\3IPSEC_FLOWINFO" \
    "\4IPSEC_OUT_CRYPTO_NEEDED\5IPSEC_PENDING_TDB\6BRIDGE\7WG\10GRE\11DLT" \
    "\12PF_DIVERT\14PF_REASSEMBLED\15SRCROUTE\16TUNNEL\17CARP_BAL_IP")

/*
 * Maximum tag payload length (that is excluding the m_tag structure).
 * Please make sure to update this value when increasing the payload
 * length for an existing packet tag type or when adding a new one that
 * has payload larger than the value below.
 */
#define PACKET_TAG_MAXSIZE    80

/* Detect mbufs looping in the kernel when spliced too often. */
#define M_MAXLOOP  128

#define NG_MBUF_FOR_EACH(m) \
  for (; (m) != NULL; (m) = (m)->m_next)
  
ng_mbuf_s *m_copym(ng_mbuf_s *, int, int, int);
ng_mbuf_s *m_free(ng_mbuf_s *);
void m_free_one(ng_mbuf_s *m);
ng_mbuf_s *m_get(int, int);
ng_mbuf_s *m_getclr(int, int);
ng_mbuf_s *m_gethdr(int, int);
ng_mbuf_s *m_inithdr(ng_mbuf_s *);
void  m_removehdr(ng_mbuf_s *);
void  m_resethdr(ng_mbuf_s *);
void  m_calc_hdrlen(ng_mbuf_s *);
ng_result_e  m_defrag(ng_mbuf_s *, int);
ng_mbuf_s *m_prepend(ng_mbuf_s *, int, int);
ng_mbuf_s *m_pulldown(ng_mbuf_s *, int, int, int *);
ng_mbuf_s *m_pullup(ng_mbuf_s *, int);
ng_mbuf_s *m_split(ng_mbuf_s *, int, int);
ng_mbuf_s *m_makespace(ng_mbuf_s *, int, int, int *);
ng_mbuf_s *m_getptr(ng_mbuf_s *, int, int *);
int  m_leadingspace(ng_mbuf_s *);
int  m_trailingspace(ng_mbuf_s *);
void  m_align(ng_mbuf_s *, int);
ng_mbuf_s *m_clget(ng_mbuf_s *, int, u_int);
void  m_extref(ng_mbuf_s *, ng_mbuf_s *);
void  m_adj(ng_mbuf_s *, int);
ng_result_e  m_copyback(ng_mbuf_s *, int, int, const void *, int);
void m_freem(ng_list_s *);
void m_purge(ng_list_s *);
void  m_reclaim(void *, int);
void  m_copydata(ng_mbuf_s *, int, int, void *);
void  m_cat(ng_mbuf_s *, ng_mbuf_s *);
ng_mbuf_s *m_devget(char *, int, int);
int  m_apply(ng_mbuf_s *, int, int,
      int (*)(caddr_t, caddr_t, unsigned int), caddr_t);
ng_mbuf_s *m_dup_pkt(ng_mbuf_s *, unsigned int, int);
int  m_dup_pkthdr(ng_mbuf_s *, ng_mbuf_s *, int);

/* Packet tag routines */
ng_mtag_s *m_tag_get(int, int, int);
void  m_tag_prepend(ng_mbuf_s *, ng_mtag_s *);
void  m_tag_delete(ng_mbuf_s *, ng_mtag_s *);
void  m_tag_delete_chain(ng_mbuf_s *);
ng_mtag_s *m_tag_find(ng_mbuf_s *, int, ng_mtag_s *);
ng_mtag_s *m_tag_copy(ng_mtag_s *, int);
int  m_tag_copy_chain(ng_mbuf_s *, ng_mbuf_s *, int);
void  m_tag_init(ng_mbuf_s *);
ng_mtag_s *m_tag_first(ng_mbuf_s *);
ng_mtag_s *m_tag_next(ng_mtag_s *);

#endif /* _SYS_MBUF_H_ */


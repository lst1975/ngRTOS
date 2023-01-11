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

/*  $OpenBSD: uipc_mbuf.c,v 1.284 2022/08/14 01:58:28 jsg Exp $  */
/*  $NetBSD: uipc_mbuf.c,v 1.15.4.1 1996/06/13 17:11:44 cgd Exp $  */

/*
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
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
 *  @(#)uipc_mbuf.c  8.2 (Berkeley) 1/4/94
 */

/*
 *  @(#)COPYRIGHT  1.1 (NRL) 17 January 1995
 *
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 *  This product includes software developed at the Information
 *  Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

#include "ng_arch.h"
#include "ng_defs.h"
#include "mem/ng_mem.h"
#include "ng_mbuf.h"
#include "ng_mcopy.h"

u_int max_linkhdr  = 16;       /* largest link-level header */
u_int max_protohdr = 40;       /* largest protocol header */
u_int max_hdr      = 16 + 40;  /* largest link+protocol header */

void  m_extfree(ng_mbuf_s *);
void  m_zero(ng_mbuf_s *);

#define M_DATABUF(m)  ((m)->m_flags & M_EXT ? (m)->m_ext.ext_buf : \
      (m)->m_flags & M_PKTHDR ? (m)->m_pktdat : (m)->m_dat)
#define M_SIZE(m)  ((m)->m_flags & M_EXT ? (m)->m_ext.ext_size : \
      (m)->m_flags & M_PKTHDR ? MHLEN : MLEN)

/*
 * Space allocation routines.
 */
ng_mbuf_s *
m_get(int nowait, int type)
{
  ng_mbuf_s *m;

  NGRTOS_ASSERT(type >= 0 && type < MT_NTYPES);

  m = __ng_malloc(nowait, sizeof(*m));
  if (m == NULL)
    return (NULL);

  m->m_type    = type;
  m->m_next    = NULL;
  m->m_nextpkt = NULL;
  m->m_data    = m->m_dat;
  m->m_flags   = 0;
  m->m_len     = 0;
  __list_poison_entry(&m->m_link);
  return (m);
}

/*
 * ATTN: When changing anything here check m_inithdr() and m_defrag() those
 * may need to change as well.
 */
ng_mbuf_s *
m_gethdr(int nowait, int type)
{
  ng_mbuf_s *m;

  NGRTOS_ASSERT(type >= 0 && type < MT_NTYPES);

  m = (ng_mbuf_s *)__ng_malloc(nowait, sizeof(*m));
  if (m == NULL)
  {
    NGRTOS_ERROR("Failed to malloc ng_mbuf_s.\n");
    return NULL;
  }
  m->m_type = type;
  return m_inithdr(m);
}

ng_mbuf_s *
m_inithdr(ng_mbuf_s *m)
{
  /* keep in sync with m_gethdr */
  m->m_next    = NULL;
  m->m_nextpkt = NULL;
  m->m_data    = m->m_pktdat;
  m->m_flags   = M_PKTHDR;
  ng_memset(&m->m_pkthdr, 0, sizeof(m->m_pkthdr));
  m->m_pkthdr.pf.prio = IFQ_DEFPRIO;
  return (m);
}

static inline void
m_clearhdr(ng_mbuf_s *m)
{
  /* delete all mbuf tags to reset the state */
  m_tag_delete_chain(m);
#if NPF > 0
  pf_mbuf_unlink_state_key(m);
  pf_mbuf_unlink_inpcb(m);
#endif  /* NPF > 0 */
  ng_memset(&m->m_pkthdr, 0, sizeof(m->m_pkthdr));
}

void
m_removehdr(ng_mbuf_s *m)
{
  NGRTOS_ASSERT(m->m_flags & M_PKTHDR);
  m_clearhdr(m);
  m->m_flags &= ~M_PKTHDR;
}

void
m_resethdr(ng_mbuf_s *m)
{
  int len = m->m_pktlen;
  uint8_t loopcnt = m->m_pkthdr.ph_loopcnt;

  NGRTOS_ASSERT(m->m_flags & M_PKTHDR);
  m->m_flags &= (M_EXT|M_PKTHDR|M_EOR|M_EXTWR|M_ZEROIZE);
  m_clearhdr(m);
  /* like m_inithdr(), but keep any associated data and mbufs */
  m->m_pkthdr.pf.prio = IFQ_DEFPRIO;
  m->m_pktlen = len;
  m->m_pkthdr.ph_loopcnt = loopcnt;
}

void
m_calc_hdrlen(ng_mbuf_s *m)
{
  ng_mbuf_s *n;
  int plen = 0;

  NGRTOS_ASSERT(m->m_flags & M_PKTHDR);
  for (n = m; n; n = n->m_next)
    plen += n->m_len;
  m->m_pktlen = plen;
}

ng_mbuf_s *
m_getclr(int nowait, int type)
{
  ng_mbuf_s *m;

  MGET(m, nowait, type);
  if (m == NULL)
    return (NULL);
  ng_memset(mtod(m, caddr_t), 0, MLEN);
  return (m);
}

ng_mbuf_s *
m_clget(ng_mbuf_s *m, int how, u_int pktlen)
{
  ng_mbuf_s *m0 = NULL;
  caddr_t buf;

  if (m == NULL) 
  {
    m0 = m_gethdr(how, MT_DATA);
    if (m0 == NULL)
    {
      NGRTOS_ERROR("Failed to m_gethdr.\n");
      return (NULL);
    }
    m = m0;
  }
  buf = __ng_malloc(how, pktlen);
  if (buf == NULL) 
  {
    NGRTOS_ERROR("Failed to malloc extbuf: %u.\n", pktlen);
    m_free_one(m0);
    return (NULL);
  }

  MEXTADD(m, buf, M_EXTWR);
  return (m);
}

ng_mbuf_s *
m_free(ng_mbuf_s *m)
{
  ng_mbuf_s *n;

  if (m == NULL)
    return NULL;

  n = m->m_next;
  if (m->m_flags & M_ZEROIZE) 
  {
    m_zero(m);
    /* propagate M_ZEROIZE to the next mbuf in the chain */
    if (n != NULL)
      n->m_flags |= M_ZEROIZE;
  }
  if (m->m_flags & M_PKTHDR) 
  {
    m_tag_delete_chain(m);
  }
  if (m->m_flags & M_EXT)
    m_extfree(m);

  ng_free(m);

  return (n);
}

void
m_extfree(ng_mbuf_s *m)
{
  if (m->m_ext.ext_buf == NULL)
    return;
  ng_free(m->m_ext.ext_buf);
  m->m_flags &= ~(M_EXT|M_EXTWR);
  m->m_ext.ext_buf = NULL;
}

void
m_free_one(ng_mbuf_s *m)
{
  if (m == NULL)
    return;

  do {
    m = m_free(m);
  }
  while (m != NULL);
}

void
m_freem(ng_list_s *list)
{
  ng_mbuf_s *n;
  if (list_empty(list))
    return;
  
  n = list_first_entry(list,ng_mbuf_s,m_link);
  list_del(&n->m_link);
  m_free_one(n);
}

void
m_purge(ng_list_s *list)
{
  while (!list_empty(list))
    m_freem(list);
}

/*
 * mbuf chain defragmenter. This function uses some evil tricks to defragment
 * an mbuf chain into a single buffer without changing the mbuf pointer.
 * This needs to know a lot of the mbuf internals to make this work.
 */
ng_result_e
m_defrag(ng_mbuf_s *m, int how)
{
  ng_mbuf_s *m0;

  if (m->m_next == NULL)
    return NG_result_ok;

  NGRTOS_ASSERT(m->m_flags & M_PKTHDR);

  m0 = m_gethdr(how, m->m_type);
  if (m0 == NULL)
    return NG_result_nmem;
  
  if (m->m_pktlen > MHLEN) 
  {
    MCLGETL(m0, how, m->m_pktlen);
    if (!(m0->m_flags & M_EXT)) 
    {
      m_free(m0);
      return NG_result_nmem;
    }
  }
  m_copydata(m, 0, m->m_pktlen, mtod(m0, caddr_t));
  m0->m_pktlen = m0->m_len = m->m_pktlen;

  /* free chain behind and possible ext buf on the first mbuf */
  m_free_one(m->m_next);
  m->m_next = NULL;
  if (m->m_flags & M_EXT)
    m_extfree(m);

  /*
   * Bounce copy mbuf over to the original mbuf and set everything up.
   * This needs to reset or clear all pointers that may go into the
   * original mbuf chain.
   */
  if (m0->m_flags & M_EXT) 
  {
    memcpy(&m->m_ext, &m0->m_ext, sizeof(ng_mext_s));
    m->m_flags |= m0->m_flags & (M_EXT|M_EXTWR);
    m->m_data = m->m_ext.ext_buf;
  } 
  else 
  {
    m->m_data = m->m_pktdat;
    memcpy(m->m_data, m0->m_data, m0->m_len);
  }
  m->m_pktlen = m->m_len = m0->m_len;

  m0->m_flags &= ~(M_EXT|M_EXTWR);  /* cluster is gone */
  m_free(m0);

  return NG_result_ok;
}

/*
 * Mbuffer utility routines.
 */

/*
 * Ensure len bytes of contiguous space at the beginning of the mbuf chain
 */
ng_mbuf_s *
m_prepend(ng_mbuf_s *m, int len, int how)
{
  ng_mbuf_s *mn;

  if (len > MHLEN)
  { 
    NGRTOS_ERROR("mbuf prepend length %d too big.\n", len);
    return NULL;
  }
  
  if (m_leadingspace(m) >= len) 
  {
    m->m_data -= len;
    m->m_len += len;
  } 
  else 
  {
    MGET(mn, how, m->m_type);
    if (mn == NULL) 
    {
      NGRTOS_ERROR("Failed to malloc MBUF.\n");
      m_free_one(m);
      return (NULL);
    }
    if (m->m_flags & M_PKTHDR)
      M_MOVE_PKTHDR(mn, m);
    mn->m_next = m;
    m = mn;
    m_align(m, len);
    m->m_len = len;
  }
  if (m->m_flags & M_PKTHDR)
    m->m_pktlen += len;
  return (m);
}

/*
 * Make a copy of an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
 * The wait parameter is a choice of M_WAIT/M_DONTWAIT from caller.
 */
ng_mbuf_s *
m_copym(ng_mbuf_s *m0, int off, int len, int wait)
{
  ng_mbuf_s *m, *n, **np;
  ng_mbuf_s *top;
  int copyhdr = 0;

  NGRTOS_ASSERT(off >= 0 && len >= 0);

  if (off == 0 && m0->m_flags & M_PKTHDR)
    copyhdr = 1;

  m = m_getptr(m0, off, &off);
  NGRTOS_ASSERT(m != NULL);

  np = &top;
  top = NULL;
  while (len > 0) 
  {
    if (m == NULL) 
    {
      NGRTOS_ASSERT(len == M_COPYALL);
      break;
    }
    MGET(n, wait, m->m_type);
    *np = n;
    if (n == NULL)
    {
      NGRTOS_ERROR("Failed to malloc MBUF.\n");
      goto nospace;
    }
    if (copyhdr) 
    {
      if (m_dup_pkthdr(n, m0, wait))
        goto nospace;
      if (len != M_COPYALL)
        n->m_pktlen = len;
      copyhdr = 0;
    }
    n->m_len = ng_min_t(int, len, m->m_len - off);
    if (m->m_flags & M_EXT) 
    {
      n->m_data = m->m_data + off;
      n->m_ext = m->m_ext;
    } 
    else 
    {
      n->m_data += m->m_data -
          (m->m_flags & M_PKTHDR ? m->m_pktdat : m->m_dat);
      n->m_data += off;
      memcpy(mtod(n, caddr_t), mtod(m, caddr_t) + off,
          n->m_len);
    }
    if (len != M_COPYALL)
      len -= n->m_len;
    off += n->m_len;
    if (off > m->m_len)
    {  
      NGRTOS_ERROR("m_copym0 overrun.\n");
      goto nospace;
    }
    if (off == m->m_len) 
    {
      m = m->m_next;
      off = 0;
    }
    np = &n->m_next;
  }
  return (top);
nospace:
  m_free_one(top);
  return (NULL);
}

/*
 * Copy data from an mbuf chain starting "off" bytes from the beginning,
 * continuing for "len" bytes, into the indicated buffer.
 */
void
m_copydata(ng_mbuf_s *m, int off, int len, void *p)
{
  caddr_t cp = p;
  unsigned count;

  NGRTOS_ASSERT(off >= 0 && len >= 0);
  m = m_getptr(m, off, &off);
  NGRTOS_ASSERT(m != NULL);

  while (len > 0) 
  {
    NGRTOS_ASSERT(m != NULL);
    count = ng_min_t(int, m->m_len - off, len);
    memmove(cp, mtod(m, caddr_t) + off, count);
    len -= count;
    cp += count;
    off = 0;
    m = m->m_next;
  }
}

/*
 * Copy data from a buffer back into the indicated mbuf chain,
 * starting "off" bytes from the beginning, extending the mbuf
 * chain if necessary. The mbuf needs to be properly initialized
 * including the setting of m_len.
 */
ng_result_e
m_copyback(ng_mbuf_s *m0, int off, int len, const void *_cp, int wait)
{
  int mlen, totlen = 0;
  ng_mbuf_s *m = m0, *n;
  caddr_t cp = (caddr_t)_cp;
  ng_result_e error = NG_result_ok;

  if (m0 == NULL)
    return NG_result_ok;
  
  while (off > (mlen = m->m_len)) 
  {
    off -= mlen;
    totlen += mlen;
    if (m->m_next == NULL) 
    {
      if ((n = m_get(wait, m->m_type)) == NULL) 
      {
        error = NG_result_nmem;
        goto out;
      }

      if (off + len > MLEN) 
      {
        MCLGETL(n, wait, off + len);
        if (!(n->m_flags & M_EXT)) 
        {
          m_free(n);
          error = NG_result_nmem;
          goto out;
        }
      }
      ng_memset(mtod(n, caddr_t), 0, off);
      n->m_len = len + off;
      m->m_next = n;
    }
    m = m->m_next;
  }
  
  while (len > 0) 
  {
    /* extend last packet to be filled fully */
    if (m->m_next == NULL && (len > m->m_len - off))
      m->m_len += ng_min_t(int, len - (m->m_len - off),
          m_trailingspace(m));
    mlen = ng_min_t(int, m->m_len - off, len);
    memmove(mtod(m, caddr_t) + off, cp, mlen);
    cp += mlen;
    len -= mlen;
    totlen += mlen + off;
    if (len == 0)
      break;
    off = 0;

    if (m->m_next == NULL) 
    {
      if ((n = m_get(wait, m->m_type)) == NULL) 
      {
        error = NG_result_nmem;
        goto out;
      }

      if (len > MLEN) 
      {
        MCLGETL(n, wait, len);
        if (!(n->m_flags & M_EXT)) 
        {
          m_free(n);
          error = NG_result_nmem;
          goto out;
        }
      }
      n->m_len = len;
      m->m_next = n;
    }
    m = m->m_next;
  }
out:
  if (((m = m0)->m_flags & M_PKTHDR) && (m->m_pktlen < totlen))
    m->m_pktlen = totlen;

  return error;
}

/*
 * Concatenate mbuf chain n to m.
 * n might be copied into m (when n->m_len is small), therefore data portion of
 * n could be copied into an mbuf of different mbuf type.
 * Therefore both chains should be of the same type (e.g. MT_DATA).
 * Any m_pkthdr is not updated.
 */
void
m_cat(ng_mbuf_s *m, ng_mbuf_s *n)
{
  while (m->m_next)
    m = m->m_next;
  while (n) 
  {
    if (M_READONLY(m) || n->m_len > m_trailingspace(m)) 
    {
      /* just join the two chains */
      m->m_next = n;
      return;
    }
    /* splat the data from one into the other */
    memcpy(mtod(m, caddr_t) + m->m_len, mtod(n, caddr_t),
        n->m_len);
    m->m_len += n->m_len;
    n = m_free(n);
  }
}

/*
 * Trim from head.
 */
void
m_adj(ng_mbuf_s *mp, int req_len)
{
  int len = req_len;
  ng_mbuf_s *m;
  int count;

  if (mp == NULL)
    return;
  if (len >= 0) 
  {
    /*
     * Trim from head.
     */
    m = mp;
    while (m != NULL && len > 0) 
    {
      if (m->m_len <= len) 
      {
        len -= m->m_len;
        m->m_data += m->m_len;
        m->m_len = 0;
        m = m->m_next;
      }
      else 
      {
        m->m_data += len;
        m->m_len -= len;
        len = 0;
      }
    }
    if (mp->m_flags & M_PKTHDR)
      mp->m_pktlen -= (req_len - len);
  } 
  else 
  {
    /*
     * Trim from tail.  Scan the mbuf chain,
     * calculating its length and finding the last mbuf.
     * If the adjustment only affects this mbuf, then just
     * adjust and return.  Otherwise, rescan and truncate
     * after the remaining size.
     */
    len = -len;
    count = 0;
    m = mp;
    for (;;) 
    {
      count += m->m_len;
      if (m->m_next == NULL)
        break;
      m = m->m_next;
    }
    if (m->m_len >= len) 
    {
      m->m_len -= len;
      if (mp->m_flags & M_PKTHDR)
        mp->m_pktlen -= len;
      return;
    }
    count -= len;
    if (count < 0)
      count = 0;
    /*
     * Correct length for chain is "count".
     * Find the mbuf with last data, adjust its length,
     * and toss data from remaining mbufs on chain.
     */
    if (mp->m_flags & M_PKTHDR)
      mp->m_pktlen = count;
    m = mp;
    for (;;) 
    {
      if (m->m_len >= count) 
      {
        m->m_len = count;
        break;
      }
      count -= m->m_len;
      m = m->m_next;
    }
    while ((m = m->m_next) != NULL)
      m->m_len = 0;
  }
}

/*
 * Rearrange an mbuf chain so that len bytes are contiguous
 * and in the data area of an mbuf (so that mtod will work
 * for a structure of size len).  Returns the resulting
 * mbuf chain on success, frees it and returns null on failure.
 */
ng_mbuf_s *
m_pullup(ng_mbuf_s *m0, int len)
{
  ng_mbuf_s *m;
  unsigned int adj;
  caddr_t head, tail;
  unsigned int space;

  /* if len is already contig in m0, then don't do any work */
  if (len <= m0->m_len)
    return (m0);

  /* look for some data */
  m = m0->m_next;
  if (m == NULL)
    goto freem0;

  head = M_DATABUF(m0);
  if (m0->m_len == 0) 
  {
    while (m->m_len == 0) 
    {
      m = m_free(m);
      if (m == NULL)
        goto freem0;
    }

    adj = mtod(m, unsigned long) & (sizeof(long) - 1);
  }
  else
    adj = mtod(m0, unsigned long) & (sizeof(long) - 1);

  tail = head + M_SIZE(m0);
  head += adj;

  if (!M_READONLY(m0) && len <= tail - head) 
  {
    /* we can copy everything into the first mbuf */
    if (m0->m_len == 0) 
    {
      m0->m_data = head;
    } 
    else if (len > tail - mtod(m0, caddr_t)) 
    {
      /* need to memmove to make space at the end */
      memmove(head, mtod(m0, caddr_t), m0->m_len);
      m0->m_data = head;
    }

    len -= m0->m_len;
  } 
  else 
  {
    /* the first mbuf is too small or read-only, make a new one */
    space = adj + len;

    if (space > MAXMCLBYTES)
      goto bad;

    m0->m_next = m;
    m = m0;

    MGET(m0, M_DONTWAIT, m->m_type);
    if (m0 == NULL)
      goto bad;

    if (space > MHLEN) 
    {
      MCLGETL(m0, M_DONTWAIT, space);
      if ((m0->m_flags & M_EXT) == 0)
        goto bad;
    }

    if (m->m_flags & M_PKTHDR)
      M_MOVE_PKTHDR(m0, m);

    m0->m_len = 0;
    m0->m_data += adj;
  }

  NGRTOS_ASSERT(m_trailingspace(m0) >= len);

  for (;;) 
  {
    space = ng_min_t(int, len, m->m_len);
    memcpy(mtod(m0, caddr_t) + m0->m_len, mtod(m, caddr_t), space);
    len -= space;
    m0->m_len += space;
    m->m_len -= space;

    if (m->m_len > 0)
      m->m_data += space;
    else
      m = m_free(m);

    if (len == 0)
      break;

    if (m == NULL)
      goto bad;
  }

  m0->m_next = m; /* link the chain back up */

  return (m0);

bad:
  m_free_one(m);
freem0:
  m_free(m0);
  return (NULL);
}

/*
 * Return a pointer to mbuf/offset of location in mbuf chain.
 */
ng_mbuf_s *
m_getptr(ng_mbuf_s *m, int loc, int *off)
{
  while (loc >= 0) 
  {
    /* Normal end of search */
    if (m->m_len > loc) 
    {
      *off = loc;
      return (m);
    }
    else 
    {
      loc -= m->m_len;

      if (m->m_next == NULL) 
      {
        if (loc == 0) 
        {
          /* Point at the end of valid data */
          *off = m->m_len;
          return (m);
        } 
        else 
        {
          return (NULL);
        }
      } 
      else 
      {
        m = m->m_next;
      }
    }
  }

  return (NULL);
}

/*
 * Partition an mbuf chain in two pieces, returning the tail --
 * all but the first len0 bytes.  In case of failure, it returns NULL and
 * attempts to restore the chain to its original state.
 */
ng_mbuf_s *
m_split(ng_mbuf_s *m0, int len0, int wait)
{
  ng_mbuf_s *m, *n;
  unsigned len = len0, remain, olen;

  for (m = m0; m && len > m->m_len; m = m->m_next)
    len -= m->m_len;
  if (m == NULL)
    return (NULL);
  remain = m->m_len - len;
  if (m0->m_flags & M_PKTHDR) 
  {
    MGETHDR(n, wait, m0->m_type);
    if (n == NULL)
      return (NULL);
    if (m_dup_pkthdr(n, m0, wait)) 
    {
      m_free_one(n);
      return (NULL);
    }
    n->m_pktlen -= len0;
    olen = m0->m_pktlen;
    m0->m_pktlen = len0;
    if (remain == 0) {
      n->m_next = m->m_next;
      m->m_next = NULL;
      n->m_len = 0;
      return (n);
    }
    if (m->m_flags & M_EXT)
      goto extpacket;
    if (remain > MHLEN) 
    {
      /* m can't be the lead packet */
      m_align(n, 0);
      n->m_next = m_split(m, len, wait);
      if (n->m_next == NULL) 
      {
        (void) m_free(n);
        m0->m_pktlen = olen;
        return (NULL);
      } 
      else 
      {
        n->m_len = 0;
        return (n);
      }
    } else
      m_align(n, remain);
  }
  else if (remain == 0) 
  {
    n = m->m_next;
    m->m_next = NULL;
    return (n);
  }
  else 
  {
    MGET(n, wait, m->m_type);
    if (n == NULL)
      return (NULL);
    m_align(n, remain);
  }
extpacket:
  if (m->m_flags & M_EXT)
  {
    n->m_ext  = m->m_ext;
    n->m_data = m->m_data + len;
  } 
  else 
  {
    memcpy(mtod(n, caddr_t), mtod(m, caddr_t) + len, remain);
  }
  n->m_len = remain;
  m->m_len = len;
  n->m_next = m->m_next;
  m->m_next = NULL;
  return (n);
}

/*
 * Make space for a new header of length hlen at skip bytes
 * into the packet.  When doing this we allocate new mbufs only
 * when absolutely necessary.  The mbuf where the new header
 * is to go is returned together with an offset into the mbuf.
 * If NULL is returned then the mbuf chain may have been modified;
 * the caller is assumed to always free the chain.
 */
ng_mbuf_s *
m_makespace(ng_mbuf_s *m0, int skip, int hlen, int *off)
{
  ng_mbuf_s *m;
  unsigned remain;

  NGRTOS_ASSERT(m0->m_flags & M_PKTHDR);
  /*
   * Limit the size of the new header to MHLEN. In case
   * skip = 0 and the first buffer is not a cluster this
   * is the maximum space available in that mbuf.
   * In other words this code never prepends a mbuf.
   */
  NGRTOS_ASSERT(hlen < MHLEN);

  for (m = m0; m && skip > m->m_len; m = m->m_next)
    skip -= m->m_len;
  if (m == NULL)
    return (NULL);
  /*
   * At this point skip is the offset into the mbuf m
   * where the new header should be placed.  Figure out
   * if there's space to insert the new header.  If so,
   * and copying the remainder makes sense then do so.
   * Otherwise insert a new mbuf in the chain, splitting
   * the contents of m as needed.
   */
  remain = m->m_len - skip;    /* data to move */
  if (skip < remain && hlen <= m_leadingspace(m)) 
  {
    if (skip)
      memmove(m->m_data-hlen, m->m_data, skip);
    m->m_data -= hlen;
    m->m_len += hlen;
    *off = skip;
  } 
  else if (hlen > m_trailingspace(m)) 
  {
    ng_mbuf_s *n;

    if (remain > 0) 
    {
      MGET(n, M_DONTWAIT, m->m_type);
      if (n && remain > MLEN) 
      {
        MCLGETL(n, M_DONTWAIT, remain);
        if ((n->m_flags & M_EXT) == 0) 
        {
          m_free(n);
          n = NULL;
        }
      }
      if (n == NULL)
        return (NULL);

      memcpy(n->m_data, mtod(m, caddr_t) + skip, remain);
      n->m_len  = remain;
      m->m_len -= remain;

      n->m_next = m->m_next;
      m->m_next = n;
    }

    if (hlen <= m_trailingspace(m)) 
    {
      m->m_len += hlen;
      *off = skip;
    }
    else 
    {
      n = m_get(M_DONTWAIT, m->m_type);
      if (n == NULL)
        return NULL;

      n->m_len = hlen;

      n->m_next = m->m_next;
      m->m_next = n;

      *off = 0;  /* header is at front ... */
      m = n;    /* ... of new mbuf */
    }
  } 
  else 
  {
    /*
     * Copy the remainder to the back of the mbuf
     * so there's space to write the new header.
     */
    if (remain > 0)
      memmove(mtod(m, caddr_t) + skip + hlen,
            mtod(m, caddr_t) + skip, remain);
    m->m_len += hlen;
    *off = skip;
  }
  m0->m_pktlen += hlen;    /* adjust packet length */
  return m;
}


/*
 * Routine to copy from device local memory into mbufs.
 */
ng_mbuf_s *
m_devget(char *buf, int totlen, int off)
{
  ng_mbuf_s *m;
  ng_mbuf_s *top, **mp;
  int     len;

  top = NULL;
  mp = &top;

  if (off < 0 || off > MHLEN)
    return (NULL);

  MGETHDR(m, M_DONTWAIT, MT_DATA);
  if (m == NULL)
    return (NULL);

  m->m_pktlen = totlen;

  len = MHLEN;

  while (totlen > 0) 
  {
    if (top != NULL) 
    {
      MGET(m, M_DONTWAIT, MT_DATA);
      if (m == NULL) 
      {
        /*
         * As we might get called by pfkey, make sure
         * we do not leak sensitive data.
         */
        top->m_flags |= M_ZEROIZE;
        m_free_one(top);
        return (NULL);
      }
      len = MLEN;
    }

    if (totlen + off >= MINCLSIZE) 
    {
      MCLGET(m, M_DONTWAIT);
      if (m->m_flags & M_EXT)
        len = MCLBYTES;
    } 
    else 
    {
      /* Place initial small packet/header at end of mbuf. */
      if (top == NULL && totlen + off + max_linkhdr <= len) 
      {
        m->m_data += max_linkhdr;
        len -= max_linkhdr;
      }
    }

    if (off) 
    {
      m->m_data += off;
      len -= off;
      off = 0;
    }

    m->m_len = len = ng_min_t(int, totlen, len);
    memcpy(mtod(m, void *), buf, (size_t)len);

    buf += len;
    *mp = m;
    mp = &m->m_next;
    totlen -= len;
  }
  return (top);
}

void
m_zero(ng_mbuf_s *m)
{
  if (M_READONLY(m)) {
    return;
  }
  ng_mzero(M_DATABUF(m), M_SIZE(m));
}

/*
 * Apply function f to the data in an mbuf chain starting "off" bytes from the
 * beginning, continuing for "len" bytes.
 */
int
m_apply(ng_mbuf_s *m, int off, int len,
    int (*f)(caddr_t, caddr_t, unsigned int), caddr_t fstate)
{
  int rval;
  unsigned int count;

  NGRTOS_ASSERT(off >= 0 && len >= 0);

  while (off > 0) 
  {
    NGRTOS_ASSERT(m != NULL);
    if (off < m->m_len)
      break;
    off -= m->m_len;
    m = m->m_next;
  }
  while (len > 0) 
  {
    NGRTOS_ASSERT(m != NULL);
    count = ng_min_t(int, m->m_len - off, len);

    rval = f(fstate, mtod(m, caddr_t) + off, count);
    if (rval)
      return (rval);

    len -= count;
    off = 0;
    m = m->m_next;
  }

  return (0);
}

/*
 * Compute the amount of space available before the current start of data
 * in an mbuf. Read-only clusters never have space available.
 */
int
m_leadingspace(ng_mbuf_s *m)
{
  if (M_READONLY(m))
    return 0;
  NGRTOS_ASSERT(m->m_data >= M_DATABUF(m));
  return m->m_data - M_DATABUF(m);
}

/*
 * Compute the amount of space available after the end of data in an mbuf.
 * Read-only clusters never have space available.
 */
int
m_trailingspace(ng_mbuf_s *m)
{
  if (M_READONLY(m))
    return 0;
  NGRTOS_ASSERT(M_DATABUF(m) + M_SIZE(m) >= (m->m_data + m->m_len));
  return M_DATABUF(m) + M_SIZE(m) - (m->m_data + m->m_len);
}

/*
 * Set the m_data pointer of a newly-allocated mbuf to place an object of
 * the specified size at the end of the mbuf, longword aligned.
 */
void
m_align(ng_mbuf_s *m, int len)
{
  NGRTOS_ASSERT(len >= 0 && !M_READONLY(m));
  NGRTOS_ASSERT(m->m_data == M_DATABUF(m));  /* newly-allocated check */
  NGRTOS_ASSERT(((len + sizeof(long) - 1) &~ (sizeof(long) - 1)) <= M_SIZE(m));

  m->m_data = M_DATABUF(m) + ((M_SIZE(m) - (len)) &~ (sizeof(long) - 1));
}

/*
 * Duplicate mbuf pkthdr from from to to.
 * from must have M_PKTHDR set, and to must be empty.
 */
int
m_dup_pkthdr(ng_mbuf_s *to, ng_mbuf_s *from, int wait)
{
  int error;

  NGRTOS_ASSERT(from->m_flags & M_PKTHDR);

  to->m_flags = (to->m_flags & (M_EXT | M_EXTWR));
  to->m_flags |= (from->m_flags & M_COPYFLAGS);
  to->m_pkthdr = from->m_pkthdr;

#if NPF > 0
  to->m_pkthdr.pf.statekey = NULL;
  pf_mbuf_link_state_key(to, from->m_pkthdr.pf.statekey);
  to->m_pkthdr.pf.inp = NULL;
  pf_mbuf_link_inpcb(to, from->m_pkthdr.pf.inp);
#endif  /* NPF > 0 */
  INIT_LIST_HEAD(&to->m_pkthdr.ph_tags);

  if ((error = m_tag_copy_chain(to, from, wait)) != 0)
    return (error);

  if ((to->m_flags & M_EXT) == 0)
    to->m_data = to->m_pktdat;

  return (0);
}

ng_mbuf_s *
m_dup_pkt(ng_mbuf_s *m0, unsigned int adj, int wait)
{
  ng_mbuf_s *m;
  int len;

  NGRTOS_ASSERT(m0->m_flags & M_PKTHDR);

  len = m0->m_pktlen + adj;
  if (len > MAXMCLBYTES) /* XXX */
    return (NULL);

  m = m_get(wait, m0->m_type);
  if (m == NULL)
    return (NULL);

  if (m_dup_pkthdr(m, m0, wait) != 0)
    goto fail;

  if (len > MHLEN) {
    MCLGETL(m, wait, len);
    if (!(m->m_flags & M_EXT))
      goto fail;
  }

  m->m_len = m->m_pktlen = len;
  m_adj(m, adj);
  m_copydata(m0, 0, m0->m_pktlen, mtod(m, caddr_t));

  return (m);

fail:
  m_free_one(m);
  return (NULL);
}

/* can't call it m_dup(), as freebsd[34] uses m_dup() with different arg */
static ng_mbuf_s *
___m_dup(ng_mbuf_s *m, int off, int len, int wait)
{
  ng_mbuf_s *n;
  int l;

  if (len > MAXMCLBYTES)
    return (NULL);
  if (off == 0 && (m->m_flags & M_PKTHDR) != 0) 
  {
    MGETHDR(n, wait, m->m_type);
    if (n == NULL)
      return (NULL);
    if (m_dup_pkthdr(n, m, wait)) 
    {
      m_free(n);
      return (NULL);
    }
    l = MHLEN;
  } 
  else 
  {
    MGET(n, wait, m->m_type);
    l = MLEN;
  }
  if (n && len > l) 
  {
    MCLGETL(n, wait, len);
    if ((n->m_flags & M_EXT) == 0) 
    {
      m_free(n);
      n = NULL;
    }
  }
  if (!n)
    return (NULL);

  m_copydata(m, off, len, mtod(n, caddr_t));
  n->m_len = len;

  return (n);
}

/*
 * ensure that [off, off + len] is contiguous on the mbuf chain "m".
 * packet chain before "off" is kept untouched.
 * if offp == NULL, the target will start at <retval, 0> on resulting chain.
 * if offp != NULL, the target will start at <retval, *offp> on resulting chain.
 *
 * on error return (NULL return value), original "m" will be freed.
 *
 * XXX m_trailingspace/m_leadingspace on shared cluster (sharedcluster)
 */
ng_mbuf_s *
m_pulldown(ng_mbuf_s *m, int off, int len, int *offp)
{
  ng_mbuf_s *n, *o;
  int hlen, tlen, olen;
  int sharedcluster;

  /* check invalid arguments. */
  if (m == NULL)
  { 
    NGRTOS_ERROR("m == NULL in m_pulldown().\n");
    return NULL;
  }
  
  if ((n = m_getptr(m, off, &off)) == NULL) 
  {
    m_free_one(m);
    return (NULL);  /* mbuf chain too short */
  }

  sharedcluster = M_READONLY(n);

  /*
   * the target data is on <n, off>.
   * if we got enough data on the mbuf "n", we're done.
   */
  if ((off == 0 || offp) && len <= n->m_len - off && !sharedcluster)
    goto ok;

  /*
   * when len <= n->m_len - off and off != 0, it is a special case.
   * len bytes from <n, off> sits in single mbuf, but the caller does
   * not like the starting position (off).
   * chop the current mbuf into two pieces, set off to 0.
   */
  if (len <= n->m_len - off) 
  {
    ng_mbuf_s *mlast;

    o = ___m_dup(n, off, n->m_len - off, M_DONTWAIT);
    if (o == NULL) 
    {
      m_free_one(m);
      return (NULL);  /* ENOBUFS */
    }
    for (mlast = o; mlast->m_next != NULL; mlast = mlast->m_next)
      ;
    n->m_len = off;
    mlast->m_next = n->m_next;
    n->m_next = o;
    n = o;
    off = 0;
    goto ok;
  }

  /*
   * we need to take hlen from <n, off> and tlen from <n->m_next, 0>,
   * and construct contiguous mbuf with m_len == len.
   * note that hlen + tlen == len, and tlen > 0.
   */
  hlen = n->m_len - off;
  tlen = len - hlen;

  /*
   * ensure that we have enough trailing data on mbuf chain.
   * if not, we can do nothing about the chain.
   */
  olen = 0;
  for (o = n->m_next; o != NULL; o = o->m_next)
    olen += o->m_len;
  if (hlen + olen < len) 
  {
    m_free_one(m);
    return (NULL);  /* mbuf chain too short */
  }

  /*
   * easy cases first.
   * we need to use m_copydata() to get data from <n->m_next, 0>.
   */
  if ((off == 0 || offp) 
    && m_trailingspace(n) >= tlen 
    && !sharedcluster) 
  {
    m_copydata(n->m_next, 0, tlen, mtod(n, caddr_t) + n->m_len);
    n->m_len += tlen;
    m_adj(n->m_next, tlen);
    goto ok;
  }
  if ((off == 0 || offp) && m_leadingspace(n->m_next) >= hlen &&
      !sharedcluster && n->m_next->m_len >= tlen) {
    n->m_next->m_data -= hlen;
    n->m_next->m_len += hlen;
    memmove(mtod(n->m_next, caddr_t), mtod(n, caddr_t) + off, hlen);
    n->m_len -= hlen;
    n = n->m_next;
    off = 0;
    goto ok;
  }

  /*
   * now, we need to do the hard way.  don't m_copym as there's no room
   * on both ends.
   */
  if (len > MAXMCLBYTES) 
  {
    m_free_one(m);
    return (NULL);
  }
  MGET(o, M_DONTWAIT, m->m_type);
  if (o && len > MLEN) 
  {
    MCLGETL(o, M_DONTWAIT, len);
    if ((o->m_flags & M_EXT) == 0) 
    {
      m_free(o);
      o = NULL;
    }
  }
  if (!o) 
  {
    m_free_one(m);
    return NULL;  /* ENOBUFS */
  }
  /* get hlen from <n, off> into <o, 0> */
  o->m_len = hlen;
  memmove(mtod(o, caddr_t), mtod(n, caddr_t) + off, hlen);
  n->m_len -= hlen;
  /* get tlen from <n->m_next, 0> into <o, hlen> */
  m_copydata(n->m_next, 0, tlen, mtod(o, caddr_t) + o->m_len);
  o->m_len += tlen;
  m_adj(n->m_next, tlen);
  o->m_next = n->m_next;
  n->m_next = o;
  n = o;
  off = 0;

ok:
  if (offp)
    *offp = off;
  return n;
}

void
m_print(void *v, int (*pr)(const char *, ...))
{
  ng_mbuf_s *m = v;

  (*pr)("mbuf %p\n", m);
  (*pr)("m_type: %i\tm_flags: %b\n", m->m_type, m->m_flags, M_BITS);
  (*pr)("m_next: %p\n", m->m_next);
  (*pr)("m_data: %p\tm_len: %u\n", m->m_data, m->m_len);
  (*pr)("m_dat: %p\tm_pktdat: %p\n", m->m_dat, m->m_pktdat);
  if (m->m_flags & M_PKTHDR) {
    (*pr)("m_ptkhdr.ph_ifidx: %u\tm_pkthdr.len: %i\n",
        m->m_pkthdr.ph_ifidx, m->m_pktlen);
    (*pr)("m_ptkhdr.ph_tags: %p\tm_pkthdr.ph_tagsset: %b\n",
        list_first_entry(&m->m_pkthdr.ph_tags, ng_mtag_s, m_tag_link),
        m->m_pkthdr.ph_tagsset, MTAG_BITS);
    (*pr)("m_pkthdr.ph_flowid: %u\tm_pkthdr.ph_loopcnt: %u\n",
        m->m_pkthdr.ph_flowid, m->m_pkthdr.ph_loopcnt);
    (*pr)("m_pkthdr.csum_flags: %b\n",
        m->m_pkthdr.csum_flags, MCS_BITS);
    (*pr)("m_pkthdr.ether_vtag: %u\tm_ptkhdr.ph_rtableid: %u\n",
        m->m_pkthdr.ether_vtag, m->m_pkthdr.ph_rtableid);
    (*pr)("m_pkthdr.pf.statekey: %p\tm_pkthdr.pf.inp %p\n",
        m->m_pkthdr.pf.statekey, m->m_pkthdr.pf.inp);
    (*pr)("m_pkthdr.pf.qid: %u\tm_pkthdr.pf.tag: %u\n",
        m->m_pkthdr.pf.qid, m->m_pkthdr.pf.tag);
    (*pr)("m_pkthdr.pf.flags: %b\n",
        m->m_pkthdr.pf.flags, MPF_BITS);
    (*pr)("m_pkthdr.pf.routed: %u\tm_pkthdr.pf.prio: %u\n",
        m->m_pkthdr.pf.routed, m->m_pkthdr.pf.prio);
  }
  if (m->m_flags & M_EXT) {
    (*pr)("m_ext.ext_buf: %p\tm_ext.ext_size: %u\n",
        m->m_ext.ext_buf, m->m_ext.ext_size);
  }
}

/* Get a packet tag structure along with specified data following. */
ng_mtag_s *
m_tag_get(int type, int len, int wait)
{
  ng_mtag_s *t;

  if (len < 0)
    return (NULL);
  t = __ng_malloc(wait, sizeof(*t));
  if (t == NULL)
    return NULL;
  t->m_tag_id = type;
  t->m_tag_len = len;
  return t;
}

/* Prepend a packet tag. */
void
m_tag_prepend(ng_mbuf_s *m, ng_mtag_s *t)
{
  list_add_head(&t->m_tag_link, &m->m_pkthdr.ph_tags);
  m->m_pkthdr.ph_tagsset |= t->m_tag_id;
}

/* Unlink and free a packet tag. */
void
m_tag_delete(ng_mbuf_s *m, ng_mtag_s *t)
{
  m->m_pkthdr.ph_tagsset &= ~t->m_tag_id;
  list_del(&t->m_tag_link);
  ng_free(t);
}

/* Unlink and free a packet tag chain. */
void
m_tag_delete_chain(ng_mbuf_s *m)
{
  while (!list_empty(&m->m_pkthdr.ph_tags)) 
  {
    ng_mtag_s *p = m_tag_first(m);
    list_del(&p->m_tag_link);
    ng_free(p);
  }
  m->m_pkthdr.ph_tagsset = 0;
}

/* Find a tag, starting from a given position. */
ng_mtag_s *
m_tag_find(ng_mbuf_s *m, int type, ng_mtag_s *t)
{
  ng_mtag_s *p;

  if (!(m->m_pkthdr.ph_tagsset & type))
    return (NULL);

  list_for_each_entry(p,ng_mtag_s,&m->m_pkthdr.ph_tags,m_tag_link)
  {
    if (p->m_tag_id == type)
      return p;
  }
  return NULL;
}

/* Copy a single tag. */
ng_mtag_s *
m_tag_copy(ng_mtag_s *t, int wait)
{
  ng_mtag_s *p;

  p = m_tag_get(t->m_tag_id, t->m_tag_len, wait);
  if (p == NULL)
    return NULL;
  ng_memcpy(p + 1, t + 1, t->m_tag_len); /* Copy the data */
  return p;
}

/*
 * Copy two tag chains. The destination mbuf (to) loses any attached
 * tags even if the operation fails. This should not be a problem, as
 * m_tag_copy_chain() is typically called with a newly-allocated
 * destination mbuf.
 */
int
m_tag_copy_chain(ng_mbuf_s *to, ng_mbuf_s *from, int wait)
{
  ng_list_s *e;

  m_tag_delete_chain(to);
  list_for_each(e, &from->m_pkthdr.ph_tags) 
  {
    ng_mtag_s *p, *t;
    p = list_entry(e, ng_mtag_s, m_tag_link);
    t = m_tag_copy(p, wait);
    if (t == NULL) 
    {
      m_tag_delete_chain(to);
      return NG_result_nmem;
    }
    list_add_tail(&t->m_tag_link, &to->m_pkthdr.ph_tags);
    to->m_pkthdr.ph_tagsset |= t->m_tag_id;
  }
  return NG_result_ok;
}

/* Initialize tags on an mbuf. */
void
m_tag_init(ng_mbuf_s *m)
{
  INIT_LIST_HEAD(&m->m_pkthdr.ph_tags);
}

/* Get first tag in chain. */
ng_mtag_s *
m_tag_first(ng_mbuf_s *m)
{
  return list_first_entry(&m->m_pkthdr.ph_tags, ng_mtag_s, m_tag_link);
}

/* Get next tag in chain. */
ng_mtag_s *
m_tag_next(ng_mtag_s *t)
{
  return list_next_entry(t, ng_mtag_s, m_tag_link);
}

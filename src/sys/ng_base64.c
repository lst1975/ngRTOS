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
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#include "ng_defs.h"
#include "ng_base64.h"

#define BASE64_PAD '='
#define BASE64DE_FIRST '+'
#define BASE64DE_LAST 'z'

/* BASE 64 encode table */
static const char base64en[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/',
};

/* ASCII order for BASE 64 decode, 255 in unused character */
static const unsigned char base64de[] = {
  /* nul, soh, stx, etx, eot, enq, ack, bel, */
     255, 255, 255, 255, 255, 255, 255, 255,

  /*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
     255, 255, 255, 255, 255, 255, 255, 255,

  /* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
     255, 255, 255, 255, 255, 255, 255, 255,

  /* can,  em, sub, esc,  fs,  gs,  rs,  us, */
     255, 255, 255, 255, 255, 255, 255, 255,

  /*  sp, '!', '"', '#', '$', '%', '&', ''', */
     255, 255, 255, 255, 255, 255, 255, 255,

  /* '(', ')', '*', '+', ',', '-', '.', '/', */
     255, 255, 255,  62, 255, 255, 255,  63,

  /* '0', '1', '2', '3', '4', '5', '6', '7', */
      52,  53,  54,  55,  56,  57,  58,  59,

  /* '8', '9', ':', ';', '<', '=', '>', '?', */
      60,  61, 255, 255, 255, 255, 255, 255,

  /* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
     255,   0,   1,  2,   3,   4,   5,    6,

  /* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
       7,   8,   9,  10,  11,  12,  13,  14,

  /* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
      15,  16,  17,  18,  19,  20,  21,  22,

  /* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
      23,  24,  25, 255, 255, 255, 255, 255,

  /* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
     255,  26,  27,  28,  29,  30,  31,  32,

  /* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
      33,  34,  35,  36,  37,  38,  39,  40,

  /* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
      41,  42,  43,  44,  45,  46,  47,  48,

  /* 'x', 'y', 'z', '{', '|', '}', '~', del, */
      49,  50,  51, 255, 255, 255, 255, 255
};

void
b64Encode_Start(b64_state_s *s)
{
  s->s = 0;
  s->l = 0;
  s->j = 0;
}

int
b64Encode_Finish(b64_state_s *state, unsigned char *out, unsigned int outlen)
{
  switch (state->s) {
  case 1:
    if (state->j + 3 > outlen)
      return -1;
    out[state->j++] = base64en[(state->l & 0x3) << 4];
    out[state->j++] = BASE64_PAD;
    out[state->j++] = BASE64_PAD;
    break;
  case 2:
    if (state->j + 2 > outlen)
      return -1;
    out[state->j++] = base64en[(state->l & 0xF) << 2];
    out[state->j++] = BASE64_PAD;
    break;
  }

  if (state->j < outlen)
    out[state->j] = '\0';

  return state->j;
}

int
b64Encode_Finish_Buf(b64_state_s *state, ng_buffer_s *out)
{
  int len = b64Encode_Finish(state, out->ptr, out->len);
  if (len < 0) return len;
  out->len += len;
  return len;
}

int
b64Encode_Process(b64_state_s *state, const unsigned char *in, 
  unsigned int inlen, unsigned char *out, unsigned int outlen)
{
  unsigned char c;
  unsigned int i;

  for (i = state->j = 0; i < inlen; i++) 
  {
    c = in[i];

    switch (state->s) {
    case 0:
      state->s = 1;
      if (state->j + 1 > outlen)
        return -1;
      out[state->j++] = base64en[(c >> 2) & 0x3F];
      break;
    case 1:
      state->s = 2;
      if (state->j + 1 > outlen)
        return -1;
      out[state->j++] = base64en[((state->l & 0x3) << 4) | ((c >> 4) & 0xF)];
      break;
    case 2:
      state->s = 0;
      if (state->j + 2 > outlen)
        return -1;
      out[state->j++] = base64en[((state->l & 0xF) << 2) | ((c >> 6) & 0x3)];
      out[state->j++] = base64en[c & 0x3F];
      break;
    }
    state->l = c;
  }

  return state->j;
}

int
b64Encode_Process_Buf(b64_state_s *state, const ng_buffer_s *in, 
  ng_buffer_s *out)
{
  int len = b64Encode_Process(state, in->ptr, in->len, 
                  &out->ptr[out->len], out->size-out->len);
  if (len < 0) return len;
  out->len += len;
  return len;
}

int
b64Encode(const unsigned char *in, unsigned int inlen, 
  unsigned char *out, unsigned int outlen)
{
  int len;
  b64_state_s s;

  b64Encode_Start(&s);
  len = b64Encode_Process(&s, in, inlen, out, outlen);
  if (len < 0) return len;
  return b64Encode_Finish(&s, &out[len], outlen-len);
}

int
b64Encode_Buf(const ng_buffer_s *in, ng_buffer_s *out)
{
  int len;
  b64_state_s s;

  b64Encode_Start(&s);
  len = b64Encode_Process_Buf(&s, in, out);
  if (len < 0) return len;
  return b64Encode_Finish_Buf(&s, out);
}

int
b64Decode_Process(b64_state_s *state, const char *in, 
  unsigned int inlen, unsigned char *out, unsigned int outlen)
{
  unsigned int i;
  unsigned char c;

  if (inlen & 0x3 || !outlen) {
    return -1;
  }

  for (i = 0; i < inlen; i++) 
  {
    if (in[i] == BASE64_PAD) 
    {
      break;
    }
    if (in[i] < BASE64DE_FIRST || in[i] > BASE64DE_LAST) 
    {
      return -1;
    }

    c = base64de[(unsigned char)in[i]];
    if (c == 255) 
    {
      return -1;
    }

    switch (i & 0x3) 
    {
    case 0:
      out[state->j] = (c << 2) & 0xFF;
      break;
    case 1:
      if (state->j + 1 > outlen)
        return -1;
      out[state->j++] |= (c >> 4) & 0x3;
      out[state->j] = (c & 0xF) << 4; 
      break;
    case 2:
      if (state->j + 1 > outlen)
        return -1;
      out[state->j++] |= (c >> 2) & 0xF;
      out[state->j] = (c & 0x3) << 6;
      break;
    case 3:
      if (state->j + 1 > outlen)
        return -1;
      out[state->j++] |= c;
      break;
    }
  }

  return state->j;
}

#if __configUseBase64Decode
int
b64Decode_Process_Buf(b64_state_s *state, const ng_buffer_s *in, 
  ng_buffer_s *out)
{
  int len = b64Decode_Process(state, (char *)in->ptr, in->len, 
                  &out->ptr[out->len], out->size-out->len);
  if (len < 0) return len;
  out->len += len;
  return len;
}

int
b64Decode(const char *in, unsigned int inlen, 
  unsigned char *out, unsigned int outlen)
{
  int len;
  b64_state_s s;

  b64Decode_Start(&s);
  len = b64Decode_Process(&s, in, inlen, out, outlen);
  if (len < 0) return len;
  return b64Decode_Finish(&s);
}

int
b64Decode_Buf(const ng_buffer_s *in, ng_buffer_s *out)
{
  int len;
  b64_state_s s;

  b64Decode_Start(&s);
  len = b64Decode_Process(&s, (char *)in->ptr, in->len, 
            &out->ptr[out->len], out->size-out->len);
  if (len < 0) return len;
  return b64Decode_Finish(&s);
}
#endif

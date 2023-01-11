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
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __ngRTOS_BASE64_H__
#define __ngRTOS_BASE64_H__

#include "ng_buffer.h"

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) << 2 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) >> 4) * 3))

struct b64_state{
	int16_t s;
	int16_t j;
	unsigned char c;
	unsigned char l;
  uint16_t pad;
};
typedef struct b64_state b64_state_s;

/*
 * out is null-terminated encode string.
 * return values is out length, exclusive terminating `\0'
 */
int b64Encode(const unsigned char *in, unsigned int inlen, 
                  unsigned char *out, unsigned int outlen);
int b64Encode_Buf(const ng_buffer_s *in, ng_buffer_s *out);

void b64Encode_Start(b64_state_s *s);
int b64Encode_Finish(b64_state_s *s, unsigned char *out, 
                  unsigned int outlen);
int b64Encode_Finish_Buf(b64_state_s *state, ng_buffer_s *out);

int b64Encode_Process(b64_state_s *s, const unsigned char *in, 
                  unsigned int inlen, unsigned char *out, unsigned int outlen);
int b64Encode_Process_Buf(b64_state_s *state, 
                  const ng_buffer_s *in, ng_buffer_s *out);

#if __configUseBase64Decode
/*
 * return values is out length
 */
int b64Decode(const char *in, unsigned int inlen, 
                  unsigned char *out, unsigned int outlen);
int b64Decode_Buf(const ng_buffer_s *in, ng_buffer_s *out);
  
#define b64Decode_Start b64Encode_Start
#define b64Decode_Finish(state) ((state)->j)
int b64Decode_Process_Buf(b64_state_s *state, 
                  const ng_buffer_s *in, ng_buffer_s *out);
#endif

#endif

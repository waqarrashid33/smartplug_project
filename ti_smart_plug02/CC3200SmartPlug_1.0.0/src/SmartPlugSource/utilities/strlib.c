/*****************************************************************************
*
*  strlib.c - Generic helper functions
*  Copyright (C) 2012 Exosite LLC
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Exosite LLC nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "strlib.h"


/*****************************************************************************
*
*  itoa
*
*  \param  n is the number to be converted to ASCII
*          s is a pointer to an array where the ASCII string will be placed
*          b is the base (10 for decimal)
*
*  \return None
*
*  \brief  Integer to ASCII
*
*****************************************************************************/
char *itoa(int n, char *s, int b) {
  const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  unsigned int i = 0;
  int sign;

  if ((sign = n) < 0)
    n = -n;

  do {
    s[i++] = digits[n % b];
  } while ((n /= b) > 0);

  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';

  return strrev(s);
}

/*****************************************************************************
*
*  strrev
*
*  \param  str is a pointer to the string to be reversed
*
*  \return None
*
*  \brief  Reverses a string
*
*****************************************************************************/
char *strrev(char *str) {
  char *p1, *p2;

  if (!str || !*str)
    return str;

  for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
    *p1 ^= *p2;
    *p2 ^= *p1;
    *p1 ^= *p2;
  }

  return str;
}

/* Little endian format */
void WordToBytes(void *pBuff, void const*pvalue, unsigned int NumBytes)
{
  unsigned char *pSource, *pDest;
  int byte = 0;

  pSource = (unsigned char *)pvalue;
  pDest = (unsigned char *)pBuff;

  for(byte = 0; byte < NumBytes; byte++)
  {
    *pDest++ = *pSource++;
  }
}



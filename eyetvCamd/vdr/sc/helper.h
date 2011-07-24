/*
 * Softcam plugin to VDR (C++)
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#ifndef ___HELPER_H
#define ___HELPER_H

#include <architecture/byte_order.h>

#if defined __i386__
#define get_misalign(_a)    *(_a)
#define put_misalign(_b,_a) *(_a)=(_b)
#else
template<class T> inline T get_misalign(T *p)
{ struct s { T v; } __attribute__((packed)); return ((s *)p)->v; }

template<class T> inline void put_misalign(unsigned int v, T* p)
{ struct s { T v; } __attribute__((packed)); ((s *)p)->v = v; }
#endif

#define UINT32_BE(a) NXSwapInt(get_misalign((unsigned int *)(a)))
#define UINT16_BE(a) NXSwapShort(get_misalign((unsigned short *)(a)))
#define UINT8_BE(a)  *((unsigned char *)a)
#define BYTE4_BE(a,b) put_misalign(NXSwapInt(b),(unsigned int *)(a))
#define BYTE2_BE(a,b) put_misalign(NXSwapShort(b),(unsigned short *)(a))
#define BYTE1_BE(a,b) *(unsigned char *)(a)=(b)

#define UINT32_LE(a) get_misalign((unsigned int *)(a))
#define UINT16_LE(a) get_misalign((unsigned short *)(a))
#define UINT8_LE(a)  *((unsigned char *)a)
#define BYTE4_LE(a,b) put_misalign(b,(unsigned int *)(a))
#define BYTE2_LE(a,b) put_misalign(b,(unsigned short *)(a))
#define BYTE1_LE(a,b) *(unsigned char *)(a)=(b)

#define __ror16_const(x,xx) ((((x)&0xFFFF)>>(xx)) | ((x)<<(16-(xx))))
#define __rol16_const(x,xx) (((x)<<(xx)) | (((x)&0xFFFF)>>(16-(xx))))
#define __sn8_const(b)      ((((b)>>4)&0xf) + (((b)&0xf)<<4))

#if defined __GNUC__ && __GNUC__ >= 2 && defined __i386__

#define ror16(x,xx) ({ unsigned int v; \
                     __asm__ ("rorw %2, %w0" : "=g" (v) : "0" (x), "i" (xx) : "cc"); \
                     v; })
#define rol16(x,xx) ({ unsigned int v; \
                     __asm__ ("rolw %2, %w0" : "=g" (v) : "0" (x), "i" (xx) : "cc"); \
                     v; })
#define sn8(b)      ({ unsigned char v; \
                     if(__builtin_constant_p(b)) \
                       v=__sn8_const(b); \
                     else \
                       __asm__ ("rorb $4, %b0" : "=g" (v) : "0" (b) : "cc"); \
                     v; })

#else //__GNUC__

#define ror16(x,xx) __ror16_const(x,xx)
#define rol16(x,xx) __rol16_const(x,xx)
#define sn8(b)      __sn8_const(b)

#endif //__GNUC__

static inline void __xxor(unsigned char *data, int len, const unsigned char *v1, const unsigned char *v2)
{
  switch(len) { // looks ugly, but the compiler can optimize it very well ;)
    case 16:
      *((unsigned int *)data+3) = *((unsigned int *)v1+3) ^ *((unsigned int *)v2+3);
      *((unsigned int *)data+2) = *((unsigned int *)v1+2) ^ *((unsigned int *)v2+2);
    case 8:
      *((unsigned int *)data+1) = *((unsigned int *)v1+1) ^ *((unsigned int *)v2+1);
    case 4:
      *((unsigned int *)data+0) = *((unsigned int *)v1+0) ^ *((unsigned int *)v2+0);
      break;
    default:
      while(len--) *data++ = *v1++ ^ *v2++;
      break;
    }
}
#define xxor(d,l,v1,v2) __xxor(d,l,v1,v2)

#endif // ___HELPER_H

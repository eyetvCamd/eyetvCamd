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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../tools.h"
#include "../thread.h"

#include "misc.h"

// -----------------------------------------------------------------------------

void DvbName(const char *Name, int n, char *buffer, int len)
{
  snprintf(buffer,len,"/dev/dvb/adapter%d/%s%d",n,Name,0);
}

int DvbOpen(const char *Name, int n, int Mode, bool ReportError)
{
  char FileName[128];
  DvbName(Name,n,FileName,sizeof(FileName));
  int fd=open(FileName,Mode);
  if(fd<0 && ReportError) LOG_ERROR_STR(FileName);
  return fd;
}

const char *HexStr(char *str, const unsigned char *mem, int len)
{
  for(int i=0 ; i<len ; i++) sprintf(&str[i*2],"%02X",mem[i]);
  return str;
}

static int HexDigit(int asc)
{
  if(asc>='A') return toupper(asc)-'A'+10;
  return asc-'0';
}

int GetChar(const char * &line, int *store, int count)
{
  line=skipspace(line);
  const char *sline=line;
  *store=0;
  while(count) {
    if(line[0]==0 || !isalnum(line[0])) break;
    *store<<=8; *store+=line[0];
    line++; count--;
    }
  if(!count && (!*line || isspace(*line) || *line==';')) return sline-line;
  *store=0;
  line=sline;
  return 0;
}

static int GetHexBase(const char * &line, unsigned char *store, int count, bool fixedLen, bool asc)
{
  line=skipspace(line);
  const char *sline=line;
  unsigned char *sstore=store;
  int scount=count;
  while(count) {
    if(line[0]==0 || line[1]==0 || !isxdigit(line[0]) || !isxdigit(line[1])) break;
    if(asc) {
       *store++=line[0]; *store++=line[1];
       }
    else {
      int d1=HexDigit(line[0]);
      int d0=HexDigit(line[1]);
      *store++=(d1<<4) + d0;
      }
    line+=2; count--;
    }
  if(asc) *store=0; // make sure strings are NULL terminated
  if((!count || (!fixedLen && count!=scount)) &&
     (!*line || isspace(*line) || *line==';' || *line==']' || *line=='/') ) return scount-count;
  memset(sstore,0,scount);
  line=sline;
  return 0;
}

int GetHex(const char * &line, unsigned char *store, int count, bool fixedLen)
{
  return GetHexBase(line,store,count,fixedLen,false);
}

int GetHexAsc(const char * &line, unsigned char *store, int count)
{
  return GetHexBase(line,store,count/2,true,true)*2;
}

unsigned long long Bin2LongLong(unsigned char *mem, int len)
{
  unsigned long long k=0;
  for(int i=0 ; i<len ; i++) { k<<=8; k+=mem[i]; }
  return k;
}

bool CheckNull(const unsigned char *data, int len)
{
  while(--len>=0) if(data[len]) return false;
  return true;
}

bool CheckFF(const unsigned char *data, int len)
{
  while(--len>=0) if(data[len]!=0xFF) return false;
  return true;
}

unsigned char XorSum(const unsigned char *mem, int len)
{
  unsigned char cs=0;
  while(len>0) { cs ^= *mem++; len--; }
  return cs;
}

// crc stuff taken from linux-2.6.0/lib/crc32.c
/*
 * There are multiple 16-bit CRC polynomials in common use, but this is
 * *the* standard CRC-32 polynomial, first popularized by Ethernet.
 * x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0
 */
#define CRCPOLY_LE 0xedb88320

unsigned int crc32_le(unsigned int crc, unsigned char const *p, int len)
{
  crc^=0xffffffff; // zlib mod
  while(len--) {
    crc^=*p++;
    for(int i=0; i<8; i++)
      crc=(crc&1) ? (crc>>1)^CRCPOLY_LE : (crc>>1);
    }
  crc^=0xffffffff; // zlib mod
  return crc;
}

// -- cLineBuff -----------------------------------------------------------------

cLineBuff::cLineBuff(int blocksize)
{
  blockSize=blocksize;
  work=0;
  blen=wlen=0;
}

cLineBuff::~cLineBuff()
{
  free(work);
}

void cLineBuff::Flush(void)
{
  blen=0;
}

char *cLineBuff::Grab(void)
{
  char *w=work;
  work=0; wlen=0;
  Flush();
  return w;
}

bool cLineBuff::Check(int num)
{
  if(wlen-blen<num) {
    if(num<blockSize) num=blockSize;
    char *w=(char *)realloc(work,wlen+num);
    if(w) {
      wlen+=num;
      work=w;
      }
    }
  return work!=0;
}

void cLineBuff::Printf(const char *fmt, ...)
{
  int q=120;
  while(Check(q)) {
    int s=wlen-blen;
    va_list ap;
    va_start(ap,fmt);
    q=vsnprintf(work+blen,s,fmt,ap);
    va_end(ap);
    if(q<s) {
      if(q>0) blen+=q;
      break;
      }
    // truncated
    q+=100;
    }
}

void cLineBuff::Strcat(const char *str)
{
  if(str) Printf("%s",str);
}

void cLineBuff::Back(int n)
{
  if(n>blen) blen=0; else blen-=n;
  if(work) work[blen]=0;
}

// -- cSimpleListBase --------------------------------------------------------------

cSimpleListBase::cSimpleListBase(void)
{
  first=last=0; count=0;
}

cSimpleListBase::~cSimpleListBase()
{
  Clear();
}

void cSimpleListBase::Add(cSimpleItem *Item, cSimpleItem *After)
{
  if(After) {
    Item->next=After->next;
    After->next=Item;
    }
  else {
    Item->next=0;
    if(last) last->next=Item;
    else first=Item;
    }
  if(!Item->next) last=Item;
  count++;
}

void cSimpleListBase::Ins(cSimpleItem *Item)
{
  Item->next=first;
  first=Item;
  if(!Item->next) last=Item;
  count++;
}

void cSimpleListBase::Del(cSimpleItem *Item, bool Del)
{
  if(first==Item) {
    first=Item->next;
    if(!first) last=0;
    }
  else {
    cSimpleItem *item=first;
    while(item) {
      if(item->next==Item) {
        item->next=Item->next;
        if(!item->next) last=item;
        break;
        }
      item=item->next;
      }
    }
  count--;
  if(Del) delete Item;
}

void cSimpleListBase::Clear(void)
{
  while(first) Del(first);
  first=last=0; count=0;
}

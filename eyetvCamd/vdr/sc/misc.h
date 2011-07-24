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

#ifndef ___MISC_H
#define ___MISC_H

#include <alloca.h>

// ----------------------------------------------------------------

#define WORD(buffer,index,mask) (((buffer[(index)]<<8) + buffer[(index)+1]) & mask)
#define SCT_LEN(sct) (3+(((sct)[1]&0x0f)<<8)+(sct)[2])

#define MBC(a,b)    (((a)<<8)+(b))
#define ADDC3(ab,c) ((ab)+((c)<<16))
#define MBC3(a,b,c) ADDC3(MBC((a),(b)),(c))
#define C2(x)       (((x)>>8)&0xFF)
#define C3(x)       (((x)>>16)&0xFF)
#define C2MASK      0xFFFF

// replacement for variable-sized arrays
#define AUTOARRAY(type,size) (type *)alloca(sizeof(type)*(size))
#define AUTOMEM(size)        (unsigned char *)alloca(size)

// ----------------------------------------------------------------

#define DEV_DVB_FRONTEND "frontend"
#define DEV_DVB_DVR      "dvr"
#define DEV_DVB_DEMUX    "demux"
#define DEV_DVB_CA       "ca"
void DvbName(const char *Name, int n, char *buffer, int len);
int DvbOpen(const char *Name, int n, int Mode, bool ReportError=false);

const char *HexStr(char *str, const unsigned char *mem, int len);
#define KeyStr(str,key) HexStr(str,key,8)

int GetHex(const char * &line, unsigned char *store, int count, bool fixedLen=true);
int GetHexAsc(const char * &line, unsigned char *store, int count);
int GetChar(const char * &line, int *store, int count);
unsigned long long Bin2LongLong(unsigned char *mem, int len);
#define Bin2Int(mem,len) ((int)Bin2LongLong((mem),(len)))

bool CheckNull(const unsigned char *data, int len);
bool CheckFF(const unsigned char *data, int len);
unsigned char XorSum(const unsigned char *mem, int len);
unsigned int crc32_le(unsigned int crc, unsigned char const *p, int len);

// ----------------------------------------------------------------

class cLineBuff {
private:
  int blockSize;
  char *work;
  int blen, wlen;
  //
  bool Check(int num);
protected:
  char *Grab(void);
public:
  cLineBuff(int blocksize);
  ~cLineBuff();
  void Printf(const char *fmt, ...) __attribute__ ((format (printf,2,3)));
  void Strcat(const char *str);
  void Flush(void);
  void Back(int n);
  const char *Line(void) const { return work; }
  int Length(void) const { return blen; }
  };

// ----------------------------------------------------------------

class cSimpleListBase;

class cSimpleItem {
friend class cSimpleListBase;
private:
  cSimpleItem *next;
public:
  virtual ~cSimpleItem() {}
  cSimpleItem *Next(void) const { return next; }
  };

class cSimpleListBase {
protected:
  cSimpleItem *first, *last;
  int count;
public:
  cSimpleListBase(void);
  ~cSimpleListBase();
  void Add(cSimpleItem *Item, cSimpleItem *After=0);
  void Ins(cSimpleItem *Item);
  void Del(cSimpleItem *Item, bool Del=true);
  void Clear(void);
  int Count(void) const { return count; }
  };

template<class T> class cSimpleList : public cSimpleListBase {
public:
  T *First(void) const { return (T *)first; }
  T *Last(void) const { return (T *)last; }
  T *Next(const T *item) const { return (T *)item->cSimpleItem::Next(); }
  };

#endif //___MISC_H

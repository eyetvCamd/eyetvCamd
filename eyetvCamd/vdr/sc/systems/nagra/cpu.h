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

#ifndef __NAGRA_CPU_H
#define __NAGRA_CPU_H

#define FILEMAP_DOMAIN "nagra"

class cFileMap;
class cLineBuff;
class c6805;

// ----------------------------------------------------------------

class cMap {
friend class c6805;
protected:
  cLineBuff *loglb;
public:
  virtual ~cMap() {};
  virtual unsigned char Get(unsigned short ea)=0;
  virtual void Set(unsigned short ea, unsigned char val)=0;
  virtual bool IsFine(void)=0;
};

// ----------------------------------------------------------------

class cMapMem : public cMap {
protected:
  unsigned short offset;
  unsigned char *mem;
  int size;
public:
  cMapMem(unsigned short Offset, int Size);
  virtual ~cMapMem();
  virtual unsigned char Get(unsigned short ea);
  virtual void Set(unsigned short ea, unsigned char val);
  virtual bool IsFine(void);
  };

// ----------------------------------------------------------------

class cMapRom : public cMap {
private:
  unsigned short offset;
  cFileMap *fm;
  unsigned char *addr;
  int size;
public:
  cMapRom(unsigned short Offset, const char *Filename, int InFileOffset=0);
  virtual ~cMapRom();
  virtual unsigned char Get(unsigned short ea);
  virtual void Set(unsigned short ea, unsigned char val);
  virtual bool IsFine(void);
  };

// ----------------------------------------------------------------

class cMapEeprom : public cMap {
private:
  unsigned short offset;
  cFileMap *fm;
  unsigned char *addr;
  int size, otpSize;
public:
  cMapEeprom(unsigned short Offset, const char *Filename, int OtpSize, int InFileOffset=0);
  virtual ~cMapEeprom();
  virtual unsigned char Get(unsigned short ea);
  virtual void Set(unsigned short ea, unsigned char val);
  virtual bool IsFine(void);
  };

// ----------------------------------------------------------------

#define MAX_BREAKPOINTS 24
#define MAX_MAPPER      10
#define MAX_PAGES       5
#define PAGE_SIZE       32*1024

#define bitset(d,bit) (((d)>>(bit))&1)

#define HILO(ea)      ((Get(ea)<<8)+Get((ea)+1))
#define HILOS(s,ea)   ((Get((s),(ea))<<8)+Get((s),(ea)+1))

class c6805 {
private:
  unsigned short pc, sp, spHi, spLow;
  unsigned short bp[MAX_BREAKPOINTS], numBp;
  unsigned char mapMap[(MAX_PAGES+1)*PAGE_SIZE];
  cMap *mapper[MAX_MAPPER];
  int nextMapper;
  int pageMap[256];
  bool indirect;
  unsigned int clockcycles;
  //
  void InitMapper(void);
  void ClearMapper(void);
  void branch(bool branch);
  inline void tst(unsigned char c);
  inline void push(unsigned char c);
  inline unsigned char pop(void);
  void pushpc(void);
  void poppc(void);
  void pushc(void);
  void popc(void);
  unsigned char add(unsigned char op, unsigned char c);
  unsigned char sub(unsigned char op1, unsigned char op2, unsigned char c);
  unsigned char rollR(unsigned char op, unsigned char c);
  unsigned char rollL(unsigned char op, unsigned char c);
protected:
  unsigned char a, x, y, cr, dr;
  struct CC { unsigned char c, z, n, i, h, v; } cc;
  bool hasReadHandler, hasWriteHandler;
  //
  int Run(int max_count);
  void AddBreakpoint(unsigned short addr);
  void ClearBreakpoints(void);
  bool AddMapper(cMap *map, unsigned short start, int size, unsigned char seg=0);
  void ResetMapper(void);
  unsigned char Get(unsigned short ea) const;
  unsigned char Get(unsigned char seg, unsigned short ea) const;
  void Set(unsigned short ea, unsigned char val);
  void Set(unsigned char seg, unsigned short ea, unsigned char val);
  void GetMem(unsigned short addr, unsigned char *data, int len, unsigned char seg=0) const;
  void SetMem(unsigned short addr, const unsigned char *data, int len, unsigned char seg=0);
  void ForceSet(unsigned short ea, unsigned char val, bool ro);
  void SetSp(unsigned short SpHi, unsigned short SpLow);
  void SetPc(unsigned short addr, unsigned char seg=0);
  unsigned short GetPc(void) const { return pc; }
  void PopPc(void);
  void PopCr(void);
  void Push(unsigned char c);
  void ResetCycles(void);
  void AddCycles(unsigned int num);
  unsigned int Cycles(void) { return clockcycles; }
  virtual void Stepper(void)=0;
  virtual void WriteHandler(unsigned char seg, unsigned short ea, unsigned char &op) {}
  virtual void ReadHandler(unsigned char seg, unsigned short ea, unsigned char &op) {}
  virtual void TimerHandler(unsigned int num) {}
private:
  unsigned int stats[256];
  char addrBuff[32];
  bool doDisAsm;
  int disAsmLogClass;
protected:
  cLineBuff *loglb;
  char * PADDR(unsigned char s, unsigned short ea);
public:
  c6805(void);
  virtual ~c6805();
  };

#endif

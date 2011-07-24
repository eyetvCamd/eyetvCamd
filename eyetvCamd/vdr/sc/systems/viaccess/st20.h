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

#ifndef __ST20_H
#define __ST20_H

class cLineBuff;

// ----------------------------------------------------------------

#define IPTR 0
#define WPTR 1
#define AREG 2
#define BREG 3
#define CREG 4

#define FLASHS	0x7FE00000
#define FLASHE	0x7FFFFFFF
#define RAMS	0x40000000
#define RAME	0x401FFFFF
#define IRAMS	0x80000000
#define IRAME	0x800017FF

#define ERR_ILL_OP -1
#define ERR_CNT    -2

// ----------------------------------------------------------------

#define STACKMAX  16
#define STACKMASK (STACKMAX-1)

class cST20 {
private:
  unsigned int Iptr, Wptr;
  unsigned char *flash, *ram;
  unsigned int flashSize, ramSize;
  int sptr, stack[STACKMAX];
  unsigned char iram[0x1800];
  int invalid;
  //
  bool verbose;
  cLineBuff *loglb;
  //
  unsigned char *Addr(unsigned int off);
  void LogOp(const char *op);
  void LogOpOper(int op, int oper);
public:
  cST20(void);
  ~cST20();
  void Init(unsigned int IPtr, unsigned int WPtr);
  void SetCallFrame(unsigned int raddr, int p1, int p2, int p3);
  void SetFlash(unsigned char *m, int len);
  void SetRam(unsigned char *m, int len);
  int Decode(int count);
  //
  unsigned int GetReg(int reg) const;
  void SetReg(int reg, unsigned int val);
  unsigned int ReadWord(unsigned int off);
  unsigned short ReadShort(unsigned int off);
  unsigned char ReadByte(unsigned int off);
  void WriteWord(unsigned int off, unsigned int val);
  void WriteShort(unsigned int off, unsigned short val);
  void WriteByte(unsigned int off, unsigned char val);
  };

#endif // __ST20_H

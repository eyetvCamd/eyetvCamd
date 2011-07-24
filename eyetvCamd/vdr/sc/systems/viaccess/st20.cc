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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../../tools.h"

#include "log-viaccess.h"
#include "st20.h"

#include "helper.h"
#include "misc.h"

//#define SAVE_DEBUG // add some (slow) failsafe checks and stricter emulation

#define INVALID_VALUE 	0xCCCCCCCC
#define ERRORVAL	0xDEADBEEF

#define MININT		0x7FFFFFFF
#define MOSTPOS		0x7FFFFFFF
#define MOSTNEG		0x80000000

#ifndef SAVE_DEBUG
#define POP() stack[(sptr++)&STACKMASK]
#else
#define POP() ({ int __t=stack[sptr&STACKMASK]; stack[(sptr++)&STACKMASK]=INVALID_VALUE; __t; })
#endif
#define PUSH(v) do { int __v=(v); stack[(--sptr)&STACKMASK]=__v; } while(0)
#define DROP(n) sptr+=n

#define AAA stack[sptr&STACKMASK]
#define BBB stack[(sptr+1)&STACKMASK]
#define CCC stack[(sptr+2)&STACKMASK]

#define GET_OP()        operand|=op1&0x0F
#define CLEAR_OP()      operand=0
#define JUMP(x)         Iptr+=(x)
#define POP64()         ({ unsigned int __b=POP(); ((unsigned long long)POP()<<32)|__b; })
#define PUSHPOP(op,val) do { int __a=val; AAA op##= (__a); } while(0)

#ifndef SAVE_DEBUG
#define RB(off) UINT8_LE(Addr(off))
#define RW(off) UINT32_LE(Addr(off))
#define WW(off,val) BYTE4_LE(Addr(off),val)
#else
#define RB(off) ReadByte(off)
#define RW(off) ReadWord(off)
#define WW(off,val) WriteWord(off,val)
#endif

#define LOG_OP(op) do { if(v) LogOp(op); } while(0)

// -- cST20 --------------------------------------------------------------------

cST20::cST20(void)
{
  flash=ram=0;
  loglb=new cLineBuff(128);
}

cST20::~cST20()
{
  free(flash);
  free(ram);
  delete loglb;
}

void cST20::SetFlash(unsigned char *m, int len)
{
  free(flash);
  flash=MALLOC(unsigned char,len);
  if(flash && m) memcpy(flash,m,len);
  else memset(flash,0,len);
  flashSize=len;
}

void cST20::SetRam(unsigned char *m, int len)
{
  free(ram);
  ram=MALLOC(unsigned char,len);
  if(ram && m) memcpy(ram,m,len);
  else memset(ram,0,len);
  ramSize=len;
}

void cST20::Init(unsigned int IPtr, unsigned int WPtr)
{
  Wptr=WPtr; Iptr=IPtr;
  memset(stack,INVALID_VALUE,sizeof(stack)); sptr=STACKMAX-3;
  memset(iram,0,sizeof(iram));
  verbose=LOG(L_SYS_DISASM);
}

void cST20::SetCallFrame(unsigned int raddr, int p1, int p2, int p3)
{
  Wptr-=16;
  WriteWord(Wptr,raddr); // RET
  WriteWord(Wptr+4,0);
  WriteWord(Wptr+8,0);
  WriteWord(Wptr+12,p1);
  WriteWord(Wptr+16,p2);
  WriteWord(Wptr+20,p3);
  SetReg(AREG,raddr);    // RET
}

unsigned int cST20::GetReg(int reg) const
{
  switch(reg) {
    case IPTR: return Iptr;
    case WPTR: return Wptr;
    case AREG: return AAA;
    case BREG: return BBB;
    case CREG: return CCC;
    default:   PRINTF(L_SYS_ST20,"getreg: unknown reg"); return ERRORVAL;
    }
}

void cST20::SetReg(int reg, unsigned int val)
{
  switch(reg) {
    case IPTR: Iptr=val; return;
    case WPTR: Wptr=val; return;
    case AREG: AAA=val; return;
    case BREG: BBB=val; return;
    case CREG: CCC=val; return;
    default:   PRINTF(L_SYS_ST20,"setreg: unknown reg"); return;
    }
}

unsigned char *cST20::Addr(unsigned int off)
{
  if(off>=FLASHS && off<=FLASHE) {
#ifndef SAVE_DEBUG
    return &flash[off-FLASHS];
#else
    off-=FLASHS; if(off<flashSize && flash) return &flash[off]; break;
#endif
    }
  else if(off>=RAMS && off<=RAME) {
#ifndef SAVE_DEBUG
    return &ram[off-RAMS];
#else
    off-=RAMS; if(off<ramSize && ram) return &ram[off]; break;
#endif
    }
  else if(off>=IRAMS && off<=IRAME)
    return &iram[off-IRAMS];
#ifndef SAVE_DEBUG
  invalid=ERRORVAL; return (unsigned char *)&invalid;
#else
  return 0;
#endif
}

unsigned int cST20::ReadWord(unsigned int off)
{
#ifndef SAVE_DEBUG
  return UINT32_LE(Addr(off));
#else
  unsigned char *addr=Addr(off);
  return addr ? UINT32_LE(addr) : ERRORVAL;
#endif
}

unsigned short cST20::ReadShort(unsigned int off)
{
#ifndef SAVE_DEBUG
  return UINT16_LE(Addr(off));
#else
  unsigned char *addr=Addr(off);
  return addr ? UINT16_LE(addr) : ERRORVAL>>16;
#endif
}

unsigned char cST20::ReadByte(unsigned int off)
{
#ifndef SAVE_DEBUG
  return UINT8_LE(Addr(off));
#else
  unsigned char *addr=Addr(off);
  return addr ? UINT8_LE(addr) : ERRORVAL>>24;
#endif
}

void cST20::WriteWord(unsigned int off, unsigned int val)
{
#ifndef SAVE_DEBUG
  BYTE4_LE(Addr(off),val);
#else
  unsigned char *addr=Addr(off);
  if(addr) BYTE4_LE(addr,val);
#endif
}

void cST20::WriteShort(unsigned int off, unsigned short val)
{
#ifndef SAVE_DEBUG
  BYTE2_LE(Addr(off),val);
#else
  unsigned char *addr=Addr(off);
  if(addr) BYTE2_LE(addr,val);
#endif
}

void cST20::WriteByte(unsigned int off, unsigned char val)
{
#ifndef SAVE_DEBUG
  BYTE1_LE(Addr(off),val);
#else
  unsigned char *addr=Addr(off);
  if(addr) BYTE1_LE(addr,val);
#endif
}

#define OP_COL 20

void cST20::LogOpOper(int op, int oper)
{
  const static char *cmds[] = { "j","ldlp",0,"ldnl","ldc","ldnlp",0,"ldl","adc","call","cj","ajw","eqc","stl","stnl",0 };
  const static char flags[] = {  3,  0,    0, 0,     1,    0,     0, 0,    1,    3,     3,   0,    1,    0,    0,    0 };
  if(oper==0) loglb->Printf("%08X %02X",Iptr-1,op);
  else        loglb->Printf("%02X",op);
  oper|=op&0xF; op>>=4;
  if(!cmds[op]) return;
  int n=loglb->Length();
  if(flags[op]&2) oper+=Iptr;
  if(flags[op]&1) loglb->Printf("%*s%-5s $%-8X ",max(OP_COL-n,1)," ",cmds[op],oper);
  else            loglb->Printf("%*s%-5s  %-8d ",max(OP_COL-n,1)," ",cmds[op],oper);
}

void cST20::LogOp(const char *op)
{
  int n=loglb->Length();
  loglb->Printf("%*s%-15s ",max(OP_COL-n,1)," ",op);
}

int cST20::Decode(int count)
{
  int operand;
  bool v=verbose;
  CLEAR_OP();
  while(Iptr!=0) {
    int a, op1=RB(Iptr++);
    if(v) LogOpOper(op1,operand);
    GET_OP();
    switch(op1>>4) {
      case 0x0: // j / jump
#ifdef SAVE_DEBUG
        POP(); POP(); POP();
#endif
        JUMP(operand);
        CLEAR_OP();
        break;
      case 0x1: // ldlp
        PUSH(Wptr+(operand*4));
        CLEAR_OP();
        break;
      case 0x2: // positive prefix
        operand<<=4;
        break;
      case 0x3: // ldnl
        AAA=RW(AAA+(operand*4));
        CLEAR_OP();
        break;
      case 0x4: // ldc
        PUSH(operand);
        CLEAR_OP();
        break;
      case 0x5: // ldnlp
        PUSHPOP(+,operand*4);
        CLEAR_OP();
        break;
      case 0x6: // negative prefix
        operand=(~operand)<<4;
        break;
      case 0x7: // ldl
        PUSH(RW(Wptr+(operand*4)));
        CLEAR_OP();
        break;
      case 0x8: // adc
        PUSHPOP(+,operand);
        CLEAR_OP();
        break;
      case 0x9: // call
        Wptr-=16;
        WW(Wptr,Iptr); WW(Wptr+4,POP()); WW(Wptr+8,POP()); WW(Wptr+12,POP());
        PUSH(Iptr);
        JUMP(operand);
        CLEAR_OP();
        break;
      case 0xA: // cj / conditional jump
        if(AAA) DROP(1); else JUMP(operand);
        CLEAR_OP();
        break;
      case 0xB: // ajw / adjust workspace
        Wptr+=operand*4;
        CLEAR_OP();
        break;
      case 0xC: // eqc / equals constant
        AAA=(operand==AAA ? 1 : 0);
        CLEAR_OP();
        break;
      case 0xD: // stl
        WW(Wptr+(operand*4),POP());
        CLEAR_OP();
        break;
      case 0xE: // stnl
        a=POP(); WW(a+(operand*4),POP());
        CLEAR_OP();
        break;
      case 0xF: // opr (secondary ins)
        switch(operand) {
	  case  0x00: LOG_OP("rev");   a=AAA; AAA=BBB; BBB=a; break;
	  case  0x01: LOG_OP("lb");    AAA=RB(AAA); break;
	  case  0x02: LOG_OP("bsub");  PUSHPOP(+,POP()); break;
	  case  0x04: LOG_OP("diff");  PUSHPOP(-,POP()); break;
	  case  0x05: LOG_OP("add");   PUSHPOP(+,POP()); break;
	  case  0x06: LOG_OP("gcall"); a=AAA; AAA=Iptr; Iptr=a; break;
	  case  0x08: LOG_OP("prod");  PUSHPOP(*,POP()); break;
	  case  0x09: LOG_OP("gt");    a=POP(); AAA=(AAA>a); break;
	  case  0x0A: LOG_OP("wsub");  a=POP(); AAA=a+(AAA*4); break;
	  case  0x0C: LOG_OP("sub");   PUSHPOP(-,POP()); break;

          case  0x1B: LOG_OP("ldpi");  PUSHPOP(+,Iptr); break;
          case  0x20: LOG_OP("ret");   Iptr=RW(Wptr); Wptr=Wptr+16; break;
          case  0x2C: LOG_OP("div");   PUSHPOP(/,POP()); break;
          case  0x32: LOG_OP("not");   AAA=~AAA; break;
          case  0x33: LOG_OP("xor");   PUSHPOP(^,POP()); break;
          case  0x34: LOG_OP("bcnt");  PUSHPOP(*,4); break;
          case  0x3B: LOG_OP("sb");    a=POP(); WriteByte(a,POP()); break;
          case  0x3F: LOG_OP("wcnt");  a=POP(); PUSH(a&3); PUSH((unsigned int)a>>2); break;
          case  0x40: LOG_OP("shr");   a=POP(); AAA=(unsigned int)AAA>>a; break;
          case  0x41: LOG_OP("shl");   a=POP(); AAA=(unsigned int)AAA<<a; break;
          case  0x42: LOG_OP("mint");  PUSH(MOSTNEG); break;
          case  0x46: LOG_OP("and");   PUSHPOP(&,POP()); break;
          case  0x4A: LOG_OP("move");  { a=POP(); int b=POP(); int c=POP(); while(a--) WriteByte(b++,ReadByte(c++)); } break;
          case  0x4B: LOG_OP("or");    PUSHPOP(|,POP()); break;
          case  0x53: LOG_OP("mul");   PUSHPOP(*,POP()); break;
          case  0x5A: LOG_OP("dup");   PUSH(AAA); break;
          case  0x5F: LOG_OP("gtu");   a=POP(); AAA=((unsigned int)AAA>(unsigned int)a); break;
          case  0x1D: LOG_OP("xdble"); CCC=BBB; BBB=(AAA>=0 ? 0:-1); break;
          case  0x1A: LOG_OP("ldiv");  { a=POP(); unsigned long long ll=POP64(); PUSH(ll%(unsigned int)a); PUSH(ll/(unsigned int)a); } break;
          case  0x1F: LOG_OP("rem");   PUSHPOP(%,POP()); break;
          case  0x35: LOG_OP("lshr");  { a=POP(); unsigned long long ll=POP64()>>a; PUSH((ll>>32)&0xFFFFFFFF); PUSH(ll&0xFFFFFFFF); } break;

          case  0xCA: LOG_OP("ls");    AAA=ReadShort(AAA); break;
          case  0xCD: LOG_OP("gintdis"); break;
          case  0xCE: LOG_OP("gintenb"); break;

          case -0x40: LOG_OP("nop");   break;

	  default: 
            if(verbose) PUTLB(L_SYS_DISASM,loglb);
            PRINTF(L_SYS_ST20,"unknown opcode %X",operand);
            return ERR_ILL_OP;
	  }
        CLEAR_OP();
        break;
      }

    if(v && operand==0) {
      loglb->Printf("%08X %08X %08X W:%08X",AAA,BBB,CCC,Wptr);
      PUTLB(L_SYS_DISASM,loglb);
      }
    if(--count<=0 && operand==0) {
      PRINTF(L_SYS_ST20,"instruction counter exceeded");
      return ERR_CNT;
      }
    }
  return 0;
}

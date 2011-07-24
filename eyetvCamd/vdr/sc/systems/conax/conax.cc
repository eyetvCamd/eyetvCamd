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
#include <string.h>
#include <ctype.h>

#include "system-common.h"
#include "crypto.h"
#include "data.h"
#include "misc.h"
#include "log-sys.h"

#define SYSTEM_CONAX         0x0B00

#define SYSTEM_NAME          "Conax"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_CONAX)

#define L_SYS        3
#define L_SYS_ALL    LALL(L_SYS_LASTDEF)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF)&LOPT_MASK,
  "conax",
  { L_SYS_DEFNAMES }
  };
ADD_MODULE(L_SYS,lm_sys)

// -- cSystemConax ---------------------------------------------------------------

class cSystemConax : public cSystem {
private:
  cRSA rsa;
  //
  void ParseECM(unsigned char *buf, int len);
public:
  cSystemConax(void);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *source);
  };

cSystemConax::cSystemConax(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{}

void cSystemConax::ParseECM(unsigned char *buf, int len)
{
  int i=0;
  while(i<len) {
    int nano=buf[i++];
    int nanolen=buf[i++];
    
    switch(nano) {
      case 0x80:
      case 0x82:
        ParseECM(buf+i+3,nanolen-3); break;
      case 0x81:
        ParseECM(buf+i+2,nanolen-2); break;
      case 0x30:
	memcpy(cw  ,buf+i+10,8);
	memcpy(cw+8,buf+i+2 ,8);
        break;
      }
    i+=nanolen;
    }
}

bool cSystemConax::ProcessECM(const cEcmInfo *ecm, unsigned char *source)
{
  int length=source[4]-2;
  const int keyid=source[6];
  const int nano=source[5];
  source+=7;

  cKeySnoop ks(this,'C',keyid,'M');
  cPlainKey *pk;
  cBN mod, exp;
  if(!(pk=keys.FindKey('C',keyid,'E',-1))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %02X E key",keyid);
    return false;
    }
  pk->Get(exp);
  if(!(pk=keys.FindKey('C',keyid,'M',-1))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %02X M key",keyid);
    return false;
    }
  pk->Get(mod);

  for(int i=length ; i>0 ;) {
    int index=i-64;
    if(index<0) index=0;
    if(rsa.RSA(source+index,source+index,64,exp,mod,false)<=0) {
      if(doLog) PRINTF(L_SYS_CRYPTO,"RSA failed");
      return false;
      }
    if(nano==0x63) {
      // nano 0x63 block only decodes to 63 bytes
      memmove(source+index,source+index+1,length-(index+1));
      length-=1;
      }
    i-=(i%60 && index) ? i%60 : 64;
    }

  static const unsigned char hash[] = { 0x05,0x00,0x05 };
  if(memcmp(hash,source+2,3) || memcmp(source+5,source+length-5,3)) {
    if(doLog) PRINTF(L_SYS_ECM,"signature check failed");
    return false;
    }

  ParseECM(source+10,length-10);
  ks.OK(pk);
  return true;
}

// -- cPlainKeyConax --------------------------------------------------------------

#define PLAINLEN_CONAX 64

class cPlainKeyConax : public cBNKey {
protected:
  virtual cString PrintKeyNr(void);
public:
  cPlainKeyConax(bool Super);
  virtual bool Parse(const char *line);
  };

static cPlainKeyTypeReg<cPlainKeyConax,'C'> KeyReg;

cPlainKeyConax::cPlainKeyConax(bool Super)
:cBNKey(Super)
{}

bool cPlainKeyConax::Parse(const char *line)
{
  unsigned char sid, skey[PLAINLEN_CONAX];
  if(GetChar(line,&type,1) && GetHex(line,&sid,1) &&
     GetChar(line,&keynr,1) && GetHex(line,skey,PLAINLEN_CONAX)) {
    type=toupper(type); keynr=toupper(keynr); id=sid;
    SetBinKey(skey,PLAINLEN_CONAX);
    return true;
    }
  return false;
}

cString cPlainKeyConax::PrintKeyNr(void)
{
  return cString::sprintf("%c",keynr);
}

// -- cSystemLinkConax ---------------------------------------------------------

class cSystemLinkConax : public cSystemLink {
public:
  cSystemLinkConax(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemConax; }
  };

static cSystemLinkConax staticInit;

cSystemLinkConax::cSystemLinkConax(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkConax::CanHandle(unsigned short SysId)
{
  SysId&=SYSTEM_MASK;
  return SYSTEM_CAN_HANDLE(SysId);
}

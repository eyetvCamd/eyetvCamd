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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <typeinfo>

#include <channels.h>
#include <sources.h>

#include "system.h"
#include "system-common.h"
#include "data.h"
#include "misc.h"

#define SYSTEM_NAME          "ConstCW"
#define SYSTEM_PRI           -20

// -- cPlainKeyConstCw ----------------------------------------------------------------

#define PLAINLEN_CW 16

class cPlainKeyConstCw : public cHexKey {
private:
  int prgId, source, transponder;
  int freq;
  char pol;
protected:
  virtual int IdSize(void) { return 4; }
  virtual cString PrintKeyNr(void);
public:
  cPlainKeyConstCw(bool Super);
  virtual bool Parse(const char *line);
  bool Matches(const cEcmInfo *ecm);
  };

static cPlainKeyTypeReg<cPlainKeyConstCw,'X'> KeyReg;

cPlainKeyConstCw::cPlainKeyConstCw(bool Super)
:cHexKey(Super)
{
  freq=-1;
}

bool cPlainKeyConstCw::Matches(const cEcmInfo *ecm)
{
  return ecm->prgId==prgId && ecm->source==source && ecm->transponder==transponder;
}

bool cPlainKeyConstCw::Parse(const char *line)
{
  unsigned char caid[2], skey[PLAINLEN_CW];
  if(GetChar(line,&type,1) && GetHex(line,caid,2)) {
    int num;
    char srcBuf[16];
    if(sscanf(line," %d:%c:%15[^:]:%d%n",&freq,&pol,srcBuf,&prgId,&num)==4) {
      source=cSource::FromString(srcBuf);
      transponder=freq;
      while(transponder>20000) transponder/=1000;
      if(cSource::IsSat(source)) transponder=cChannel::Transponder(transponder,pol);
      line+=num;
      if(GetHex(line,skey,PLAINLEN_CW)) {
        type=toupper(type);
        id=Bin2Int(caid,2);
        keynr=0;
        SetBinKey(skey,PLAINLEN_CW);
        return true;
        }
      }
    }
  return false;
}

cString cPlainKeyConstCw::PrintKeyNr(void)
{
  return freq<0 ? "" : cString::sprintf("%d:%c:%s:%d",freq,pol,*cSource::ToString(source),prgId);
}

// -- cSystemConstCw ------------------------------------------------------------------

class cSystemConstCw : public cSystem {
public:
  cSystemConstCw(void);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *source);
  };

cSystemConstCw::cSystemConstCw(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
  constant=true;
}

bool cSystemConstCw::ProcessECM(const cEcmInfo *ecm, unsigned char *source)
{
  cKeySnoop ks(this,'X',ecm->caId,0);
  cPlainKey *pk=0;
  while((pk=keys.FindKey('X',ecm->caId,0,16,pk))) {
    if(typeid(*pk)==typeid(cPlainKeyConstCw) && ((cPlainKeyConstCw *)pk)->Matches(ecm)) {
      pk->Get(cw);
      ks.OK(pk);
      return true;
      }
    }
  return false;
}

// -- cSystemLinkConstCw ------------------------------------------------------------

class cSystemLinkConstCw : public cSystemLink {
public:
  cSystemLinkConstCw(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemConstCw; }
  };

static cSystemLinkConstCw staticInit;

cSystemLinkConstCw::cSystemLinkConstCw(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkConstCw::CanHandle(unsigned short SysId)
{
  return keys.FindKeyNoTrig('X',SysId,0,16)!=0;
}

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

#ifndef ___PARSE_H
#define ___PARSE_H

#include "misc.h"

// ----------------------------------------------------------------

void SetSctLen(unsigned char *data, int len);

// ----------------------------------------------------------------

class cAssSct : public cSimpleItem {
private:
  const unsigned char *data;
public:
  cAssSct(const unsigned char *Data);
  const unsigned char *Data(void) const { return data; }
  };

class cAssembleData : private cSimpleList<cAssSct> {
private:
  const unsigned char *data;
  cAssSct *curr;
public:
  cAssembleData(const unsigned char *Data);
  void SetAssembled(const unsigned char *Data);
  const unsigned char *Assembled(void);
  const unsigned char *Data(void) const { return data; }
  };

class cProvider : public cSimpleItem {
public:
  virtual bool MatchID(const unsigned char *data)=0;
  virtual bool MatchEMM(const unsigned char *data)=0;
  virtual unsigned long long ProvId(void)=0;
  virtual int UpdateType(const unsigned char *data) { return 2; }
  virtual int Assemble(cAssembleData *ad) { return 0; }
  };

class cProviders : public cSimpleList<cProvider> {
public:
  void AddProv(cProvider *p);
  cProvider *FindProv(const unsigned char *data);
  cProvider *MatchProv(const unsigned char *data);
  };  

class cCard {
public:
  virtual ~cCard() {}
  virtual bool MatchEMM(const unsigned char *data)=0;
  virtual int UpdateType(const unsigned char *data) { return 3; }
  virtual int Assemble(cAssembleData *ad) { return 0; }
  };

class cIdSet : public cProviders {
protected:
  cCard *card;
public:
  cIdSet(void);
  ~cIdSet();
  void SetCard(cCard *c);
  void ResetIdSet(void);
  bool MatchEMM(const unsigned char *data);
  bool MatchAndAssemble(cAssembleData *ad, int *updtype, cProvider **p);
  };

// -- IRDETO ------------------------------------------------------

class cProviderIrdeto : public cProvider {
public:
  unsigned char provBase;
  unsigned char provId[3];
  //
  cProviderIrdeto(void) {}
  cProviderIrdeto(unsigned char pb, const unsigned char *pi);
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  };

class cCardIrdeto : public cCard {
public:
  unsigned char hexBase;
  unsigned char hexSer[3];
  //
  cCardIrdeto(void) {}
  cCardIrdeto(unsigned char hb, const unsigned char *hs);
  virtual bool MatchEMM(const unsigned char *data);
  virtual int UpdateType(const unsigned char *data);
  };

class cParseIrdeto {
public:
  static unsigned int AddrLen(const unsigned char *data);
  static unsigned int AddrBase(const unsigned char *data);
  };

// -- SECA --------------------------------------------------------

struct SecaCmd {
  unsigned char tid;
  unsigned char cmdLen1;
  unsigned char cmdLen2;
  };

struct SecaEcm {
  struct SecaCmd cmd;
  unsigned char id[2];
  unsigned char unknown;
  unsigned char sm;
  unsigned char keyNr;
  };

struct SecaEmmShared {
  struct SecaCmd cmd;
  unsigned char id[2];
  unsigned char sa[3];
  unsigned char sm;
  unsigned char keyNr;
  };
      
struct SecaEmmUnique {
  struct SecaCmd cmd;
  unsigned char ua[6];
  unsigned char id[2];
  unsigned char sm;
  unsigned char keyNr;
  };

#define TID(x) (((struct SecaCmd *)(x))->tid)
#define SA(x)  (((struct SecaEmmShared *)(x))->sa)
#define SID(x) (((struct SecaEmmShared *)(x))->id)
#define UA(x)  (((struct SecaEmmUnique *)(x))->ua)
#define UID(x) (((struct SecaEmmUnique *)(x))->id)

class cProviderSeca : public cProvider {
public:
  unsigned char provId[2];
  unsigned char sa[3];
  //
  cProviderSeca(void) {}
  cProviderSeca(const unsigned char *pi, const unsigned char *s);
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  };

class cCardSeca : public cCard {
public:
  unsigned char ua[6];
  //
  cCardSeca(void) {}
  cCardSeca(const unsigned char *u);
  virtual bool MatchEMM(const unsigned char *data);
  };

class cParseSeca {
public:
  static int CmdLen(const unsigned char *data);
  static int Payload(const unsigned char *data, const unsigned char **payload);
  static int SysMode(const unsigned char *data);
  static int KeyNr(const unsigned char *data);
  static const unsigned char *ProvIdPtr(const unsigned char *data);
  static int ProvId(const unsigned char *data);
  };

// -- VIACCESS -----------------------------------------------------

class cProviderViaccess : public cProvider {
private:
  unsigned char *sharedEmm;
  int sharedLen, sharedToggle;
public:
  unsigned char ident[3];
  unsigned char sa[4];
  //
  cProviderViaccess(void);
  cProviderViaccess(const unsigned char *id, const unsigned char *s);
  virtual ~cProviderViaccess();
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  virtual int UpdateType(const unsigned char *data);
  virtual int Assemble(cAssembleData *ad);
  };

class cCardViaccess : public cCard {
public:
  unsigned char ua[5];
  //
  cCardViaccess(void) {}
  cCardViaccess(const unsigned char *u);
  virtual bool MatchEMM(const unsigned char *data);
  };

class cParseViaccess {
public:
  static const unsigned char *NanoStart(const unsigned char *data);
  static const unsigned char *CheckNano90(const unsigned char *data);
  static const unsigned char *CheckNano90FromNano(const unsigned char *data);
  static int KeyNr(const unsigned char *data);
  static int KeyNrFromNano(const unsigned char *data);
  static const unsigned char *ProvIdPtr(const unsigned char *data);
  static int ProvId(const unsigned char *data);
  };

// -- NAGRA 2 ------------------------------------------------------

class cCardNagra2 : public cCard {
public:
  unsigned char addr[4];
  //
  cCardNagra2(const unsigned char *a);
  virtual bool MatchEMM(const unsigned char *data);
  virtual int UpdateType(const unsigned char *data);
  };

// -- CONAX --------------------------------------------------------

class cProviderConax : public cProvider {
public:
  unsigned char addr[7];
  //
  cProviderConax(const unsigned char *a);
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  };

class cCardConax : public cCard {
public:
  unsigned char addr[7];
  //
  cCardConax(const unsigned char *a);
  virtual bool MatchEMM(const unsigned char *data);
  };

// -- CRYPTOWORKS --------------------------------------------------

class cProviderCryptoworks : public cProvider {
public:
  unsigned char addr[5];
  //
  cProviderCryptoworks(const unsigned char *a);
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  };

class cCardCryptoworks : public cCard {
private:
  unsigned char *sharedEmm;
  int sharedLen, globalToggle;
  unsigned int globalCrc;
public:
  unsigned char addr[5];
  //
  cCardCryptoworks(const unsigned char *a);
  virtual ~cCardCryptoworks();
  virtual bool MatchEMM(const unsigned char *data);
  virtual int UpdateType(const unsigned char *data);
  virtual int Assemble(cAssembleData *ad);
  };

// -- NDS -----------------------------------------------------------

class cProviderNDS : public cProvider {
public:
  unsigned char sa[4];
  //
  cProviderNDS(const unsigned char *s);
  virtual bool MatchID(const unsigned char *data);
  virtual bool MatchEMM(const unsigned char *data);
  virtual unsigned long long ProvId(void);
  virtual int Assemble(cAssembleData *ad);
  };

class cCardNDS : public cCard {
public:
  unsigned char ua[4];
  //
  cCardNDS(const unsigned char *u);
  virtual bool MatchEMM(const unsigned char *data);
  virtual int Assemble(cAssembleData *ad);
  };

class cParseNDS {
public:
  static unsigned int NumAddr(const unsigned char *data);
  static int AddrMode(const unsigned char *data);
  static bool IsSA(const unsigned char *data);
  static bool HasAddr(const unsigned char *data, const unsigned char *a);
  static const unsigned char *PayloadStart(const unsigned char *data);
  static int PayloadSize(const unsigned char *data);
  static int Assemble(cAssembleData *ad, const unsigned char *a);
  };

#endif //___PARSE_H

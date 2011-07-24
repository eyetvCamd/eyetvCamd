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

#ifndef ___SYSTEM_COMMON_H
#define ___SYSTEM_COMMON_H

#include "data.h"
#include "system.h"

// ----------------------------------------------------------------

class cHexKey : public cPlainKey {
private:
  int keylen;
  unsigned char *key;
protected:
  virtual bool SetKey(void *Key, int Keylen);
  virtual bool SetBinKey(unsigned char *Mem, int Keylen);
  virtual cString Print(void);
  virtual bool Cmp(void *Key, int Keylen);
  virtual bool Cmp(cPlainKey *k);
  virtual void Get(void *mem);
  virtual int Size(void) { return keylen; }
public:
  cHexKey(bool Super);
  virtual ~cHexKey();
  virtual bool Parse(const char *line) { return false; }
  };

// ----------------------------------------------------------------

class cBN;

class cBNKey : public cPlainKey {
private:
  bool rotate;
  int keylen;
  cBN *key;
protected:
  virtual bool SetKey(void *Key, int Keylen);
  virtual bool SetBinKey(unsigned char *Mem, int Keylen);
  virtual cString Print(void);
  virtual bool Cmp(void *Key, int Keylen);
  virtual bool Cmp(cPlainKey *k);
  virtual void Get(void *mem);
  virtual int Size(void) { return keylen; }
public:
  cBNKey(bool Super, bool Rotate=false);
  virtual ~cBNKey();
  virtual bool Parse(const char *line) { return false; }
  };

// ----------------------------------------------------------------

class cDualKey : public cMutableKey {
private:
  bool rotate;
protected:
  virtual bool IsBNKey(void) const=0;
  virtual cPlainKey *Alloc(void) const;
public:
  cDualKey(bool Super, bool Rotate);
  };

// ----------------------------------------------------------------

class cPlainKeyStd : public cHexKey {
public:
  cPlainKeyStd(bool Super);
  virtual bool Parse(const char *line);
  };

// ----------------------------------------------------------------

template<class T> class cCardInfos : public cStructList<T> {
public:
  cCardInfos(const char *Type, const char *Filename, int fl):cStructList<T>(Type,Filename,fl|SL_MISSINGOK|SL_WATCH) {}
  virtual cStructItem *ParseLine(char *line)
    {
      T *k=new T;
      if(k && !k->Parse(line)) { delete k; k=0; }
      return k;
    }
  };

// ----------------------------------------------------------------

class cSystemScCore : public cSystem {
private:
  int scId;
  const char *scName;
public:
  cSystemScCore(const char *Name, int Pri, int ScId, const char *ScName);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *data);
  virtual void ProcessEMM(int pid, int caid, unsigned char *data);
  };

#endif //___SYSTEM_COMMON_H

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

#ifndef ___OPTS_H
#define ___OPTS_H

#include "i18n.h"

class cOsdMenu;

// ----------------------------------------------------------------

class cOpt {
private:
  char *fullname;
  bool persistant;
protected:
  const char *name, *title;
  //
  const char *FullName(const char *PreStr);
public:
  cOpt(const char *Name, const char *Title);
  virtual ~cOpt();
  virtual void Parse(const char *Value)=0;
  virtual void Backup(void)=0;
  virtual bool Set(void)=0;
  virtual void Store(const char *PreStr)=0;
  virtual void Create(cOsdMenu *menu)=0;
  const char *Name(void) { return name; }
  void Persistant(bool per) { persistant=per; }
  bool Persistant(void) { return persistant; }
  };

// ----------------------------------------------------------------

class cOptInt : public cOpt {
protected:
  int *storage, value, min, max;
public:
  cOptInt(const char *Name, const char *Title, int *Storage, int Min, int Max);
  virtual void Parse(const char *Value);
  virtual void Backup(void);
  virtual bool Set(void);
  virtual void Store(const char *PreStr);
  virtual void Create(cOsdMenu *menu);
  };

// ----------------------------------------------------------------

class cOptSel : public cOptInt {
private:
  const char **trStrings;
protected:
  const char * const *strings;
public:
  cOptSel(const char *Name, const char *Title, int *Storage, int NumStr, const char * const *Strings);
  virtual ~cOptSel();
  virtual void Create(cOsdMenu *menu);
  };

// ----------------------------------------------------------------

class cOptBool : public cOptInt {
public:
  cOptBool(const char *Name, const char *Title, int *Storage);
  virtual void Create(cOsdMenu *menu);
  };

// ----------------------------------------------------------------

class cOptStr : public cOpt {
protected:
  char *storage, *value;
  const char *allowed;
  int size;
public:
  cOptStr(const char *Name, const char *Title, char *Storage, int Size, const char *Allowed);
  virtual ~cOptStr();
  virtual void Parse(const char *Value);
  virtual void Backup(void);
  virtual bool Set(void);
  virtual void Store(const char *PreStr);
  virtual void Create(cOsdMenu *menu);
  };

// ----------------------------------------------------------------

class cOpts {
private:
  int numOpts, numAdd;
  const char *preStr;
  cOpt **opts;
public:
  cOpts(const char *PreStr, int NumOpts);
  ~cOpts();
  void Add(cOpt *opt);
  bool Parse(const char *Name, const char *Value);
  bool Store(bool AsIs);
  void Backup(void);
  void Create(cOsdMenu *menu);
  };

#endif //___OPTS_H

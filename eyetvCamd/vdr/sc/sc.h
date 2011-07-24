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

#ifndef ___SC_H
#define ___SC_H

class cEcmInfo;
class cLogHook;

// ----------------------------------------------------------------

class cSoftCAM {
public:
  static bool Load(const char *cfgdir);
  static void Shutdown(void);
  //
  static bool Active(bool log);
  static char *CurrKeyStr(int CardNum, int num);
  static void SetLogStatus(int CardNum, const cEcmInfo *ecm, bool on);
  static void AddHook(int CardNum, cLogHook *hook);
  static bool TriggerHook(int CardNum, int id);
  };

#endif // ___SC_H

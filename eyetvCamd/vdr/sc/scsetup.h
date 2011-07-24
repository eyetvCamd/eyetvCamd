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

#ifndef ___SC_SETUP_H
#define ___SC_SETUP_H

#define MAXSCCAPS 10
#define MAXCAIGN  16

// ----------------------------------------------------------------

class cScSetup {
public:
  int AutoUpdate;
  int ScCaps[MAXSCCAPS];
  int ConcurrentFF;
  int CaIgnore[MAXCAIGN];
  int LocalPriority;
  int ForceTransfer;
  int PrestartAU;
  int SuperKeys;
public:
  cScSetup(void);
  void Check(void);
  void Store(bool AsIs);
  bool CapCheck(int n);
  bool Ignore(unsigned short caid);
  };

extern cScSetup ScSetup;

#endif

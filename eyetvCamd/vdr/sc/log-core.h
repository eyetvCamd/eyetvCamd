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

#ifndef ___LOG_CORE_H
#define ___LOG_CORE_H

#include "log.h"

// ----------------------------------------------------------------

#define L_CORE		1

#define L_CORE_LOAD     LCLASS(L_CORE,0x2)
#define L_CORE_ACTION	LCLASS(L_CORE,0x4)
#define L_CORE_ECM      LCLASS(L_CORE,0x8)
#define L_CORE_ECMPROC	LCLASS(L_CORE,0x10)
#define L_CORE_PIDS	LCLASS(L_CORE,0x20)
#define L_CORE_AU	LCLASS(L_CORE,0x40)
#define L_CORE_AUSTATS	LCLASS(L_CORE,0x80)
#define L_CORE_AUEXTRA	LCLASS(L_CORE,0x100)
#define L_CORE_AUEXTERN	LCLASS(L_CORE,0x200)
#define L_CORE_CAIDS    LCLASS(L_CORE,0x400)
#define L_CORE_KEYS	LCLASS(L_CORE,0x800)
#define L_CORE_DYN	LCLASS(L_CORE,0x1000)
#define L_CORE_CSA	LCLASS(L_CORE,0x2000)
#define L_CORE_CI	LCLASS(L_CORE,0x4000)
#define L_CORE_AV7110	LCLASS(L_CORE,0x8000)
#define L_CORE_NET	LCLASS(L_CORE,0x10000)
#define L_CORE_NETDATA	LCLASS(L_CORE,0x20000)
#define L_CORE_MSGCACHE	LCLASS(L_CORE,0x40000)
#define L_CORE_SERIAL	LCLASS(L_CORE,0x80000)
#define L_CORE_SC	LCLASS(L_CORE,0x100000)
#define L_CORE_HOOK	LCLASS(L_CORE,0x200000)
#define L_CORE_CIFULL	LCLASS(L_CORE,0x400000)

#define L_CORE_ALL      LALL(L_CORE_CIFULL)

#endif //___LOG_CORE_H

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

#ifndef __LOG_VIACCESS_H
#define __LOG_VIACCESS_H

#include "log-sys.h"

#define L_SYS        16
#define L_SYS_TPS    LCLASS(L_SYS,L_SYS_LASTDEF<<1)
#define L_SYS_TPSAU  LCLASS(L_SYS,L_SYS_LASTDEF<<2)
#define L_SYS_TIME   LCLASS(L_SYS,L_SYS_LASTDEF<<3)
#define L_SYS_ST20   LCLASS(L_SYS,L_SYS_LASTDEF<<4)
#define L_SYS_DISASM LCLASS(L_SYS,L_SYS_LASTDEF<<5)
#define L_SYS_ALL    LALL(L_SYS_DISASM)

#endif


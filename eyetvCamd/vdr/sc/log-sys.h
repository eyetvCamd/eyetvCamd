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

#ifndef ___LOG_SYS_H
#define ___LOG_SYS_H

#include "log.h"

#define L_SYS_KEY      LCLASS(L_SYS,0x2)
#define L_SYS_ECM      LCLASS(L_SYS,0x4)
#define L_SYS_EMM      LCLASS(L_SYS,0x8)
#define L_SYS_CRYPTO   LCLASS(L_SYS,0x10)
#define L_SYS_VERBOSE  LCLASS(L_SYS,0x20)

#define L_SYS_LASTDEF  L_SYS_VERBOSE
#define L_SYS_DEFDEF   L_SYS_KEY|L_SYS_ECM
#define L_SYS_DEFNAMES "key","ecm","emm","crypto","verbose"

#endif //___LOG_SYS_H

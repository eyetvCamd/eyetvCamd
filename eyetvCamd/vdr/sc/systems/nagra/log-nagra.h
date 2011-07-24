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

#ifndef __LOG_NAGRA_H
#define __LOG_NAGRA_H

#include "../../log-sys.h"

#define L_SYS          7
#define L_SYS_EMU      LCLASS(L_SYS,L_SYS_LASTDEF<<1)
#define L_SYS_DISASM   LCLASS(L_SYS,L_SYS_LASTDEF<<2)
#define L_SYS_DISASM80 LCLASS(L_SYS,L_SYS_LASTDEF<<3)
#define L_SYS_CPUSTATS LCLASS(L_SYS,L_SYS_LASTDEF<<4)
#define L_SYS_MAP      LCLASS(L_SYS,L_SYS_LASTDEF<<5)
#define L_SYS_RAWEMM   LCLASS(L_SYS,L_SYS_LASTDEF<<6)
#define L_SYS_RAWECM   LCLASS(L_SYS,L_SYS_LASTDEF<<7)
#define L_SYS_ALL      LALL(L_SYS_RAWECM)

#define bprint(a) {fprintf(stdout, #a "="); BN_print_fp(stdout,a); fprintf(stdout,"\n");}

#endif


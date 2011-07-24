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

#ifndef __NAGRA_NAGRA_DEF_H
#define __NAGRA_NAGRA_DEF_H

// ----------------------------------------------------------------

#define SYSTEM_NAGRA         0x1800
#define SYSTEM_NAGRA2        0x1801
#define SYSTEM_NAGRA_BEV     0x1234

// ----------------------------------------------------------------

// Nagra1 defines
#define DEF_TYPE 0
#define DEF_PK   2
#define DEF_ROM  0

#define TYPE(keynr)         (((keynr)>>16)&1)
#define PK(keynr)           (((keynr)>>17)&3)
#define ROM(keynr)          (((keynr)>>19)&15)
#define KEYSET(rom,pk,type) ((((rom)&15)<<3)|(((pk)&3)<<1)|((type)&1))

// Nagra2 defines
#define N2_MAGIC   0x80
#define N2_EMM_G_I 0x02
#define N2_EMM_G_R 0x12
#define N2_EMM_S_I 0x01
#define N2_EMM_S_R 0x11
#define N2_EMM_SEL 0x40
#define N2_EMM_V   0x03

#endif

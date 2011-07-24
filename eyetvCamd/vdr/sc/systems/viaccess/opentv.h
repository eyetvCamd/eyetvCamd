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

#ifndef __OPENTV_H
#define __OPENTV_H

#define COMP_SECTION_HDR 0x434F4D50
#define INFO_SECTION_HDR 0x494E464F
#define CODE_SECTION_HDR 0x434F4445
#define DATA_SECTION_HDR 0x44415441
#define GDBO_SECTION_HDR 0x4744424F
#define LAST_SECTION_HDR 0x4C415354
#define SWAP_SECTION_HDR 0x53574150

typedef struct comp_header {
	unsigned int magic;
	unsigned int csize, dsize, usize;
	unsigned char end;
} comp_header_t;

typedef struct info_header {
	unsigned int magic;
	unsigned int bsssize;
	unsigned int stacksize;
} info_header_t;

typedef struct code_header {
	unsigned int magic;
	unsigned int size, m_id;
	unsigned short entry_point;
	unsigned short end;
	const unsigned char *code;
} code_header_t;

typedef struct data_header {
	unsigned int magic;
	unsigned int dlen;
	const unsigned char *data;
} data_header_t;

#endif /* __OPENTV_H */

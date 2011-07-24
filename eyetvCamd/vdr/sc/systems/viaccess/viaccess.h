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

#ifndef __VIACCESS_VIACCESS_H
#define __VIACCESS_VIACCESS_H

#define SYSTEM_VIACCESS      0x0500

#define FILEMAP_DOMAIN       "viaccess"
#define TPSBIN               "tps.bin"
#define HOOK_SATTIME         ((SYSTEM_VIACCESS<<8)+0x01)
#define HOOK_TPSAU           ((SYSTEM_VIACCESS<<8)+0x02)

#endif

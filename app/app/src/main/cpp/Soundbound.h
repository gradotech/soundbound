/*
 * Soundbound ESP8266 Library
 * Dynamic audio panning control
 *
 * Copyright (C) 2023 Grado Technologies
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SOUNDBOUND_H__
#define __SOUNDBOUND_H__

#define SB_SERVER_PORT	1881
#define SB_STREAM_PORT	1882

#define SB_MAX_SPEAKERS		8
#define SB_DEV_NAME_SIZE	21

enum sb_command {
    SB_NO_CMD		= 0,

    SB_QUERY_DATA	= 1,
    SB_SET_VOLUME	= 2,
    SB_START		= 3,
    SB_STOP		= 4,

    SB_CMD_MAX	= UCHAR_MAX
};

struct sb_qdata_packet {
    jbyte major;			/* Major version number */
    jbyte minor;			/* Minor version number */
    char spks[SB_MAX_SPEAKERS];		/* Speaker IDs */
    char device[SB_DEV_NAME_SIZE];	/* Name of the HW controller */
};

struct sb_volume_packet {
    jbyte id;				/* Spekaer ID */
    jbyte volume;			/* Volume to be set */
    jbyte reserved;
};

#endif /* __SOUNDBOUND_H__ */

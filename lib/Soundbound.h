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

#include <cstdint>
#include <map>

#include <ESP8266WiFi.h>

#define SB_VERSION_MINOR	1
#define SB_VERSION_MAJOR	0

#define SB_MAX_SPEAKERS		8
#define SB_DEV_NAME_SIZE	21
#define SB_MAX_PCKT_SIZE	32

#define SB_IS_LONG_PACKET(cmd)	(	\
	((cmd) == SB_QUERY_DATA ||	\
	(cmd) == SB_SET_VOLUME)		\
)

/*
 * Communication interface
 */

enum sb_command {
	SB_NO_CMD	= 0,

	SB_QUERY_DATA	= 1,
	SB_SET_VOLUME	= 2,
	SB_START	= 3,
	SB_STOP		= 4,

	SB_CMD_MAX	= UCHAR_MAX
};

struct sb_qdata_packet {
	uint8_t cmd;			/* SB_QUERY_DATA */
	uint8_t major;			/* Major version number */
	uint8_t minor;			/* Minor version number */
	char spks[SB_MAX_SPEAKERS];	/* Speaker IDs */
	char device[SB_DEV_NAME_SIZE];	/* Name of the HW controller */
};

struct sb_volume_packet {
	uint8_t cmd;			/* SB_SET_VOLUME */
	char id;			/* Spekaer ID */
	uint8_t volume;			/* Volume to be set */
	uint8_t reserved;
};


/*
 * HW configuration interface
 */

enum sb_pmeter_cmd {
	SB_POT0_SEL	= 0x11,
	SB_POT1_SEL	= 0x12,
	SB_BOTH_POT_SEL	= 0x13,
};

struct sb_potentiometer {
	int8_t chipsel;			/* SPI chip select pin */
	uint8_t hw_vol_min;		/* HW volume lower bound */
	uint8_t hw_vol_max;		/* HW volume higher bound */
	sb_pmeter_cmd pot_sel;		/* Select witch pmeter to control */
};

struct sb_speaker {
	char spk_id;			/* Speaker unique identifier */
	uint8_t volume;			/* Initial volume */
	sb_potentiometer pmeter;	/* Potentiometer HW description */
};


/*
 * Class declaration
 */

class Potentiometer {

private:
	int8_t chipsel;
	uint8_t hw_vol_min;
	uint8_t hw_vol_max;
	sb_pmeter_cmd pot_sel;

public:
	Potentiometer(const sb_potentiometer& sb_potentiometer);
	int toHwVolume(uint8_t volume);
	uint8_t setVolume(uint8_t volume);
};

class Speaker {

private:
	char id;
	uint8_t volume;
	Potentiometer *pmeter;

public:
	Speaker(const sb_speaker& speaker);
	~Speaker();
	Potentiometer *getPotentiometer();
	void setVolume(uint8_t volume);
	int getVolume();
};

class Packet {
private:
	bool done;
	sb_command cmd;
	uint8_t buff[SB_MAX_PCKT_SIZE];
	uint8_t cursor;

public:
	Packet(sb_command cmd);
	sb_command getCmdType();
	void setDone(bool done);
	bool isDone();
	bool fillBuffer(uint8_t byte);
	uint8_t *getBuffer();
};

class Soundbound {

private:
	char device[SB_DEV_NAME_SIZE];
	uint8_t speakers_count;
	const sb_speaker *speakers_desc;
	std::map<char, Speaker *> m_speakers;

	bool capturePacket;
	Packet *packet;

public:
	Soundbound(const char *device, const sb_speaker *speakers, uint8_t count);
	~Soundbound();
	Speaker *getSpeaker(char id);
	void update(WiFiClient &client);
	void queryData(WiFiClient &client);
	sb_command cmdRecieved(WiFiClient &client);
	void doCapture(sb_command cmd, bool capture);
	bool hasPacket();
};

#endif /* __SOUNDBOUND_H__ */

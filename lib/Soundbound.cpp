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

#include <Arduino.h>
#include <SPI.h>

#include "Soundbound.h"

#define TX_BUFF_EMPTY -1
#define VOL_SCALE_MAX 100

/*
 * Soundbound
 */

Soundbound::Soundbound(const char *device, const sb_speaker *speakers, uint8_t count)
{
	uint8_t i;

	strcpy(this->device, device);

	for (i = 0; i < count; i++) {
		const sb_speaker& spk	= speakers[i];
		Speaker *speaker	= new Speaker(spk);

		this->m_speakers.emplace(spk.spk_id, speaker);
	}

	this->speakers_count = count;
	this->speakers_desc = speakers;

	SPI.begin();
}

Soundbound::~Soundbound()
{
	std::map<char, Speaker *>::iterator it;

	for (it = this->m_speakers.begin(); it != this->m_speakers.end(); it++)
		delete it->second;
}

Speaker *Soundbound::getSpeaker(char id)
{
	return this->m_speakers.at(id);
}

void Soundbound::update(WiFiClient &client)
{
	sb_volume_packet *volPacket = nullptr;

	if (hasPacket()) {
		Packet *pckt	= this->packet;
		int data	= client.read();

		if (data == TX_BUFF_EMPTY)
			pckt->setDone(true);

		if (!pckt->isDone()) {
			uint8_t byte = static_cast<uint8_t>(data);
			bool hasSpace = pckt->fillBuffer(byte);

			if (!hasSpace)
				pckt->setDone(true);
		} else if (pckt->getCmdType() == SB_SET_VOLUME) {
			uint8_t *buff = pckt->getBuffer();

			volPacket = reinterpret_cast<sb_volume_packet *>(buff);
		}
	}

	if (volPacket) {
		Speaker *speaker = this->getSpeaker(volPacket->id);
		int vol;

		speaker->setVolume(volPacket->volume);

		vol = speaker->getVolume();
		Serial.printf("Speaker '%c' hw vol %d\n", volPacket->id, vol);
		Serial.flush();

		doCapture(SB_NO_CMD, false);
	}
}

void Soundbound::queryData(WiFiClient &client)
{
	sb_qdata_packet qdata = { 0 };
	uint8_t *buff = (uint8_t *)&qdata;
	uint8_t i;

	strcpy(qdata.device, this->device);

	qdata.cmd	= SB_QUERY_DATA;
	qdata.major	= SB_VERSION_MAJOR;
	qdata.minor	= SB_VERSION_MINOR;

	for (i = 0; i < SB_MAX_SPEAKERS; i++) {
		if (i < this->speakers_count)
			qdata.spks[i] = this->speakers_desc[i].spk_id;
		else
			break;
	}

	for (i = 0; i < sizeof(qdata); i++)
		client.write(buff[i]);
}

sb_command Soundbound::cmdRecieved(WiFiClient &client)
{
	int byte = client.read();
	sb_command cmd;

	if (byte == TX_BUFF_EMPTY)
		return SB_NO_CMD;

	cmd = static_cast<sb_command>(byte);

	if (SB_IS_LONG_PACKET(cmd))
		doCapture(cmd, true);

	if (hasPacket())
		return packet->getCmdType();

	return cmd;
}

void Soundbound::doCapture(sb_command cmd, bool capture)
{
	if (capture && !hasPacket()) {
		this->packet = new Packet(cmd);
	} else if (!capture && hasPacket()) {
		delete this->packet;
		this->packet = nullptr;
	}

	this->capturePacket = capture;
}

bool Soundbound::hasPacket()
{
	return this->capturePacket;
}

/*
 * Speaker
 */

Speaker::Speaker(const sb_speaker& speaker)
{
	this->id	= speaker.spk_id;
	this->pmeter	= new Potentiometer(speaker.pmeter);

	this->volume	= pmeter->setVolume(speaker.volume);
}

Speaker::~Speaker()
{
	delete this->pmeter;
}

void Speaker::setVolume(uint8_t volume)
{
	Potentiometer *pmeter = this->getPotentiometer();

	this->volume = pmeter->setVolume(volume);
}

int Speaker::getVolume()
{
	return static_cast<int>(this->volume);
}

/*
 * Potentiometer
 */

Potentiometer::Potentiometer(const sb_potentiometer& pmeter)
{
	this->chipsel		= pmeter.chipsel;
	this->hw_vol_min	= pmeter.hw_vol_min;
	this->hw_vol_max	= pmeter.hw_vol_max;
	this->pot_sel		= pmeter.pot_sel;

	pinMode(this->chipsel, OUTPUT);
}

Potentiometer *Speaker::getPotentiometer()
{
	return this->pmeter;
}

int Potentiometer::toHwVolume(uint8_t volume)
{
	int hw_vol = static_cast<int>(volume);

	hw_vol = VOL_SCALE_MAX - hw_vol;
	hw_vol *= this->hw_vol_max - this->hw_vol_min;
	hw_vol /= VOL_SCALE_MAX;

	return hw_vol;
}

uint8_t Potentiometer::setVolume(uint8_t volume)
{
	int hw_vol = this->toHwVolume(volume);

	digitalWrite(this->chipsel, LOW);

	SPI.transfer(this->pot_sel);
	SPI.transfer(hw_vol);

	digitalWrite(this->chipsel, HIGH);

	delay(1);

	return static_cast<uint8_t>(hw_vol);
}

/*
 * Packet
 */

Packet::Packet(sb_command cmd)
{
	this->cmd	= cmd;
	this->cursor	= 0;
}

sb_command Packet::getCmdType()
{
	return this->cmd;
}

void Packet::setDone(bool done)
{
	this->done = done;
}

bool Packet::isDone()
{
	return this->done;
}

bool Packet::fillBuffer(uint8_t byte)
{
	if (this->cursor >= SB_MAX_PCKT_SIZE)
		return false;

	this->buff[this->cursor] = byte;
	this->cursor++;

	return true;
}

uint8_t *Packet::getBuffer()
{
	return this->buff;
}

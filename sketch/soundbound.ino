#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#include "Soundbound.h"

#define SERVER_PORT	1881
#define STREAM_PORT	1882
#define BUFF_SIZE	4096
#define SPKS_COUNT	2
#define URL_FORMAT	"http://%s:%d/stream"

const char *ssid = "";
const char *password = "";

WiFiServer server(SERVER_PORT);
WiFiClient client;

AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;

Soundbound *sb;

sb_speaker speaker[SPKS_COUNT] = {
	{
		.spk_id = 'a',
		.volume = 100,
		.pmeter = {
			.chipsel = 15,
			.hw_vol_min = 0,
			.hw_vol_max = 255,
			.pot_sel = SB_POT0_SEL
		}
	},
	{
		.spk_id = 'b',
		.volume = 100,
		.pmeter = {
			.chipsel = 15,
			.hw_vol_min = 0,
			.hw_vol_max = 255,
			.pot_sel = SB_POT1_SEL
		}
	},
};

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
	const char *ptr = reinterpret_cast<const char *>(cbData);
	char s1[32];
	char s2[64];

	strncpy_P(s1, type, sizeof(s1));
	s1[sizeof(s1) - 1] = 0;

	strncpy_P(s2, string, sizeof(s2));
	s2[sizeof(s2) - 1] = 0;
}

void statusCallback(void *cbData, int code, const char *string)
{
	const char *ptr = reinterpret_cast<const char *>(cbData);
	char s1[64];

	strncpy_P(s1, string, sizeof(s1));
	s1[sizeof(s1) - 1] = 0;

	Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
	Serial.flush();
}

void reinitAudioLib()
{
	const char *ip;
	char url[64];

	if (mp3)
		delete mp3;

	if (file)
		delete file;

	if (buff)
		delete buff;

	if (out)
		delete out;

	ip = client.remoteIP().toString().c_str();

	sprintf_P(url, URL_FORMAT, ip, STREAM_PORT);
	Serial.print("Stream URL: ");
	Serial.println(url);

	file = new AudioFileSourceICYStream(url);
	file->RegisterMetadataCB(MDCallback, (void *)"ICY");

	buff = new AudioFileSourceBuffer(file, BUFF_SIZE);
	buff->RegisterStatusCB(statusCallback, (void *)"buffer");

	out = new AudioOutputI2SNoDAC();

	mp3 = new AudioGeneratorMP3();
	mp3->RegisterStatusCB(statusCallback, (void *)"mp3");
	mp3->begin(buff, out);
}

void setup()
{
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
		delay(1000);

	server.begin();
	while (!client.connected()) {
		client = server.available();
		delay(100);
	}

	sb = new Soundbound("ESP8266", speaker, SPKS_COUNT);
	sb->queryData(client);

	audioLogger = &Serial;
}

inline bool playAudio()
{
	bool playing = true;

	if (!mp3)
		return false;

	if (mp3->isRunning()) {
		if (!mp3->loop()) {
			playing = false;

			mp3->stop();
		}
	}

	return playing;
}

void loop()
{
	if (client) {
		if(client.connected()) {
			sb_command cmd = sb->cmdRecieved(client);

			switch (cmd) {
			case SB_SET_VOLUME:
				sb->update(client);
				break;
			case SB_START:
				reinitAudioLib();
				break;
			case SB_STOP:
				mp3->stop();
				break;
			case SB_NO_CMD:
			default:
				/* Skip invalid commands */
				break;
			}

			if (!playAudio())
				delay(1000);
		}
	} else {
		ESP.restart();
	}
}
#include <jni.h>
#include <string>

#include "Soundbound.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_gradotech_soundbound_MainActivity_getServerPort(
	JNIEnv *env,
	jobject /* this */)
{
	return SB_SERVER_PORT;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_gradotech_soundbound_MainActivity_getStreamPort(
	JNIEnv *env,
	jobject /* this */)
{
	return SB_STREAM_PORT;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_gradotech_soundbound_MainActivity_getQDataPacketSize(
	JNIEnv *env,
	jobject /* this */)
{
	return sizeof(sb_qdata_packet);
}

extern "C" JNIEXPORT jchar JNICALL
Java_com_gradotech_soundbound_MainActivity_getNextSpkId(
	JNIEnv *env,
	jobject /* this */,
	jbyteArray packet,
	jint index)
{
	jbyte *buff = env->GetByteArrayElements(packet, NULL);
	sb_qdata_packet *_packet = (sb_qdata_packet *)buff;
	jchar id;

	if (!_packet)
		return 0;

	if (index < 0 || index >= SB_MAX_SPEAKERS)
		return 0;

	id = _packet->spks[index];

	env->ReleaseByteArrayElements(packet, buff, 0);

	return id;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_gradotech_soundbound_MainActivity_parseDeviceName(
	JNIEnv *env,
	jobject /* this */,
	jbyteArray packet)
{
	jbyte *buff = env->GetByteArrayElements(packet, NULL);
	sb_qdata_packet *_packet = (sb_qdata_packet *)buff;
	jstring device;

	if (!_packet)
		return 0;

	device = env->NewStringUTF(_packet->device);

	env->ReleaseByteArrayElements(packet, buff, 0);

	return device;
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_gradotech_soundbound_MainActivity_parseHWVersion(
	JNIEnv *env,
	jobject /* this */,
	jbyteArray packet)
{
	jbyte *buff = env->GetByteArrayElements(packet, NULL);
	sb_qdata_packet *_packet = (sb_qdata_packet *)buff;
	jint values[2];
	jintArray versions;

	if (!_packet)
		return 0;

	values[0] = _packet->major;
	values[1] = _packet->minor;

	versions = env->NewIntArray(2);
	env->SetIntArrayRegion(versions, 0, 2, values);

	env->ReleaseByteArrayElements(packet, buff, 0);

	return versions;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_gradotech_soundbound_MainActivity_getStartCmd(
	JNIEnv *env,
	jobject /* this */)
{
	jbyte cmd[] = { SB_START };
	jbyteArray buff;

	buff = env->NewByteArray(1);
	env->SetByteArrayRegion(buff, 0, 1, cmd);

	return buff;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_gradotech_soundbound_MainActivity_getStopCmd(
	JNIEnv *env,
	jobject /* this */)
{
	jbyte cmd[] = { SB_STOP };
	jbyteArray buff;

	buff = env->NewByteArray(1);
	env->SetByteArrayRegion(buff, 0, 1, cmd);

	return buff;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_gradotech_soundbound_MainActivity_getVolumeCmd(
	JNIEnv *env,
	jobject /* this */,
	jbyte id,
	jbyte volume)
{
	const jint cmd_size = sizeof(sb_volume_packet) + 1;
	jbyte cmd[cmd_size] = { SB_SET_VOLUME };
	sb_volume_packet *packet = (sb_volume_packet *)&cmd[1];
	jbyteArray buff;

	packet->id	= id;
	packet->volume	= volume;

	buff = env->NewByteArray(cmd_size);
	env->SetByteArrayRegion(buff, 0, cmd_size, cmd);

	return buff;
}

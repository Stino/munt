#ifndef WIN_MM_AUDIO_DRIVER_H
#define WIN_MM_AUDIO_DRIVER_H

#include <QtCore>
#include <windows.h>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../MasterClock.h"

class Master;
class QSynth;
class WinMMAudioDriver;
class WinMMAudioDevice;

class WinMMAudioStream : public AudioStream {
private:
	unsigned int minSamplesToRender;
	unsigned int bufferSize;
	MasterClockNanos latency;
	QSynth *synth;
	const unsigned int sampleRate;
	HWAVEOUT hWaveOut;
	WAVEHDR	 WaveHdr;
	MT32Emu::Bit16s *buffer;
	bool volatile pendingClose;

	static void processingThread(void *);

public:
	WinMMAudioStream(const WinMMAudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~WinMMAudioStream();
	bool start(int deviceIndex);
	void close();
};

class WinMMAudioDevice : public AudioDevice {
friend class WinMMAudioDriver;
private:
	UINT deviceIndex;
	WinMMAudioDevice(const WinMMAudioDriver * const driver, int useDeviceIndex, QString useDeviceName);
public:
	WinMMAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class WinMMAudioDriver : public AudioDriver {
private:
	void validateAudioSettings();
public:
	WinMMAudioDriver(Master *useMaster);
	~WinMMAudioDriver();
	QList<AudioDevice *> getDeviceList() const;
};

#endif

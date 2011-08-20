/*
 *  mt32emu_osx_au.h
 *  mt32emu_osx_au
 *
 *  Created by Stino former ByteGeiZ on 7/27/11
 *
 *  Copyright 2011 OpenSource. GNU GPLv3.
 *
 */

// activate here that AU with MT32 is remapped at startup to General MIDI
// #define USE_MT32_AS_GMIDI 1

// activate here higher debug output
// #define DEBUG_PRINT 1 

#include "mt32emu.h"
#include "Structures.h"

#include "mt32emu_osx_auVersion.h"
#include "AUInstrumentBase.h"

#ifdef USE_MT32_AS_GMIDI
#include <AudioToolbox/AudioToolbox.h>
#endif

//this value scales the volume output 
//use a small value here, if not it is much too loud
#define OUTSCALE 0.0001f;

static void vdebug(void *userData, const char *fmt, va_list list) {
	char message[1024];
	
	vsprintf(message, fmt, list);
#if DEBUG_PRINT
	printf("MT32: %s\n",message);
#endif
}

static int report(void *userData, MT32Emu::ReportType type, const void *reportData) {
	switch(type) {
		case MT32Emu::ReportType_errorControlROM:
#if DEBUG_PRINT
			printf("MT32: Couldn't find control files\n");
#endif
			break;
		case MT32Emu::ReportType_errorPCMROM:
#if DEBUG_PRINT
			printf("MT32: Couldn't open MT32_PCM.ROM file\n");
#endif
			break;
		default:
#if DEBUG_PRINT
			printf("MT32: Report %d\n",type);
#endif						
			break;
	}
	return 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


enum {
	kGlobalVolumeParam = 0,
	kChannelNumberParameter = 1,
	//Add your parameters here...
	kNumberOfParameters=2
};

static const CFStringRef kGlobalVolumeName = CFSTR("global volume");
static const CFStringRef kChannelNumberName = CFSTR("used channels(0=mono,1=stereo,2=surround)");
const UInt32 kEventQueueSize = 1024;

typedef MT32Emu::Bit32u Bit32u;
typedef MT32Emu::Bit16s Bit16s;

struct MidiMsg
{
	Bit32u msg;
	UInt32 inStartFrame;
	
	void Free()
	{
		// nothing to free at the moment
	}
};

typedef LockFreeFIFOWithFree<MidiMsg> MidiMsgQueue;

class mt32emu_osx_au : public AUInstrumentBase
{
public:
	mt32emu_osx_au(ComponentInstance inComponentInstance);
	virtual						~mt32emu_osx_au();
	ComponentResult				RestoreState(	CFPropertyListRef	plist);
	
	virtual ComponentResult		Render(AudioUnitRenderActionFlags &	ioActionFlags,
									   const AudioTimeStamp &			inTimeStamp,
									   UInt32							inNumberFrames);
	
	
	virtual OSStatus			Initialize();
	virtual OSStatus			Version() { return kmt32emu_osx_auVersion; }
	
	virtual OSStatus			GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo);
	
	virtual OSStatus			HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inStartFrame);
	virtual OSStatus			HandleSysEx(const UInt8 *inData, UInt32	inLength);

private:
	OSStatus			SetOutChannelsFromView();

	inline ComponentResult	RenderAllChan(  UInt32							frameOffset,
											UInt32							inNumberFrames,
											AudioBufferList&				outOutputData,
											AudioStreamBasicDescription& outStreamFormat);
	
	inline ComponentResult		Render2Chan(UInt32							frameOffset,
									        UInt32							inNumberFrames,
											AudioBufferList&				outOutputData,
											AudioStreamBasicDescription& outStreamFormat);
	
	inline ComponentResult		Render6Chan(UInt32							frameOffset,
									        UInt32							inNumberFrames,
											AudioBufferList&				outOutputData,
											AudioStreamBasicDescription& outStreamFormat);
	
	void LoadSysExFromMIDIFile(const char *filename);
	
	AudioTimeStamp lastTimeStamp;
	pthread_mutex_t mAUMutex;
	MT32Emu::Synth *_synth;
	bool isOpen;
	int mt32samplerate;
	MidiMsgQueue mMidiQueue;
};

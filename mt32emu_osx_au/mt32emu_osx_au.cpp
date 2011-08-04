/*
 *	File:		mt32emu_osx_au.cpp
 *	
 *	Version:	1.0
 * 
 *  Created by Stino former ByteGeiZ on 7/27/11
 *
 *	Copyright:  Copyright © 2011 OpenSource, GNU GPLv3
 * 
 *
 */
/*=============================================================================
 mt32emu_osx_au.cpp
 
 =============================================================================*/


#include "mt32emu_osx_au.h"
#include "AUInstrumentBase.h"


#include <unistd.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>


COMPONENT_ENTRY(mt32emu_osx_au)

#pragma mark mt32emu_osx_au Methods

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::mt32emu_osx_au
//
// This synth has One MIDI input, One output
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mt32emu_osx_au::mt32emu_osx_au(ComponentInstance inComponentInstance)
: AUInstrumentBase(inComponentInstance, 0, 1),
_synth(0),
isOpen(false),
mMidiQueue(kEventQueueSize)

{
	MT32Emu::SynthProperties tmpProp;
	memset(&tmpProp, 0, sizeof(tmpProp));
	
	CreateElements();	
	
	_synth = new MT32Emu::Synth();
	if (!_synth) COMPONENT_THROW(-1);
	
	// force sample rate to 32000 Hz scince the original MT32 use this
	mt32samplerate = 32000;
	
#ifdef DEBUG_PRINT
	printf("MT32: mt32samplerate %d\n",mt32samplerate);
#endif	
	
	tmpProp.sampleRate = mt32samplerate;
	tmpProp.useDefaultReverb = true;
	tmpProp.useReverb = true;
	tmpProp.reverbType = 1;
	tmpProp.reverbTime = 5;
	tmpProp.reverbLevel = 3;
	tmpProp.printDebug = &vdebug;
	tmpProp.report = &report;
	
	// lett MUNT also search for MT32 ROM's in the common path
    // when I use baseDir than my bundle path is not longer working, so I comment this out
	//tmpProp.baseDir = "/usr/share/mt32-rom-data";
	
	// change dir to Resurce Path of our Component Bundle for ROMS before init MUNT
	CFBundleRef muntBundle = CFBundleGetBundleWithIdentifier( CFSTR("com.mt32emu.mt32emu_osx_au"));
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(muntBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		COMPONENT_THROW(-2);
	}
	CFRelease(resourcesURL);
	chdir(path);
	
	// try to init MUNT, if this failes most times the MT32 ROM's are not found
	// we expect them by default in the Resources Folder of out AU component bundel
	if (_synth->open(tmpProp)==0) COMPONENT_THROW(-3);
	
	isOpen = true;
	
	Globals()->UseIndexedParameters (kNumberOfParameters); // we're only defining one param
	Globals()->SetParameter (kGlobalVolumeParam, 0.5);
	Globals()->SetParameter (kChannelNumberParameter, 1); // 1 is equal to stereo, that is the default at moment
	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::~mt32emu_osx_au
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
mt32emu_osx_au::~mt32emu_osx_au()
{
	if (!isOpen) return;
	_synth->close();
	delete _synth;
	_synth = NULL;
	isOpen=false;	
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::SetOutChannelsFromView
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus mt32emu_osx_au::SetOutChannelsFromView()
{	
	OSStatus err = noErr;
	// read actally set stream format
	AudioStreamBasicDescription outStreamFormat = GetOutput(0)->GetStreamFormat();
	int oldChan = (int)outStreamFormat.mChannelsPerFrame;
	int newChan = oldChan;
	
#ifdef DEBUG_PRINT		
	printf("MT32: mChannelsPerFrame before %d\n",oldChan);
#endif	
	
	switch ((int)Globals()->GetParameter(kChannelNumberParameter)){
		case 0:
			newChan = 1;
			break;
		case 1:
			newChan = 2;
			break;
		case 2:
			newChan = 6;
			break;
		default:
			COMPONENT_THROW(-4);
			break;
	}
	
	// force 6 channels at start for debugging
	//newChan = 6;
	
	if (oldChan != newChan)
	{
		// change channel number
		outStreamFormat.mChannelsPerFrame = newChan;
		// this is needed for convert of stream description since ChangeStreamFormat() need this format
		CAStreamBasicDescription prefForm;
		CAStreamBasicDescription *newForm = new CAStreamBasicDescription(outStreamFormat);
		// new setup of output channels with 
		err = AUBase::ChangeStreamFormat(kAudioUnitScope_Global, 0, prefForm, *newForm);
		// delate convert class
		delete newForm;
	}

#ifdef DEBUG_PRINT		
	printf("MT32: mChannelsPerFrame after %d\n",(int)GetOutput(0)->GetStreamFormat().mChannelsPerFrame);
#endif
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Initialize
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus mt32emu_osx_au::Initialize()
{	
	AUInstrumentBase::Initialize();	
			
	// setup initial output channels
	return SetOutChannelsFromView();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::GetParameterInfo
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OSStatus mt32emu_osx_au::GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID, AudioUnitParameterInfo &outParameterInfo)
{	
	if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
	switch(inParameterID)
	{
		case kGlobalVolumeParam:
			outParameterInfo.flags = SetAudioUnitParameterDisplayType (0, kAudioUnitParameterFlag_DisplaySquareRoot);
			outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
			outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
			
			AUBase::FillInParameterName (outParameterInfo, kGlobalVolumeName, false);
			outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			outParameterInfo.minValue = 0;
			outParameterInfo.maxValue = 1;
			outParameterInfo.defaultValue = 0.5;
			
			return noErr;
		
		case kChannelNumberParameter:
			outParameterInfo.flags = SetAudioUnitParameterDisplayType (0, kAudioUnitParameterFlag_ValuesHaveStrings);
			outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
			outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
			
			AUBase::FillInParameterName (outParameterInfo, kChannelNumberName, false);
			outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
			outParameterInfo.minValue = 0;
			outParameterInfo.maxValue = 2;
			outParameterInfo.defaultValue = 1;
			return noErr;
			
		default:
			return kAudioUnitErr_InvalidParameter;
	}
	

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::RestoreState
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ComponentResult mt32emu_osx_au::RestoreState(CFPropertyListRef plist)
{
	return AUInstrumentBase::RestoreState(plist);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Render
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The Sound output, that is created by MT32 synth
ComponentResult	mt32emu_osx_au::Render(AudioUnitRenderActionFlags &	ioActionFlags,
								const AudioTimeStamp &			inTimeStamp,
								UInt32							inNumberFrames)
{	
	ComponentResult err = noErr;
	MidiMsg *event;	
	UInt32 fromFrame = 0;
	UInt32 diff = 0;
	AudioBufferList& outOutputData = GetOutput(0)->GetBufferList();	
#ifdef DEBUG_PRINT	
	printf("MT32: Render start\n");
#endif	
	
	//workaround: we send now all MIDI messages from puffer to MUNT
	// later it is needed to look on the inStartFrame to decide when it rendered
	while ((event = mMidiQueue.ReadItem()) != NULL)
	{
		if (fromFrame == event->inStartFrame)		
		{
			// play midi notes as long as they are at fromFrame
#ifdef DEBUG_PRINT				
			printf("MT32: play MIDI at: %ld\n",(long int)event->inStartFrame);
#endif			
			pthread_mutex_lock(&mAUMutex);
			if (_synth) _synth->playMsg(event->msg);
			pthread_mutex_unlock(&mAUMutex);
		}
		else
		{
			// check correct sort
			if (event->inStartFrame < fromFrame) COMPONENT_THROW(-5);		
			//render all played midi notes till frame before actual frame
			diff = event->inStartFrame - fromFrame;
			pthread_mutex_lock(&mAUMutex);
			err = RenderAllChan(fromFrame,diff,outOutputData);
			pthread_mutex_unlock(&mAUMutex);
#ifdef DEBUG_PRINT				
			printf("MT32: render %ld Frames from: %ld \n",(long int)diff,(long int)fromFrame);
#endif			
			
			// play actual midi note
#ifdef DEBUG_PRINT				
			printf("MT32: play MIDI at: %ld\n",(long int)event->inStartFrame);
#endif			
			pthread_mutex_lock(&mAUMutex);
			if (_synth) _synth->playMsg(event->msg);
			pthread_mutex_unlock(&mAUMutex);
			// set fromFrame to actual frame
			fromFrame = event->inStartFrame;
		}
		
		mMidiQueue.AdvanceReadPtr();
	}	
	// render rest
	diff = inNumberFrames - fromFrame;
	pthread_mutex_lock(&mAUMutex);
	err = RenderAllChan(fromFrame,diff,outOutputData);
	pthread_mutex_unlock(&mAUMutex);
#ifdef DEBUG_PRINT		
	printf("MT32: render rest %ld Frames from: %ld \n",(long int)diff,(long int)fromFrame);
#endif	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::RenderAllChan
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The Sound output, that is created by MT32 synth
inline ComponentResult	mt32emu_osx_au::RenderAllChan(UInt32			frameOffset,
									   UInt32							inNumberFrames,
									   AudioBufferList&				    outOutputData)
{	
	ComponentResult err = noErr;

	
	// Debug inTimeStamp
#ifdef DEBUG_PRINT		
	printf("MT32: inTimeStamp.mHostTime: %lld\n",inTimeStamp.mHostTime);
	printf("MT32: inTimeStamp.mHostTime Delta: %lld\n",(inTimeStamp.mHostTime-lastTimeStamp.mHostTime));
	lastTimeStamp = inTimeStamp;
#endif	
	
	
#ifdef DEBUG_PRINT	
	printf("MT32: numChans: %d\n",(int)outOutputData.mNumberBuffers);
#endif
	
	switch (outOutputData.mNumberBuffers)
	{
		case 1:		
		case 2:			
			// mono and stereo are handled by 2 channel render of MUNT
			err = Render2Chan(frameOffset,inNumberFrames, outOutputData);
			break;			
		case 6:			
			// 6 channel render of MUNT
			err = Render6Chan(frameOffset,inNumberFrames, outOutputData);
			break;			
		default:
			return kAudioDeviceUnsupportedFormatError;
	}
	
	return err;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Render2Chan
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline ComponentResult		mt32emu_osx_au::Render2Chan(UInt32					frameOffset,
										UInt32							inNumberFrames,
										AudioBufferList&				outOutputData)
{
	UInt32 len;
	float *l1, *r1 = 0;
	float scaleVol = Globals()->GetParameter(kGlobalVolumeParam) * OUTSCALE;
	// buffer for 2 channel variant
	Bit16s *tempbuff = new Bit16s[inNumberFrames << 1]; // "<< 1" is like "* 2" just faster
	Bit16s *t = tempbuff;
	
	// init pointers for moving in array
	l1 = (float*)outOutputData.mBuffers[0].mData;
	l1 += frameOffset;
	if (outOutputData.mNumberBuffers == 2) r1 = (float*)outOutputData.mBuffers[1].mData;
	if (outOutputData.mNumberBuffers == 2) r1 += frameOffset;
	
	// call 2 channel render of MUNT (know it is intenal an 6 channel render)	
	_synth->render((Bit16s *)tempbuff, inNumberFrames);
	
	// convert outbuffer of MUNT to outpuffer of AU
	len = inNumberFrames;
	while(len>0)
	{
		len--;
		*l1 = (*t) * scaleVol;
		l1++;
		t++;
		if (outOutputData.mNumberBuffers == 2)
		{
			*r1 = (*t) * scaleVol;
			r1++;
		}
		t++;
	}		
	
	// free buffer
	delete tempbuff;
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::Render6Chan
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
inline ComponentResult	mt32emu_osx_au::Render6Chan(UInt32						frameOffset,
										UInt32							inNumberFrames,
										AudioBufferList&				outOutputData)
{	
	UInt32 len;
	float *l1, *r1, *l2, *r2, *l3, *r3 = 0;
	float scaleVol = Globals()->GetParameter(kGlobalVolumeParam) * OUTSCALE;
	
	// buffers for 6 channel variant
	Bit16s *tempLeft1 = new Bit16s[inNumberFrames];
	Bit16s *tl1 = tempLeft1;
	Bit16s *tempRight1 = new Bit16s[inNumberFrames];
	Bit16s *tr1 = tempRight1;
	Bit16s *tempLeft2 = new Bit16s[inNumberFrames];
	Bit16s *tl2 = tempLeft2;
	Bit16s *tempRight2 = new Bit16s[inNumberFrames];
	Bit16s *tr2 = tempRight2;
	Bit16s *tempLeft3 = new Bit16s[inNumberFrames];
	Bit16s *tl3 = tempLeft3;
	Bit16s *tempRight3 = new Bit16s[inNumberFrames];
	Bit16s *tr3 = tempRight3;

	// init pointers for moving in array
	l1 = (float*)outOutputData.mBuffers[0].mData;
	r1 = (float*)outOutputData.mBuffers[1].mData;
	l2 = (float*)outOutputData.mBuffers[2].mData;
	r2 = (float*)outOutputData.mBuffers[3].mData;
	l3 = (float*)outOutputData.mBuffers[4].mData;
	r3 = (float*)outOutputData.mBuffers[5].mData;
	l1 += frameOffset;
	r1 += frameOffset;
	l2 += frameOffset;
	r2 += frameOffset;
	l3 += frameOffset;
	r3 += frameOffset;
	
	// call 6 channel render of MUNT
	_synth->renderStreams((Bit16s *)tempLeft1, (Bit16s *)tempRight1, (Bit16s *)tempLeft2, (Bit16s *)tempRight2,(Bit16s *)tempLeft3, (Bit16s *)tempRight3, inNumberFrames);
	
	// convert outbuffer of MUNT to outpuffer of AU// convert outbuffer of MUNT to outpuffer of AU
	len = inNumberFrames;
	while(len>0)
	{
		len--;
		*l1 = (*tl1) * scaleVol;
		*r1 = (*tr1) * scaleVol;
		*l2 = (*tl2) * scaleVol;
		*r2 = (*tr2) * scaleVol;
		*l3 = (*tl3) * scaleVol;
		*r3 = (*tr3) * scaleVol;
		l1++;
		r1++;
		l2++;
		r2++;
		l3++;
		r3++;
		tl1++;
		tr1++;
		tl2++;
		tr2++;
		tl3++;
		tr3++;
	}
	
	// free buffers
	delete tempLeft1;
	delete tempRight1;
	delete tempLeft2;
	delete tempRight2;
	delete tempLeft3;
	delete tempRight3;
	
	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::HandleMidiEvent
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The MIDI input, that we forward to MT32 synth
OSStatus 	mt32emu_osx_au::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 inStartFrame)
{
	unsigned long msg = 0;
	
	msg = channel & 0x0000000F;
	msg = msg + ((status     ) & 0x000000F0); // normaly status needs to shift 4 bits, but status is already shifted
	msg = msg + ((data1 <<  8) & 0x0000FF00);
	msg = msg + ((data2 << 16) & 0x00FF0000);
#ifdef DEBUG_PRINT
	printf("MT32: Midi-Data: %8.8X\n",(unsigned int)msg);
#endif	
		
	MidiMsg *event = mMidiQueue.WriteItem();
	if (!event) return -1;
	event->msg = msg;
	event->inStartFrame = inStartFrame;
	mMidiQueue.AdvanceWritePtr();

	return noErr;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	mt32emu_osx_au::HandleSysEx
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The SysEx input, that we forward to MT32 synth
OSStatus		mt32emu_osx_au::HandleSysEx(		const UInt8 *	inData,	UInt32			inLength )
{ 
	pthread_mutex_lock(&mAUMutex);
#ifdef DEBUG_PRINT
	printf("MT32: SysEx-Data: ");
	for (unsigned long i = 0;i<inLength;i++) printf("%2.2X ",inData[i]);
	printf("\n");
#endif	
	if (inData[0]== 0xf0)
	{
		_synth->playSysex(inData,inLength);
	}
	else
	{
		if (inData[inLength-1]== 0xf7) {
			// only header removed, but footer still exsiting, so we cut off footer as well
			_synth->playSysexWithoutFraming(inData,inLength-1); 
		}
		else
		{
			// allready removed header and footer, so we pass it as it is
			_synth->playSysexWithoutFraming(inData,inLength); 
		}
	}
	pthread_mutex_unlock(&mAUMutex);
	return noErr; 
}

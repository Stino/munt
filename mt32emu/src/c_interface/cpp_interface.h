/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2015 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_CPP_INTERFACE_H
#define MT32EMU_CPP_INTERFACE_H

#include <cstdarg>

#include "../globals.h"
#include "c_types.h"

#if defined _WIN32 || defined __CYGWIN__
#define MT32EMU_METHOD __cdecl
#else
#define MT32EMU_METHOD
#endif

namespace MT32Emu {

/**
 * The interfaces below correspond to those defined in c_types.h provided for convenience when using C++.
 * Although, the approach used assumes an arbitrary memory layout of internal class data, and in particular, virtual function tables,
 * that most of C++ compiler tend to implement, and it may successfully work, it is necessary to note that the C++ standard does not
 * provide any detail in this area and leaves it up to the implementation.
 * Therefore, portability is not guaranteed.
 * See c_types.h and c_interface.h for description of corresponding interfaces.
 */

/** Basic interface definition */
struct Interface {
	virtual mt32emu_interface_version MT32EMU_METHOD getVersionID() = 0;

protected:
	~Interface() {}
};

/** Interface for handling reported events (initial version) */
struct ReportHandler : public Interface {
	virtual void MT32EMU_METHOD printDebug(const char *fmt, va_list list) = 0;
	virtual void MT32EMU_METHOD onErrorControlROM() = 0;
	virtual void MT32EMU_METHOD onErrorPCMROM() = 0;
	virtual void MT32EMU_METHOD showLCDMessage(const char *message) = 0;
	virtual void MT32EMU_METHOD onMIDIMessagePlayed() = 0;
	virtual void MT32EMU_METHOD onMIDIQueueOverflow() = 0;
	virtual void MT32EMU_METHOD onMIDISystemRealtime(mt32emu_bit8u systemRealtime) = 0;
	virtual void MT32EMU_METHOD onDeviceReset() = 0;
	virtual void MT32EMU_METHOD onDeviceReconfig() = 0;
	virtual void MT32EMU_METHOD onNewReverbMode(mt32emu_bit8u mode) = 0;
	virtual void MT32EMU_METHOD onNewReverbTime(mt32emu_bit8u time) = 0;
	virtual void MT32EMU_METHOD onNewReverbLevel(mt32emu_bit8u level) = 0;
	virtual void MT32EMU_METHOD onPolyStateChanged(int partNum) = 0;
	virtual void MT32EMU_METHOD onProgramChanged(int partNum, const char *soundGroupName, const char *patchName) = 0;

protected:
	~ReportHandler() {}
};

/** Interface for receiving MIDI messages generated by MIDI stream parser (initial version) */
struct MidiReceiver : public Interface {
	virtual void MT32EMU_METHOD handleShortMessage(const mt32emu_bit32u message) = 0;
	virtual void MT32EMU_METHOD handleSysex(const mt32emu_bit8u stream[], const mt32emu_bit32u length) = 0;
	virtual void MT32EMU_METHOD handleSystemRealtimeMessage(const mt32emu_bit8u realtime) = 0;

protected:
	~MidiReceiver() {}
};

/**
 * Basic interface that defines all the library services (initial version).
 * The members closely resemble C functions declared in c_interface.h, and the intention is to provide for easier
 * access when the library is dynamically loaded in run-time, e.g. as a plugin. This way the client only needs
 * to bind to mt32emu_create_synth() factory function instead of binding to each function it needs to use.
 * See c_interface.h for parameter description.
 */
struct Synth : public Interface {
	virtual void MT32EMU_METHOD freeSynth() = 0;
	virtual mt32emu_return_code MT32EMU_METHOD addROMData(const mt32emu_bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD addROMFile(const char *filename) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD openSynth(const unsigned int *partial_count, const mt32emu_analog_output_mode *analog_output_mode) = 0;
	virtual void MT32EMU_METHOD closeSynth() = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isOpen() = 0;
	virtual unsigned int MT32EMU_METHOD getStereoOutputSamplerate(mt32emu_const_context context, const mt32emu_analog_output_mode analog_output_mode) = 0;
	virtual unsigned int MT32EMU_METHOD getActualStereoOutputSamplerate() = 0;
	virtual void MT32EMU_METHOD flushMIDIQueue() = 0;
	virtual mt32emu_bit32u MT32EMU_METHOD setMIDIEventQueueSize(const mt32emu_bit32u queue_size) = 0;
	virtual mt32emu_midi_receiver_version MT32EMU_METHOD setMIDIReceiver(const MidiReceiver *midi_receiver) = 0;

	virtual void MT32EMU_METHOD parseStream(const mt32emu_bit8u *stream, mt32emu_bit32u length) = 0;
	virtual void MT32EMU_METHOD parseStream_At(const mt32emu_bit8u *stream, mt32emu_bit32u length, mt32emu_bit32u timestamp) = 0;
	virtual void MT32EMU_METHOD playShortMessage(mt32emu_bit32u message) = 0;
	virtual void MT32EMU_METHOD playShortMessageAt(mt32emu_bit32u message, mt32emu_bit32u timestamp) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD playMsg(mt32emu_bit32u msg) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD playSysex(const mt32emu_bit8u *sysex, mt32emu_bit32u len) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD playMsgAt(mt32emu_bit32u msg, mt32emu_bit32u timestamp) = 0;
	virtual mt32emu_return_code MT32EMU_METHOD playSysexAt(const mt32emu_bit8u *sysex, mt32emu_bit32u len, mt32emu_bit32u timestamp) = 0;

	virtual void MT32EMU_METHOD playMsgNow(mt32emu_bit32u msg) = 0;
	virtual void MT32EMU_METHOD playMsgOnPart(unsigned char part, unsigned char code, unsigned char note, unsigned char velocity) = 0;
	virtual void MT32EMU_METHOD playSysexNow(const mt32emu_bit8u *sysex, mt32emu_bit32u len) = 0;
	virtual void MT32EMU_METHOD writeSysex(unsigned char channel, const mt32emu_bit8u *sysex, mt32emu_bit32u len) = 0;

	virtual void MT32EMU_METHOD setReverbEnabled(const mt32emu_boolean reverb_enabled) = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isReverbEnabled() = 0;
	virtual void MT32EMU_METHOD setReverbOverridden(const mt32emu_boolean reverbOverridden) = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isReverbOverridden() = 0;
	virtual void MT32EMU_METHOD setReverbCompatibilityMode(const mt32emu_boolean mt32_compatible_mode) = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isMT32ReverbCompatibilityMode() = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isDefaultReverbMT32Compatible() = 0;

	virtual void MT32EMU_METHOD setDACInputMode(const mt32emu_dac_input_mode mode) = 0;
	virtual mt32emu_dac_input_mode MT32EMU_METHOD getDACInputMode() = 0;

	virtual void MT32EMU_METHOD setMIDIDelayMode(const mt32emu_midi_delay_mode mode) = 0;
	virtual mt32emu_midi_delay_mode MT32EMU_METHOD getMIDIDelayMode() = 0;

	virtual void MT32EMU_METHOD setOutputGain(float gain) = 0;
	virtual float MT32EMU_METHOD getOutputGain() = 0;
	virtual void MT32EMU_METHOD setReverbOutputGain(float gain) = 0;
	virtual float MT32EMU_METHOD getReverbOutputGain() = 0;

	virtual void MT32EMU_METHOD setReversedStereoEnabled(const mt32emu_boolean enabled) = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isReversedStereoEnabled() = 0;

	virtual void MT32EMU_METHOD render(mt32emu_sample *stream, mt32emu_bit32u len) = 0;
	virtual void MT32EMU_METHOD renderStreams(const mt32emu_dac_output_streams *streams, mt32emu_bit32u len) = 0;

	virtual mt32emu_boolean MT32EMU_METHOD hasActivePartials() = 0;
	virtual mt32emu_boolean MT32EMU_METHOD isActive() = 0;
	virtual unsigned int MT32EMU_METHOD getPartialCount() = 0;
	virtual mt32emu_bit32u MT32EMU_METHOD getPartStates() = 0;
	virtual void MT32EMU_METHOD getPartialStates(mt32emu_bit8u *partial_states) = 0;
	virtual unsigned int MT32EMU_METHOD getPlayingNotes(unsigned int part_number, mt32emu_bit8u *keys, mt32emu_bit8u *velocities) = 0;
	virtual const char * MT32EMU_METHOD getPatchName(unsigned int part_number) = 0;
	virtual void MT32EMU_METHOD readMemory(mt32emu_bit32u addr, mt32emu_bit32u len, mt32emu_bit8u *data) = 0;
	virtual mt32emu_report_handler_version MT32EMU_METHOD getSupportedReportHandlerVersionID() = 0;

private:
	~Synth() {}
};

} // namespace MT32Emu

#undef MT32EMU_EXPORT
#if (MT32EMU_EXPORTS_TYPE == 2)
#define MT32EMU_EXPORT MT32EMU_EXPORT_ATTRIBUTE
#else
#define MT32EMU_EXPORT
#endif

extern "C" {

// Creates an instance of Synth interface implementation and installs custom report handler if non-NULL. */
MT32EMU_EXPORT MT32Emu::Synth *mt32emu_create_synth(const MT32Emu::ReportHandler *report_handler);

}

#undef MT32EMU_METHOD

#endif /* #ifndef MT32EMU_CPP_INTERFACE_H */

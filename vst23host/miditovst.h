//-----------------------------------------------------------------------------
// Flags       : clang-format auto
// Project     : VST SDK
//
// Category    : AudioHost
// Filename    : public.sdk/samples/vst-hosting/audiohost/source/media/miditovst.h
// Created by  : Steinberg 09.2016
// Description : Audio Host Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "vst/hosting/optional.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include <functional>
#include <limits>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
    
const uint8_t kNoteOff = 0x80; ///< note, off velocity
const uint8_t kNoteOn = 0x90; ///< note, on velocity
const uint8_t kPolyPressure = 0xA0; ///< note, pressure
const uint8_t kController = 0xB0; ///< controller, value
const uint8_t kProgramChangeStatus = 0xC0; ///< program change
const uint8_t kAfterTouchStatus = 0xD0; ///< channel pressure
const uint8_t kPitchBendStatus = 0xE0; ///< lsb, msb
static const uint32 kDataMask = 0x7F;
    
const float kMidiScaler = 1.f / 127.f;
    
using MidiData = uint8_t;
    
float toNormalized (const MidiData& data);
    
using OptionalEvent = VST3::Optional<Event>;
    
using ParameterChange = std::pair<ParamID, ParamValue>;
using OptionParamChange = VST3::Optional<ParameterChange>;
    
OptionalEvent midiToEvent (MidiData status, MidiData channel, MidiData midiData0,
                           MidiData midiData1);
OptionalEvent midiToDataEvent (MidiData status, uint32 size, const uint8* bytes);
   
//------------------------------------------------------------------------
using ToParameterIdFunc = std::function<ParamID (int32, MidiData)>;
OptionParamChange midiToParameter (MidiData status, MidiData channel, MidiData midiData1,
                                   MidiData midiData2, const ToParameterIdFunc& toParamID);

} // Vst
} // Steinberg


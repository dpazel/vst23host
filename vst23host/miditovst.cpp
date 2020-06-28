//
//  miditovst.cpp
//  vstplayer
//
//  Created by Donald Pazel on 2/20/19.
//  Copyright © 2019 Pazel. All rights reserved.
//

#include <stdio.h>
#include "miditovst.h"
#include <iostream>

#include "public.sdk/source/vst/vsteventshelper.h"


//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
    
float toNormalized (const MidiData& data) {
  return (float)data * kMidiScaler;
}
  
uint32 size;      ///< size in bytes of the data block bytes
const uint8* bytes;
    
OptionalEvent midiToDataEvent (MidiData status, uint32 size, const uint8* bytes) {
  Event new_event = {};
  new_event.type = Event::kDataEvent;
  new_event.data.type = DataEvent::kMidiSysEx;
  new_event.data.size = size;
  new_event.data.bytes = (uint8 *)calloc(size, 1);
  std::memcpy((void *)new_event.data.bytes, bytes, size);
  
  return std::move (new_event);
}

OptionalEvent midiToEvent (MidiData status, MidiData channel, MidiData midiData0,
                           MidiData midiData1)
{
  Event new_event = {};
  if (status == kNoteOn || status == kNoteOff)
  {
    if (status == kNoteOff) // note off
    {
      new_event.noteOff.noteId = -1;
      new_event.type = Event::kNoteOffEvent;
      new_event.noteOff.channel = channel;
      new_event.noteOff.pitch = midiData0;
      new_event.noteOff.velocity = toNormalized (midiData1);
      return std::move (new_event);
    }
    else if (status == kNoteOn) // note on
    {
      new_event.noteOn.noteId = -1;
      new_event.type = Event::kNoteOnEvent;
      new_event.noteOn.channel = channel;
      new_event.noteOn.pitch = midiData0;
      new_event.noteOn.velocity = toNormalized (midiData1);
      return std::move (new_event);
    }
  }
  //--- -----------------------------
  else if (status == kPolyPressure)
  {
    new_event.type = Vst::Event::kPolyPressureEvent;
    new_event.polyPressure.channel = channel;
    new_event.polyPressure.pitch = midiData0;
    new_event.polyPressure.pressure = toNormalized (midiData1);
    return std::move (new_event);
  } else if (status == kController) {
    new_event.type = Vst::Event::kLegacyMIDICCOutEvent;
    new_event.midiCCOut.channel = channel;
    new_event.midiCCOut.controlNumber = midiData0;
    new_event.midiCCOut.value = midiData1;
    
    std::cout << "   controller " << (int)midiData0 << " value=" << (int)midiData1 << std::endl;
    return std::move (new_event);
  } else if (status == kProgramChangeStatus) {
    new_event.noteOn.noteId = -1;
    new_event.type = Event::kNoteOnEvent;
    new_event.noteOn.channel = channel;
    new_event.noteOn.pitch = midiData0;
    new_event.noteOn.velocity = toNormalized (midiData1);
    return std::move (new_event);
  }
  
  return {};
}

//------------------------------------------------------------------------
using ToParameterIdFunc = std::function<ParamID (int32, MidiData)>;
OptionParamChange midiToParameter (MidiData status, MidiData channel, MidiData midiData1,
                                   MidiData midiData2, const ToParameterIdFunc& toParamID)
{
  if (!toParamID)
    return {};
  
  ParameterChange paramChange;
  if (status == kController) // controller
  {
    paramChange.first = toParamID (channel, midiData1);
    if (paramChange.first != kNoParamId)
    {
      paramChange.second = (double)midiData2 * kMidiScaler;
      return std::move (paramChange);
    }
  }
  else if (status == kPitchBendStatus)
  {
    paramChange.first = toParamID (channel, Vst::kPitchBend);
    if (paramChange.first != kNoParamId)
    {
      const double kPitchWheelScaler = 1. / (double)0x3FFF;
      
      const int32 ctrl = (midiData1 & kDataMask) | (midiData2 & kDataMask) << 7;
      paramChange.second = kPitchWheelScaler * (double)ctrl;
      return std::move (paramChange);
    };
  }
  else if (status == kAfterTouchStatus)
  {
    paramChange.first = toParamID (channel, Vst::kAfterTouch);
    if (paramChange.first != kNoParamId)
    {
      paramChange.second = (ParamValue) (midiData1 & kDataMask) * kMidiScaler;
      return std::move (paramChange);
    };
  }
  else if (status == kProgramChangeStatus)
  {
    // TODO
    paramChange.first = toParamID (channel, Vst::kCtrlProgramChange);
    if (paramChange.first != kNoParamId)
    {
      paramChange.second = (ParamValue) (midiData1 & kDataMask) * kMidiScaler;
      return std::move (paramChange);
    };
  }
  
  return {};
}
  
} // Vst
} // Steinberg


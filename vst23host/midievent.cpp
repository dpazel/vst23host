#include <stdio.h>
#include <stdlib.h>

#include "midievent.h"

MidiEvent::MidiEvent() :
  eventType(MIDI_TYPE_INVALID),
  deltaFrames(0),
  timestamp(0),
  status(0),
  data1(0),
  data2(0),
extraData(NULL) {}

MidiEvent::MidiEvent(byte code, int channel, int byte1, int byte2, long deltaFrames):
  eventType(MIDI_TYPE_REGULAR),
  status(code + channel),
  deltaFrames(deltaFrames),
  data1(byte1),
  data2(byte2),
  timestamp(0){}

#ifndef vst3eventstructures_h
#define vst3eventstructures_h

#include <stdio.h>
#include <string>
#include <array>
#include <vector>
#include <list>

namespace Steinberg {
namespace Vst {
namespace VST3Host {
      
enum
{
  kMaxMidiMappingBusses = 4,
  kMaxMidiChannels = 16
};
      
using Controllers = std::vector<int32>;
using Channels = std::array<Controllers, kMaxMidiChannels>;
using Busses = std::array<Channels, kMaxMidiMappingBusses>;
using MidiCCMapping = Busses;
      
using MidiData = uint8_t;
      
struct Buffers
{
  float** inputs;
  int32_t numInputs;
  float** outputs;
  int32_t numOutputs;
  int32_t numSamples;
};
      
struct EventStructure
{
  MidiData type;
  MidiData channel;
  MidiData data0;
  MidiData data1;
  int64_t timestamp;
};
      
struct DataEventStructure
{
  MidiData type;    ///< type of this data block (see \ref DataTypes)
  uint32 size;      ///< size in bytes of the data block bytes
  const uint8* bytes;
  int64_t timestamp;
};
      
class EventRepresentation
{
public:
  EventRepresentation(MidiData type, MidiData channel, MidiData data0, MidiData data1, int64_t timestamp): is_data_type(false)
  {
    this->std_event.type = type; this->std_event.timestamp = timestamp;
    this->std_event.channel = channel; this->std_event.data0 = data0; this->std_event.data1 = data1;
  }
        
  EventRepresentation(MidiData type, uint32 size, const uint8* bytes, int64_t timestamp): is_data_type(true) {
    this->data_event.type = type; this->data_event.timestamp = timestamp;
    this->data_event.size = size; this->data_event.bytes = bytes;
  }
        
  virtual ~EventRepresentation() {
    if (this->is_data_type) {
      if (this->data_event.bytes) {
        delete this->data_event.bytes;
      }
    }
  }
        
  int64_t getTimestamp() {
    return is_data_type ? getDataEventStructure()->timestamp : getEventStructure()->timestamp;
  }
        
  EventStructure * getEventStructure() {
    return &std_event;
  }
  
  DataEventStructure * getDataEventStructure() {
    return &data_event;
  }
        
  bool is_data_type;
  union {
    EventStructure std_event;
    DataEventStructure data_event;
  };
};
      
// E.G. Midi Master Volume Sysex Event: 0xF0 0x7F 0x7F 0x04 0x01 0x7F 0x3F 0xF7  ref. https://www.recordingblogs.com/wiki/midi-master-volume-message
      
// The MIDI master volume message has the following format.
// ref.: https://www.recordingblogs.com/wiki/midi-master-volume-message
//  0xF0 0x7F 0xid 0x04 0x01 0xmm 0xnn 0xF7
//
//  0xid is the system exclusive channel, usually the manufacturer ID,
//     but could be anywhere from 0x00 to 0x7F (from 0 to 127), where 0x7F prompts all devices to respond.
// 0x04 and 0x01 are the sub IDs of the universal system exclusive message and show that this is a master volume system exclusive message
//    (0x04 shows that this is a device control message and 0x01 shows that this is a master volume message)
// 0xmm and 0xnn together form a 14-bit volume value. 0xmm carries the low 7 bits and 0xnn carries the high 7 bits of the 14-bit value.
// volume range 0..16383, i.e. 0x0000 to 0x3FFF with mid value 01FFF (8191)
// 0x1FFF as volume would be 0x3f7f.
// https://www.casiomusicforums.com/index.php?/topic/6156-has-anyone-got-master-volume-controlled-by-midi/
class SysExVolume: public EventRepresentation
{
public:
  SysExVolume(int16 volume, int64_t timestamp): EventRepresentation(0xF0, 9, buildVolume(volume), timestamp) { }
        
  static uint8 * buildVolume(int16 volume) {
    uint8 * p = (uint8 *)malloc(8);
    uint32 b1 = ((uint8 *)&volume)[0];
    uint32 b2 = ((uint8 *)&volume)[1];
    b2 = b2 << 1;
    if (b1 & 0x80) {
      b1 = b1 & 0x7f;
      b2 = b2 | 0x01;
    } else {
      b2 = b2 & 0xFE;
    }
          
    p[0] = 0xF0;
    p[1] = 0x7F;
    p[2] = 0x7F;
    p[3] = 0x04;
    p[4] = 0x01;
    p[5] = b1;  // low
    p[6] = b2;  // high
    p[7] = 0xF7;
    return p;
  }
};
      
} // VST3Host
} // Vst
} // Steinberg

#endif /* vst3eventstructures_h */

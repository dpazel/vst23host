#ifndef MidiEvent_h
#define MidiEvent_h

#ifndef byte
typedef unsigned char byte;
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


typedef enum {
    MIDI_TYPE_INVALID,
    MIDI_TYPE_REGULAR,
    MIDI_TYPE_SYSEX,
    MIDI_TYPE_META,
    NUM_MIDI_TYPES
} MidiEventType;

typedef struct {
    MidiEventType eventType;
    unsigned long deltaFrames;
    unsigned long timestamp;
    byte status;
    byte data1;
    byte data2;
    byte* extraData;
} MidiEventMembers;

// MIDI Meta Event types
#define MIDI_META_TYPE_TEXT 0x01
#define MIDI_META_TYPE_COPYRIGHT 0x02
#define MIDI_META_TYPE_SEQUENCE_NAME 0x03
#define MIDI_META_TYPE_INSTRUMENT 0x04
#define MIDI_META_TYPE_LYRIC 0x05
#define MIDI_META_TYPE_MARKER 0x06
#define MIDI_META_TYPE_CUE_POINT 0x07
#define MIDI_META_TYPE_PROGRAM_NAME 0x08
#define MIDI_META_TYPE_DEVICE_NAME 0x09
#define MIDI_META_TYPE_TEMPO 0x51
#define MIDI_META_TYPE_TIME_SIGNATURE 0x58
#define MIDI_META_TYPE_KEY_SIGNATURE 0x59
#define MIDI_META_TYPE_PROPRIETARY 0x7f
#define MIDI_META_TYPE_TRACK_END 0x2f

class MidiEvent {
public:
    MidiEvent();
  
    MidiEvent(byte code, int channel, int byte1, int byte2, long deltaFrames);
    
    MidiEventType eventType;
    unsigned long deltaFrames;
    unsigned long timestamp;
    byte status;
    byte data1;
    byte data2;
    byte* extraData;
protected:
private:
    
};

#endif  // MidiEvent_h

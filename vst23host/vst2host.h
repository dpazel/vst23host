#ifndef VST2HOST_H
#define VST2HOST_H

#include <list>

// The following includes are header files for VST2 plug0ins.
// You are required to have VST2 SDK in your header searchpaths.
// The VST2 SDK can be found in the vstsdk3610_11_06_2018_build_37 (or older) VST3 SDK
// Ref. VST2_SDK_PATH, and VST2_SDK_PATH_PUBLIC in build/
#include "pluginterfaces/vst2.x/aeffect.h"
#include "pluginterfaces/vst2.x/aeffectx.h"
#include "vst2.x/audioeffect.h"
#include "vst2.x/audioeffectx.h"
#include "pluginterfaces/vst2.x/vstfxstore.h"

#include "midievent.h"
#include <string>

using namespace std;

class VST2Host {
public:
  enum RenderState {INACTIVE, ACTIVE};
  VST2Host();
  ~VST2Host();
  
  bool openVST(string locationForVst);
  
  AEffect * getPlugin() { return plugin; }
  void openEditWindow();
  void openEditWindowCocoa();
  void closeEditWindow();
  bool getBank(int &dataLength, void **data);
  bool saveBank(string fileName);
  bool setBank(int length, void *bankData);
  bool loadBank(string fileName);
  void displayData();
  
  size_t getCurrentSample() { return current_sample; }
  size_t getSampleRate() const { return 44100; }
  
  bool processEvents(VstMidiEvent** events,
                     int numEvents,
                     int playTimeInMillisec,
                     double ** leftBuffer,
                     double ** rightBuffer,
                     int &numSamplesProduced);
  
  bool beginEventRendering();
  void feedEvents(list<VstMidiEvent *> &events);
  bool processEvents(int playTimeInMillisec, long frameBaseLine, double ** leftBuffer, double ** rightBuffer, long &numSamplesProduced);
  long numberEventsInProcess();
  bool endEventRendering();
  RenderState getRenderState() { return render_state; }
  VstTimeInfo * getTimeInfo() { return &timeInfo; }
  
  AudioEffectX * getAudioEffect() { return (AudioEffectX*)audioEffect; }
  
  int sendSimpleMidiNotes(float ** bufferl, float ** bufferr, int numSeconds);
  
  static VstEvents* convertToVstMidi(MidiEvent** midiEvents, int numEvents);
  
protected:
private:
  void cleanRenderBuffers();
  void deallocateCurrentList();
  void deallocateDiscardList();
  
  AEffect *plugin;
  AudioEffect *audioEffect;
  bool canDoReplacing;
  bool canDoDoubleReplacing;
  // Path to location of vst2 plugin.
  string vstLocation { "" };
  
  VstTimeInfo timeInfo {};
  size_t current_sample { 0 };
  
  // Used for event rendering
  float ** inbase;
  float ** outbase;
  RenderState render_state;
  list<VstMidiEvent *> current_events;
  long event_base_count;   // For each processEvents(), this is used as the new 'base' on the rendering cycle.
  list<VstMidiEvent *> discard_list;   // Holds events fully rendered, and needs to be discarded.
  
};
#endif // VST2HOST_H

#ifndef vst3host_hpp
#define vst3host_hpp

#include <stdio.h>
#include <string>
#include <array>
#include <vector>
#include <list>

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "vst/hosting/hostclasses.h"
#include "vst/hosting/processdata.h"
#include "vst/hosting/eventlist.h"
#include "vst/hosting/parameterchanges.h"
#include "vst/hosting/plugprovider.h"

#include "main/pluginfactoryvst3.h"

#include "vst3eventstructures.h"

using namespace std;

namespace Steinberg {
namespace Vst {
namespace VST3Host {
  
class AbstractWindow;

class VST3Host {
public:
  VST3Host(const string& vst_player_path);
  ~VST3Host();
  
  VST3::Hosting::Module::Ptr getModule() { return this->module; }
  const VST3::Hosting::PluginFactory * getFactory() { return this->factory; }
  IPtr<PlugProvider> getPlugProvider() { return this->plug_provider; }
  IComponent * getComponent() { return this->component; }
  IEditController* getController() { return this->controller; }
  IAudioProcessor* getAudioEffect() { return this->audioEffect; }
  IMidiMapping* getMidiMapping() { return this->midiMapping; }
  //Steinberg::Vst::VSTPlayerTestApp::IPlatform* getPlatform() { return this->platform; }
  void setWindow(AbstractWindow* w) { window = w; }
  AbstractWindow* getWindow() { return window; }
  SampleRate getSampleRate() { return sampleRate; }
  static std::ostream* getErrorStream() { return errorStream; }

  void feedEvents(vector<EventRepresentation *> &events);
  void clearFeedEvents();
  bool processEvents(int playTimeInMillisec, long frameBaseLine, float ** leftBuffer, float ** rightBuffer, long &numSamplesProduced);
  
  void createViewAndShow();
  
  void savePreset(string filename);
  bool loadPreset(string fileName);
  
  // smart pointer support  :-)
  uint32 PLUGIN_API addRef ()  { return 1000; }
  uint32 PLUGIN_API release ()  { return 1000; }

private:
  
  static std::ostream* errorStream;

  VST3::Hosting::Module::Ptr openModule(const string&);
  VST3::Hosting::ClassInfo * produceAudioClass();
  IPtr<PlugProvider> producePlugProvider();
  list<EventRepresentation *> current_events;
  
  void processInit();
  static MidiCCMapping initMidiCtrlerAssignment (IComponent* component, IMidiMapping* midiMapping);
  void initProcessData();
  
  bool processEvent (EventList &eventList, EventRepresentation* event_representation, long block_sample_start, int32 port=0);
  bool isPortInRange (int32 port, int32 channel) const;

  bool processParamChange (ParameterChanges &input_parameter_changes, const EventStructure& event, long block_sample_start, int32 port=0);

  static void assignBusBuffers (const Buffers& buffers, HostProcessData& processData, bool unassign = false);
  bool process_chunk_events(EventList &target_event_list, ParameterChanges &parameter_changes, long block_sample_start, int32 block_size, list<EventRepresentation *> &events);
  
  VST3::Hosting::Module::Ptr module;
  const VST3::Hosting::PluginFactory *factory {nullptr};
  IPtr<PlugProvider> plug_provider {nullptr};
  IComponent *component {nullptr};
  IEditController *controller {nullptr};
  IPtr<IAudioProcessor> audioEffect {nullptr};
  
  AbstractWindow* window { nullptr };
  bool isProcessing = false;
  int32 blockSize = 1024;
  SampleRate sampleRate = 44100;   // Samples per second.
  
  ParameterChangeTransfer paramTransferrer;
  
  IMidiMapping *midiMapping;
  
  MidiCCMapping midiCCMapping;
  
  Steinberg::Vst::HostApplication hostApp;
};

void printAllInstalledPlugins ();
VST3::Hosting::Module::Ptr openModule(string&);
  
} // VST3Host
} // Vst
} // Steinberg

#endif /* vst3host_hpp */

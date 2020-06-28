#define DEVELOPMENT 1

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "vst3host.hpp"

#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/gui/iplugview.h"
#include "vst/hosting/stringconvert.h"
#include "vst/vstpresetfile.h"

#include "windowcontroller.h"
#include "abstractwindow.h"
#include "miditovst.h"

#include "hostutil.h"

using namespace std;

std::ostream* Steinberg::Vst::VST3Host::VST3Host::errorStream = &std::cout;

// gStandardPluginContext is referenced in plugprovider.cpp.  It is thus defined here and set below.
namespace Steinberg { FUnknown* gStandardPluginContext = nullptr; }

class ComponentHandler : public Steinberg::Vst::IComponentHandler
{
public:
  Steinberg::tresult PLUGIN_API beginEdit (Steinberg::Vst::ParamID /*id*/) override {
    return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API performEdit (Steinberg::Vst::ParamID /*id*/, Steinberg::Vst::ParamValue /*valueNormalized*/) override
  {
    return Steinberg::kNotImplemented;
  }
  Steinberg::tresult PLUGIN_API endEdit (Steinberg::Vst::ParamID /*id*/) override {
    return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API restartComponent (Steinberg::int32 /*flags*/) override {
    return Steinberg::kNotImplemented; }
  
private:
  Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID /*_iid*/, void** /*obj*/) override
  {
    return Steinberg::kNoInterface;
  }
  Steinberg::uint32 PLUGIN_API addRef () override { return 1000; }
  Steinberg::uint32 PLUGIN_API release () override { return 1000; }
};

static ComponentHandler gComponentHandler;

namespace Steinberg {
namespace Vst {
namespace VST3Host {

//------------------------------------------------------------------------
VST3Host::VST3Host(const string& vst_player_path) {
  log("VST3Host::VST3Host - construction.");
  Steinberg::gStandardPluginContext = &hostApp;
  
  this->module = this->openModule(vst_player_path);
  if (!this->module) {
    *errorStream << "VST3Host::VST3Host - openModule() did not work." << endl;
    return;
  }
  
  this->plug_provider = this->producePlugProvider();
  this->component = this->plug_provider->getComponent();
  if (!this->component) {
    *errorStream << "VST3Host::VST3Host - no component." << endl;
    return;
  }
  
  this->controller = this->plug_provider->getController();
  if (!this->controller) {
    *errorStream << "VST3Host::VST3Host - no controller." << endl;
    return;
  }

  IAudioProcessor* aeffect = nullptr;
  Steinberg::tresult check = component->queryInterface (Steinberg::Vst::IAudioProcessor::iid, (void**)&(aeffect));
  if (check != Steinberg::kResultTrue) {
    *errorStream << "VST3Host::VST3Host - no audio processor." << endl;
    return;
  }
  this->audioEffect = aeffect;
  
  check = controller->queryInterface (Steinberg::Vst::IMidiMapping::iid, (void**)&(this->midiMapping));
  if (check != Steinberg::kResultTrue) {
    log("VST3Host::VST3Host - no IMidiMapping.");
  } else {
    log("VST3Host::VST3Host - found  IMidiMapping.");
  }

  this->controller->setComponentHandler (&gComponentHandler);

  processInit();
  
  if (this->getAudioEffect()->canProcessSampleSize(kSample32) == kResultTrue) {
    log("VST3Host::VST3Host - can process 32 bit.");
  } else {
    log("VST3Host::VST3Host - cannot process 32 bit");
  }
  if (this->getAudioEffect()->canProcessSampleSize(kSample64) == kResultTrue) {
    log("VST3Host::VST3Host - can process 64 bit");
  } else {
    log("VST3Host::VST3Host - cannot process 64 bit");
  }
  
  log("VST3Host::VST3Host - finish construction.");
}
    
//------------------------------------------------------------------------
void VST3Host::processInit() {
  paramTransferrer.setMaxParameters (1000);
  
  FUnknownPtr<IMidiMapping> midiMapping (controller);
  
  midiCCMapping = initMidiCtrlerAssignment(component, midiMapping);
  
  log("VST3Host::processInit - num busses = %ld", midiCCMapping.size());
  
  for (int i=0; i < midiCCMapping.size(); i++) {
    Channels cs = midiCCMapping[i];
    log("VST3Host::processInit - bus[%d] has %ld channels.", i, cs.size());
    for (int j = 0; j < cs.size(); ++j) {
      Controllers ctls = cs[j];
      log("VST3Host::processInit - channel[%d] has %ld components.", j, ctls.size());
    }
  }
}

//------------------------------------------------------------------------
void printComponentInfo(IComponent* component) {
  cout << "Component event bus inputs: " << component->getBusCount (kEvent, kInput) << endl;
  cout << "Component audio bus inputs: " << component->getBusCount (kAudio, kInput) << endl;
  cout << "Component event bus outputs: " << component->getBusCount (kEvent, kOutput) << endl;
  cout << "Component audio bus outputs: " << component->getBusCount (kAudio, kOutput) << endl;
  
  for (int i = 0; i < component->getBusCount (kAudio, kOutput); ++i) {
    BusInfo busInfo;
    component->getBusInfo (kAudio, kOutput, i, busInfo);
    cout << "   " << "output audio [" << i << "] named " << VST3::StringConvert::convert(busInfo.name).c_str() << " has channel count " << busInfo.channelCount <<
    " flags: " << busInfo.flags << endl;
  }
}

//------------------------------------------------------------------------
MidiCCMapping VST3Host::initMidiCtrlerAssignment (IComponent* component, IMidiMapping* midiMapping) {
  MidiCCMapping midiCCMapping {};
        
  if (!midiMapping || !component)
    return midiCCMapping;
        
  int32 busses = std::min<int32> (component->getBusCount (kEvent, kInput), kMaxMidiMappingBusses);
        
  if (midiCCMapping[0][0].empty ())
  {
    for (int32 b = 0; b < busses; b++) {
      for (int32 i = 0; i < kMaxMidiChannels; i++) {
        midiCCMapping[b][i].resize (Vst::kCountCtrlNumber);
      }
    }
  }
        
  ParamID paramID;
  for (int32 b = 0; b < busses; b++)
  {
    for (int16 ch = 0; ch < kMaxMidiChannels; ch++)
    {
      for (int32 i = 0; i < Vst::kCountCtrlNumber; i++)
      {
        paramID = kNoParamId;
        if (midiMapping->getMidiControllerAssignment (b, ch, (CtrlNumber)i, paramID) == kResultTrue)
        {
          midiCCMapping[b][ch][i] = paramID;
        }
        else {
          midiCCMapping[b][ch][i] = kNoParamId;
        }
      }
    }
  }
  return midiCCMapping;
}
    
//------------------------------------------------------------------------
VST3Host::~VST3Host() {
  log("VST3Host::~VST3Host - deleting VST3Host");
  plug_provider.reset();
  audioEffect.reset();
  delete plug_provider;
  delete factory;
  
  module.reset();
  gStandardPluginContext = nullptr;
  log("VST3Host::~VST3Host - delete finished");
}

//------------------------------------------------------------------------
VST3::Hosting::Module::Ptr VST3Host::openModule(const string& vst_player_path) {
  std::string error;
  VST3::Hosting::Module::Ptr module = VST3::Hosting::Module::create (vst_player_path, error);
  if (!module)
  {
    *errorStream << "No Module: \n" << (!error.empty() ? error : "") << "\n";
  }
  return (module) ? module : nullptr;
}

//------------------------------------------------------------------------
void printAllInstalledPlugins ()
{
  cout << "Searching installed plug-ins...\n";
  cout.flush ();
  
  auto paths = VST3::Hosting::Module::getModulePaths();
  if (paths.empty ())
  {
    cout << "No plug-ins found.\n";
    return;
  }
  for (const auto& path : paths)
  {
    cout << path << "\n";
  }
}

//------------------------------------------------------------------------
VST3::Hosting::ClassInfo *VST3Host::produceAudioClass() {
  log("VST3Host::produceAudioClass - scanning classes...");
    
  auto factoryInfo = factory->info ();
  
  log("Factory Info:\n\tvendor = %s \n\turl = %s \n\temail = %s\n",
      factoryInfo.vendor().c_str(), factoryInfo.url().c_str(), factoryInfo.email().c_str());
    
  //---print all included Plug-ins---------------
  VST3::Hosting::ClassInfo *audio_class = nullptr;
  int i = 0;
  for (auto& classInfo : factory->classInfos ()) {
    log("Class Info[%d]:\n\tname = %s\n\tcategory = %s\n\tcid = %s\n", i, classInfo.name().c_str(),
        classInfo.category().c_str(), classInfo.ID().toString().c_str());
    ++i;
    if (classInfo.category () == kVstAudioEffectClass) {
      audio_class = &classInfo;
      break;
    }
  }
  
  if (!audio_class) {
    log("VST3Host::produceAudioClass - no Audio Class found.");
  }
  return audio_class;
}

//------------------------------------------------------------------------
IPtr<PlugProvider> VST3Host::producePlugProvider() {
  log("VST3Host::producePlugProvider");
  
  auto factory = module->getFactory ();
  for (auto& classInfo : factory.classInfos ()) {
    if (classInfo.category () == kVstAudioEffectClass)
    {
      log("VST3Host::producePlugProvider - found plug provider.");
      IPtr<PlugProvider> pp =  owned(new Steinberg::Vst::PlugProvider (this->module->getFactory(), classInfo, true));
      return pp;
    }
  }
  
  *errorStream << "VST3Host::producePlugProvider - no plug provider.\n";
  return nullptr;
}

//------------------------------------------------------------------------
void VST3Host::createViewAndShow()
{
  log("VST3Host::createViewAndShow - Entering.");
  Steinberg::IPtr<Steinberg::IPlugView> view = owned(this->controller->createView (Steinberg::Vst::ViewType::kEditor));
  if (!view) {
    *errorStream << "VST3Host::createViewAndShow - EditController does not provide its own editor view.";
    return;
  }
  
  Steinberg::ViewRect plugViewSize {};
  auto result = view->getSize(&plugViewSize);
  if (result != Steinberg::kResultTrue) {
    *errorStream << "Could not get editor view size";
    return;
  }
  
  auto viewRect = Steinberg::Vst::VST3Host::ViewRectToRect(plugViewSize);
  log("VST3Host::createViewAndShow - ViewRect width/height (%d, %d)", viewRect.size.width, viewRect.size.height);

  Steinberg::Vst::VST3Host::AbstractWindow* window = Steinberg::Vst::VST3Host::AbstractWindow::make ("Editor",
                                                                viewRect.size,
                                                                view->canResize () == Steinberg::kResultTrue,
                                                                std::make_shared<Steinberg::Vst::VST3Host::WindowController> (view, this));
  
  if (!window) {
    *errorStream << "VST3Host::createViewAndShow - could not create window";
    return;
  }

  window->show ();
  
  this->setWindow(window);  
}

//------------------------------------------------------------------------
void VST3Host::assignBusBuffers (const Buffers& buffers, HostProcessData& processData, bool unassign)
{
  // Set outputs
  auto bufferIndex = 0;
  for (auto busIndex = 0; busIndex < processData.numOutputs; busIndex++)
  {
    auto channelCount = processData.outputs[busIndex].numChannels;
    for (auto chanIndex = 0; chanIndex < channelCount; chanIndex++)
    {
      if (bufferIndex < buffers.numOutputs)
      {
        processData.setChannelBuffer (BusDirections::kOutput, busIndex, chanIndex,
                                      unassign ? nullptr : buffers.outputs[bufferIndex]);
        bufferIndex++;
      }
    }
  }
        
  // Set inputs
  bufferIndex = 0;
  for (auto busIndex = 0; busIndex < processData.numInputs; busIndex++)
  {
    auto channelCount = processData.inputs[busIndex].numChannels;
    for (auto chanIndex = 0; chanIndex < channelCount; chanIndex++)
    {
      if (bufferIndex < buffers.numInputs)
      {
        processData.setChannelBuffer (BusDirections::kInput, busIndex, chanIndex,
                                      unassign ? nullptr : buffers.inputs[bufferIndex]);
              
        bufferIndex++;
      }
    }
  }
}
      
//------------------------------------------------------------------------
bool VST3Host::isPortInRange (int32 port, int32 channel) const
{
  return port < kMaxMidiMappingBusses && !midiCCMapping[port][channel].empty ();
}

//------------------------------------------------------------------------
void log_events(list<EventRepresentation *> &events) {
  log("log_events: there are %lu events.", events.size());
  
  std::list<EventRepresentation *>::iterator it;
  int i = 0;
  for (it = events.begin(); it != events.end(); ++it, ++i){
    EventStructure* e = (*it)->getEventStructure();
    log("  [%d] type: %d channel: %d data0: %d data1: %d ts: %lld", i, (int)(e->type), (int)(e->channel), (int)(e->data0), (int)(e->data1), e->timestamp);
  }
}
 
//------------------------------------------------------------------------
void VST3Host::feedEvents(vector<EventRepresentation *> &events) {
  log("VST3Host::feedEvents - %lu events.", events.size());
  current_events.insert(current_events.end(), events.begin(), events.end());
  log_events(current_events);
  log("VST3Host::feedEvents - end feedEvents, total=%lu events.", current_events.size());
}

//------------------------------------------------------------------------
void VST3Host::clearFeedEvents() {
  current_events.clear();
}
  
//------------------------------------------------------------------------
bool VST3Host::process_chunk_events(EventList &target_event_list, ParameterChanges &parameter_changes, long block_sample_start, int32 block_size, list<EventRepresentation *> &events) {
  for(auto it = events.begin(); it != events.end(); ++it) {
    if ((*it)->getTimestamp() >= block_sample_start && (*it)->getTimestamp() < block_sample_start + block_size) {
      if (!processEvent(target_event_list, *it, block_sample_start)) {
        processParamChange(parameter_changes, *((*it)->getEventStructure()), block_sample_start);
      }
      events.erase(it--);
    }
  }
}
      
//------------------------------------------------------------------------
bool VST3Host::processEvent (EventList &eventList, EventRepresentation* event_representation, long block_sample_start, int32 port) {
  if (!event_representation->is_data_type) {
    EventStructure* event_structure = event_representation->getEventStructure();
    if (event_structure->type != kNoteOn && event_structure->type != kNoteOff && event_structure->type != kProgramChangeStatus) {
      return false;
    }
    VST3::Optional<Event> event = midiToEvent (event_structure->type, event_structure->channel, event_structure->data0, event_structure->data1);
    if (event) {
      event->busIndex = port;
      event->sampleOffset = event_structure->timestamp - block_sample_start;
      if (eventList.addEvent (*event) != kResultOk) {
        *errorStream << "VST3Host::processEvent - non-data Event was not added to target EventList!" << endl;
        return false;
      }
          
      return true;
    }
  } else {
    DataEventStructure* data_event_structure = event_representation->getDataEventStructure();
    VST3::Optional<Event> event = midiToDataEvent (data_event_structure->type, data_event_structure->size, data_event_structure->bytes);
    if (event) {
      event->busIndex = port;
      event->sampleOffset = data_event_structure->timestamp - block_sample_start;
      if (eventList.addEvent (*event) != kResultOk) {
        *errorStream << "VST3Host::processEvent - Data Event was not added to target EventList!" << endl;
        return false;
      }
      
      return true;
    }
  }
        
  return false;
}
      
//------------------------------------------------------------------------
bool VST3Host::processParamChange (ParameterChanges &input_parameter_changes, const EventStructure& event_structure, long block_sample_start, int32 port) {
  auto paramMapping = [port, this] (int32 channel, MidiData data1) -> ParamID {
    if (!isPortInRange (port, channel))
      return kNoParamId;
    
    return midiCCMapping[port][channel][data1];
  };
  
  auto paramChange = midiToParameter (event_structure.type, event_structure.channel, event_structure.data0, event_structure.data1, paramMapping);
  if (paramChange) {
    int32 index = 0;
    IParamValueQueue* queue = input_parameter_changes.addParameterData ((*paramChange).first, index);
    if (queue) {
      if (queue->addPoint (event_structure.timestamp - block_sample_start, (*paramChange).second, index) != kResultOk) {
        cout << "Parameter point was not added to ParamValueQueue!" << endl;
        return false;
      }
    }
          
    return true;
  }
        
  return false;
}

//------------------------------------------------------------------------
bool VST3Host::processEvents(int playTimeInMillisec, long frameBaseLine,
                             float ** leftBuffer, float ** rightBuffer, long &numSamplesProduced) {
  long totalSamples = floor((playTimeInMillisec / 1000.0) * sampleRate);
  
  log("VST3Host::processEvents - generating %ld samples in blocks of %d.", totalSamples, blockSize);
        
  long endFrame = frameBaseLine + totalSamples;
  long frameCount = frameBaseLine;
  long startFrame = 0;
  numSamplesProduced = 0;
  
  *leftBuffer = new Sample32[totalSamples];;
  *rightBuffer = new Sample32[totalSamples];
        
  audioEffect->setProcessing (false);
  
  BusInfo busInfo;
  component->getBusInfo (kAudio, kOutput, 0, busInfo);  // Only port 0!
  Vst::AudioBusBuffers output;
  output.numChannels = busInfo.channelCount;
  output.silenceFlags = 0;
  output.channelBuffers32 = (Sample32 **) new Sample32 * [output.numChannels];
  for (int i = 0; i < output.numChannels; i++) {
    output.channelBuffers32[i] = new Sample32[blockSize];
  }
  
  Vst::ProcessContext processContext = {};
  Vst::ProcessData processData = {};
        
  ProcessSetup setup {kOffline, kSample32, blockSize, sampleRate};
  audioEffect->setupProcessing (setup);
  component->setActive (true);
  
  int loop_count = 1;
  do {
    long chunk_size = std::min((long)blockSize, endFrame - frameCount);
    log("VST3Host::processEvents - loop to produce %ld samples.", chunk_size);
    
    EventList localEventList(20);
    ParameterChanges inputParameterChanges;
    this->process_chunk_events(localEventList, inputParameterChanges, frameCount, chunk_size, current_events);
    for (int i=0; i < localEventList.getEventCount(); i++) {
      Event e;
      localEventList.getEvent(i, e);
    }
  
    processContext.tempo = 60;
    processContext.sampleRate = sampleRate;
    processContext.projectTimeSamples = frameCount;
    processContext.continousTimeSamples = 0;
    processContext.state =
          Vst::ProcessContext::kTempoValid |
          Vst::ProcessContext::kProjectTimeMusicValid;

    processData.processMode = Vst::kOffline;
    processData.symbolicSampleSize = kSample32;
    processData.numSamples = chunk_size;
    processData.numOutputs = 2;
    processData.outputs = &output;
    processData.inputEvents = &localEventList;
    EventList* outputEventList = new EventList();
    processData.outputEvents = outputEventList;
    
    processData.inputParameterChanges = &inputParameterChanges;
    Vst::ParameterChanges out_parameter_changes;
    processData.outputParameterChanges = &out_parameter_changes;
    processData.processContext = &processContext;
      
    audioEffect->setProcessing (true);
    log("VST3Host::processEvents - calling vst process() loop [%d].", loop_count);
    if ( audioEffect->process(processData) != kResultOk) {
      *errorStream << "VST3Host::processEvents - failure processing events." << endl;
      audioEffect->setProcessing (false);
      break;
    }
        
    audioEffect->setProcessing (false);
    loop_count++;
      
    for (int i = 0; i < processData.numOutputs; ++i) {
      if (i == 0) {
        for (int j = 0; j < processData.outputs[i].numChannels; ++j) {
          Sample32* samples = processData.outputs[i].channelBuffers32[j];
          for (int k = 0; k < chunk_size; ++k) {
            if (j == 0) {
              (*leftBuffer)[startFrame + k] = samples[k];
            } else {
              (*rightBuffer)[startFrame + k] = samples[k];
            }
          }
        }
      }
    }
      
    startFrame += chunk_size;
    frameCount += chunk_size;
    numSamplesProduced += chunk_size;
  } while (frameCount < endFrame);
  
  // Clean up allocated memory
  for (int i = 0; i < output.numChannels; i++) {
    delete [] output.channelBuffers32[i];
  }
  
  log("VST3Host::processEvents - exiting.");
        
  return true;
}

//------------------------------------------------------------------------
void VST3Host::savePreset(string fileName) {
  log("VST3Host::savePreset - '%s' NOT IMPLEMENTED", fileName.c_str());
}

//------------------------------------------------------------------------
bool VST3Host::loadPreset(string fileName) {
  log("VST3Host::loadPreset - '%s' NOT IMPLEMENTED", fileName.c_str());
}
  
//------------------------------------------------------------------------
void print_parameters(IEditController* editController) {
  cout << "There are " << editController->getParameterCount() << " parameters." << endl;
  for (int i = 0 ; i < editController->getParameterCount(); ++i) {
    ParameterInfo pi;
    editController->getParameterInfo(i, pi);
    if (pi.flags & ParameterInfo::kIsProgramChange) {
      string title = VST3::StringConvert::convert(pi.title).c_str();
      string shortTitle = VST3::StringConvert::convert(pi.shortTitle).c_str();
      string units = VST3::StringConvert::convert(pi.units).c_str();
      int f = pi.flags & ParameterInfo::kIsProgramChange;
      cout << "  [" << i << "] id=" << pi.id << " title=" << title << " shortTitle=" << shortTitle << " stepCount=" << pi.stepCount << " f=" << f <<
      " units=" << units << " unitId=" << pi.unitId << " stepcount=" << pi.stepCount << " denormalized:" << pi.defaultNormalizedValue << endl;
    }
  }
}
  
//------------------------------------------------------------------------
void print_keyswitches(IComponent * component, IEditController * controller) {
  int32 eventBusCount = component->getBusCount (kAudio, kOutput);
  cout << "output event bus count=" << eventBusCount << endl;
    
  FUnknownPtr<IKeyswitchController> keyswitchcontroller (controller);
  if (!keyswitchcontroller)
  {
    cout << "No Keyswitch interface supplied!" << endl;
  } else  {
    for (int32 bus = 0; bus < eventBusCount; bus++) {
      if (bus == 0) {
        BusInfo busInfo;
        component->getBusInfo (kAudio, kOutput, bus, busInfo);
        for (int16 channel = 0; channel < busInfo.channelCount; channel++) {
          int32 count = keyswitchcontroller->getKeyswitchCount(bus, channel);
          cout << "There are " << count << " keyswitches bus " << bus << " channel " << channel <<endl;
          
          for (int32 programListIndex = 0; programListIndex < count; programListIndex++) {
            KeyswitchInfo keyswitchInfo;
            if (keyswitchcontroller->getKeyswitchInfo(bus, channel, programListIndex, keyswitchInfo) == kResultOk) {
              cout << "--- [" << keyswitchInfo.keyswitchMin << "] list title=" << VST3::StringConvert::convert(keyswitchInfo.title).c_str() <<
              "  short title=" << VST3::StringConvert::convert(keyswitchInfo.shortTitle).c_str() <<
              " typeId=" << keyswitchInfo.typeId << " min=" << keyswitchInfo.keyswitchMin << " max=" << keyswitchInfo.keyswitchMax << " unitID=" <<
              keyswitchInfo.unitId << " keyremapped=" << keyswitchInfo.keyRemapped << endl;
            }
          }
        }
      }
    }
      
    cout << "------ note expressions -----" << endl;
    FUnknownPtr<INoteExpressionController> noteExpressionController (controller);
    if (noteExpressionController) {
      int32 exprCount = noteExpressionController->getNoteExpressionCount (0, 0);
      for (int32 i = 0; i < exprCount; ++i) {
        NoteExpressionTypeInfo info;
       noteExpressionController->getNoteExpressionInfo (0, 0, i, info);
          
       cout << "[" << i << "] type=" << info.typeId << " title=" << VST3::StringConvert::convert(info.title) << " short title=" <<
          VST3::StringConvert::convert(info.shortTitle) << " units=" << VST3::StringConvert::convert(info.units) << " param=" << info.associatedParameterId << endl;
          
       }
     }
   }
    
  FUnknownPtr<IUnitInfo> iUnitInfo (controller);
  if (!iUnitInfo) {
    cout << "No iUnitInfo" << endl;
    return;
  }
  
  int32 uc = iUnitInfo->getUnitCount ();
  cout << "Unit info count=" << uc << endl;
  for (int32 ui = 0; ui < uc; ++ui)
  {
    UnitInfo uinfo = {0};
    if (iUnitInfo->getUnitInfo (ui, uinfo) == kResultTrue) {
      cout << "  [" << ui << "] name=" << VST3::StringConvert::convert(uinfo.name).c_str() << " id=" << uinfo.id << endl;
    }
  }
  int32 plc = iUnitInfo->getProgramListCount();
  cout << "There are " << plc << " program lists." << endl;
  for (int32 pl = 0; pl < plc; ++pl) {
    ProgramListInfo pli;
    iUnitInfo->getProgramListInfo(pl, pli);
    cout << "  [" << pl << "] name=" << VST3::StringConvert::convert(pli.name).c_str() << " id=" << pli.id << " num programs=" << pli.programCount << endl;
  }
  // Parameters
  int32 p_ct = controller->getParameterCount();
  cout << "Edit controller has " << p_ct << " parameters." << endl;
  for (int i = 0; i < p_ct; ++i) {
    ParameterInfo pi;
    controller->getParameterInfo(i, pi);
    // cout << "  [" << i << "] " << VST3::StringConvert::convert(pi.title) << endl;
  }
    
  FUnknownPtr<IUnitData> iUnitData(component);
  if (iUnitData == nullptr) {
    cout << "There is no IUnitData." << endl;
  } else {
    cout << "There is IUnitData." << endl;
  }
  
  FUnknownPtr<IProgramListData> iProgramListData(component);
  if (iProgramListData == nullptr) {
    cout << "There is no iProgramListData." << endl;
  } else {
    cout << "There is iProgramListData." << endl;
    for (int32 pl = 0; pl < plc; ++pl) {
      ProgramListInfo pli;
      iUnitInfo->getProgramListInfo(pl, pli);
      tresult trslt = iProgramListData->programDataSupported(pli.id);
      cout << "plid=" << pli.id << " program data supported rc=" << trslt << endl;
    }
  }
    
}
  
//------------------------------------------------------------------------
void print_context(ProcessContext & context) {
  std::cout << "state=" << std::hex << context.state << std::dec << std::endl;
  cout << "sample rate=" << context.sampleRate << endl;
  cout << "project time samples=" << context.projectTimeSamples << endl;
  cout << "system time=" << context.systemTime << endl;
  cout << "continuous time samples=" << context.continousTimeSamples << endl;
  cout << "project time music=" << context.projectTimeMusic << endl;
  cout << "bar position music=" << context.barPositionMusic << endl;
  cout << "cycleStartMusic=" << context.cycleStartMusic << endl;
  cout << "cycleEndMusic=" << context.cycleEndMusic << endl;
  cout << "tempo=" << context.tempo << endl;
}
  
} // VST3Host
} // Vst
} // Steinberg

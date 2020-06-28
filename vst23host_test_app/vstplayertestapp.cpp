#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "vstplayertestapp.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/vsttypes.h"

#include "../vst23host/vst3host.hpp"
#include "../vst23host/windowcontroller.h"
#include "../vst23host/miditovst.h"

#include "appinit.h"
#include "platform.h"

//------------------------------------------------------------------------
namespace Steinberg {
  
//------------------------------------------------------------------------
inline bool operator== (const ViewRect& r1, const ViewRect& r2)
{
  return memcmp (&r1, &r2, sizeof (ViewRect)) == 0;
}
  
//------------------------------------------------------------------------
inline bool operator!= (const ViewRect& r1, const ViewRect& r2)
{
  return !(r1 == r2);
}
  
namespace Vst {
namespace VSTPlayerTestApp {
      
  static AppInit gInit (std::make_unique<App> ());
  
  //------------------------------------------------------------------------
  class ComponentHandler : public IComponentHandler
  {
  public:
    tresult PLUGIN_API beginEdit (ParamID /*id*/) override { return kNotImplemented; }
    tresult PLUGIN_API performEdit (ParamID /*id*/, ParamValue /*valueNormalized*/) override
    {
      return kNotImplemented;
    }
    tresult PLUGIN_API endEdit (ParamID /*id*/) override { return kNotImplemented; }
    tresult PLUGIN_API restartComponent (int32 /*flags*/) override { return kNotImplemented; }
        
  private:
    tresult PLUGIN_API queryInterface (const TUID /*_iid*/, void** /*obj*/) override
    {
      return kNoInterface;
    }
    uint32 PLUGIN_API addRef () override { return 1000; }
    uint32 PLUGIN_API release () override { return 1000; }
  };
      
//------------------------------------------------------------------------
  App::~App () noexcept
  {
    plugProvider.reset ();
    module.reset ();
  }
      
  //------------------------------------------------------------------------
  void App::init (const std::vector<std::string>& cmdArgs)
  {
    if (cmdArgs.empty ())
    {
      auto helpText = R"(
        usage: EditorHost [options] pluginPath
          
        options:
          
          --componentHandler
          set optional component handler on edit controller
          
          --uid UID
          use effect class with unique class ID==UID
      )";
      cout << "Kill called for help.\n";
      IPlatform::instance ().kill (0, helpText);
    }
        
    VST3::Optional<VST3::UID> uid;
    uint32 flags {};
    for (auto it = cmdArgs.begin (), end = cmdArgs.end (); it != end; ++it)
    {
      if (*it == "--componentHandler")
        flags |= kSetComponentHandler;
      else if (*it == "--uid")
      {
        if (++it != end)
          uid = VST3::UID::fromString (*it);
        if (!uid)
          IPlatform::instance ().kill (-1, "wrong argument to --uid");
      }
    }
        
    std::string path = cmdArgs.front ();

    if (path.substr(std::max(1, (int)path.size())-1) == std::string("3")) {
      vst3host = new Steinberg::Vst::VST3Host::VST3Host(path);
      isvst2 = false;
    } else {
      vst2host = new VST2Host();
      bool tf = vst2host->openVST(path);
      if (!tf) {
        cout << "Presumed vst2 player cannot be opened: " << path << endl;
      }
      isvst2 = true;
    }
        
    openEditor( std::move (uid));
  }
      
  //------------------------------------------------------------------------
  void App::openEditor (VST3::Optional<VST3::UID> effectID)
  {
    std::string error;

    if (isVst2()) {
      vst2host->openEditWindow();
    } else {
      vst3host->createViewAndShow ();
    }
  }
      
  //------------------------------------------------------------------------
  void App::vst2_test(bool doPrint) {
    int numEvents = 12;
    int numSeconds = 5;
    MidiEvent ** midiEvents = (MidiEvent **)malloc(numEvents * sizeof(MidiEvent *));
    //MidiEvent* sustain = simpleMidiNode(0xb0, 0, 0x40, 0x7f, 0);   // sustain pedal down
    midiEvents[0] = new MidiEvent(0x90, 0, 0x3c, 0x46, 0);   // key 60 == middle C  2 beats
    midiEvents[1] = new MidiEvent(0x90, 0, 0x40, 0x50, 0);   // key 60 == middle C 2 beats
    midiEvents[2] = new MidiEvent(0x80, 0, 0x3c, 0, 1 * 44100);
    midiEvents[3] = new MidiEvent(0x80, 0, 0x40, 0, 1 * 44100);
        
    midiEvents[4]  = new MidiEvent(0x90, 0, 0x3e, 0x28, 1 * 44100);   // key 62 == middle d  1 beat
    midiEvents[5]  = new MidiEvent(0x90, 0, 0x43, 0x3c, 1 * 44100);   // key 62 == middle d 2 beats
    midiEvents[6] = new MidiEvent(0x80, 0, 0x3e, 0, 2 * 44100);
    midiEvents[7] = new MidiEvent(0x80, 0, 0x43, 0, 2 * 44100);
    midiEvents[8] = new MidiEvent(0x90, 0, 0x43, 0x3c, 2 * 44100);   // key 62 == middle d 2 beats
    midiEvents[8] = new MidiEvent(0x90, 0, 0x43, 0x3c, 2 * 44100);   // key 62 == middle d 2 beats
    midiEvents[9] = new MidiEvent(0x80, 0, 0x48, 0x41, 2 * 44100);
        
    midiEvents[10] = new MidiEvent(0x80, 0, 0x43, 0, 4 * 44100);
    midiEvents[11] = new MidiEvent(0x80, 0, 0x48, 0, 4 * 44100);
        
    VstEvents* retEvents = VST2Host::convertToVstMidi(midiEvents, numEvents);
        
    list<VstMidiEvent *> *eventsList = new list<VstMidiEvent *>();
    for (int i = 0; i < retEvents->numEvents; ++i){
      eventsList->push_back((VstMidiEvent *)retEvents->events[i]);
    }
        
    this->getVST2Host()->feedEvents(*eventsList);
        
    double *lbuffer = NULL;
    double *rbuffer = NULL;
    long numFrames = 0;
    this->getVST2Host()->processEvents(numSeconds * 1000, 0, &lbuffer, &rbuffer, numFrames);
        
    if (doPrint) {
      cout << "numFrames=" << numFrames << endl;
      for (int i=0; i < numFrames; i++) {
        cout << "[" << i << "]  " << lbuffer[i] << "  " << rbuffer[i] << endl;
      }
    }
        
    static const int MAX_MESSAGE_SIZE = 133;
    char message_buffer[MAX_MESSAGE_SIZE];
    sprintf(message_buffer, "ProcessEvents(): number of frames = %ld\n", numFrames);
    cout << message_buffer;
        
    delete [] lbuffer;
    delete [] rbuffer;
  }
      
  //------------------------------------------------------------------------
  void App::vst3_test(bool print_output) {
    if (isVst2()) {
      cout << "Cannot run VST3 test with VST2 model." << endl;
      return;
    }
        
    int n = (int)vst3host->getSampleRate();
        
    using namespace Steinberg::Vst::VST3Host;
        
    vector<EventRepresentation *> events;
        
    events.push_back(new EventRepresentation{kProgramChangeStatus, 0, 22, 80, 0});  //22   EW 40==E:1
    events.push_back(new EventRepresentation{0x90, 0, 72, 80, 2047});
    events.push_back(new EventRepresentation{0x80, 0, 72, 0, n * 1});
    events.push_back(new EventRepresentation{0x90, 0, 60, 80, n * 1});
    events.push_back(new EventRepresentation{0x80, 0, 60, 0, n * 2});
    events.push_back(new EventRepresentation{0x90, 0, 61, 100, n * 2});
    events.push_back(new EventRepresentation{0x80, 0, 61, 0, n * 3});
    events.push_back(new EventRepresentation{0x90, 0, 62, 120, n * 3});
    events.push_back(new EventRepresentation{0x80, 0, 62, 0, n * 4});
    events.push_back(new EventRepresentation{0x90, 0, 63, 100, n * 4});
    events.push_back(new EventRepresentation{0x80, 0, 63, 0, n * 5});
    events.push_back(new EventRepresentation{0x90, 0, 64, 100, n * 5});
    events.push_back(new EventRepresentation{0x80, 0, 64, 0, n * 6});
        
    vst3host->feedEvents(events);
        
    long numSamplesProduced = 0;
    float* buffer1 = nullptr;
    float* buffer2 = nullptr;
    vst3host->processEvents(6500, 0, &buffer1, &buffer2, numSamplesProduced);
        
    if (print_output) {
      cout << "numSamplesProduced=" << numSamplesProduced << endl;
      for (int i=0; i < numSamplesProduced; i++) {
        cout << "[" << i << "]  " << buffer1[i] << "  " << buffer2[i] << endl;
      }
    }
        
    delete [] buffer1;
    delete [] buffer2;
  }

  //------------------------------------------------------------------------
} // VstPlayerTestApp
} // Vst
} // Steinberg

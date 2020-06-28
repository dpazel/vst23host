#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <sys/time.h>
#include <pthread.h>
#include <cmath>

#include <AudioUnit/AudioUnit.h>

#include "vst2host.h"
#include "hostutil.h"

extern "C" {
  class AEffect;
  class AudioEffect;
  class VST2Host;
  extern void * BuildEffEditWindow(int width, int height, AEffect *aeffect, AudioEffect * audioEffect, VST2Host *);

  // Main host callback
  VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value,
                                     void *ptr, float opt);
}
typedef AEffect *(*vstPluginFuncPtr)(VstIntPtr (*audioMaster)(AEffect *effect,
                                                              VstInt32 opcode,
                                                              VstInt32 index,
                                                              VstInt32 value,
                                                              void *ptr,
                                                              float opt));

typedef enum {
  PLUGIN_TYPE_UNKNOWN,
  PLUGIN_TYPE_UNSUPPORTED,
  PLUGIN_TYPE_EFFECT,
  PLUGIN_TYPE_INSTRUMENT,
  NUM_PLUGIN_TYPES
} PluginType;

const int kBlockSize = 1024;
const int kVSTBlockSize = 512;
const float kSampleRate = 44100.0;
const double kSamplesPerMillisecond = 44.1000000000;

static const int MAX_MESSAGE_SIZE = 256;

/* plugCanDos strings Host -> Plug-in */
namespace PlugCanDos
{
  const char* canDoSendVstEvents = "sendVstEvents"; ///< plug-in will send Vst events to Host
  const char* canDoSendVstMidiEvent = "sendVstMidiEvent"; ///< plug-in will send MIDI events to Host
  const char* canDoReceiveVstEvents = "receiveVstEvents"; ///< plug-in can receive MIDI events from Host
  const char* canDoReceiveVstMidiEvent = "receiveVstMidiEvent"; ///< plug-in can receive MIDI events from Host
  const char* canDoReceiveVstTimeInfo = "receiveVstTimeInfo"; ///< plug-in can receive Time info from Host
  const char* canDoOffline = "offline"; ///< plug-in supports offline functions (#offlineNotify, #offlinePrepare, #offlineRun)
  const char* canDoMidiProgramNames = "midiProgramNames"; ///< plug-in supports function #getMidiProgramName ()
  const char* canDoBypass = "bypass"; ///< plug-in supports function #setBypass ()
}

//------------------------------------------------------------------------
VST2Host::VST2Host() : plugin(NULL), vstLocation(""),
                       inbase(NULL), outbase(NULL), render_state(INACTIVE), event_base_count(0) {
  log("VST2Host construction");
}

//------------------------------------------------------------------------
VST2Host::~VST2Host() {
	 log("VST2Host Closing");

  // Free the render buffers.
  if (inbase != NULL) {
    for (int i = 0; i < plugin->numInputs; ++i) {
      delete inbase[i];
    }
    delete inbase;
  }
  if (outbase != NULL) {
    for (int i = 0; i < plugin->numOutputs; ++i) {
      delete outbase[i];
    }
    delete outbase;
  }
  
  if (plugin != NULL) {
    plugin->dispatcher(plugin, effStopProcess, 0, 0, 0, 0);
    plugin->dispatcher(plugin, effClose, 0, 0, 0, 0);
  }
  
  log("VST2Host closed");
}

//------------------------------------------------------------------------
bool VST2Host::openVST(string locationForVst) {
  if (!locationForVst.empty()) {
    vstLocation = locationForVst;
  }
  log("VST2Host::openVST entering with vst path '%s'.", vstLocation.c_str());
  
  CFStringRef pluginPathStringRef = CFStringCreateWithCString(kCFAllocatorDefault, vstLocation.c_str(), kCFStringEncodingASCII);
  // Convert the path to a URL
  CFURLRef urlRef = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pluginPathStringRef, kCFURLPOSIXPathStyle, true);
  CFRelease(pluginPathStringRef);
  if (urlRef == NULL) {
    log("VST2Host::openVST - urlRef is null.");
    return false;
  }
  
  // Create the bundle using the URL
  CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorDefault, urlRef);
  CFRelease(urlRef);
  // Bail if the bundle wasn't created
  if (bundleRef == NULL) {
    log("VST2Host::openVST - bundleRef is null.");
    return false;
  }

  vstPluginFuncPtr mainEntryPoint = NULL;
  mainEntryPoint = (vstPluginFuncPtr)CFBundleGetFunctionPointerForName(bundleRef, CFSTR("VSTPluginMain"));
  if (mainEntryPoint == NULL) {
      mainEntryPoint = (vstPluginFuncPtr)CFBundleGetFunctionPointerForName(bundleRef, CFSTR("main_macho"));
  }
  
  // This must not be a VST2 plugin
  if (mainEntryPoint == NULL) {
    CFRelease(bundleRef);
    log("VST2Host::openVST - No main entry point found in plugin.");
    return false;
  }
  
  // Initialize the plugin
  plugin = mainEntryPoint(hostCallback);
  
  if (plugin) {
    log("VST2Host::openVST - plugin returned by VST component.");
    plugin->dispatcher(plugin, effOpen, 0, 0, 0, 0);
    plugin->dispatcher(plugin, effSetSampleRate, 0, 0, 0, kSampleRate);
	   plugin->dispatcher(plugin, effSetBlockSize, 0, kVSTBlockSize, 0, 0);
	   plugin->dispatcher(plugin, effCanBeAutomated, 1, 0, 0, 0); // doesn't seem very useful
    
	   audioEffect = (AudioEffect *)plugin->object;
    plugin->user = this;
  } else {
    log("VST2Host::openVST - plugin not returned by VST component.");
    CFRelease(bundleRef);
    return false;
  }
  
  CFRelease(bundleRef);
  
  if (plugin->flags & effFlagsCanReplacing) {
    log("VST2Host::openVST REPLACING VALID.");
    canDoReplacing = true;
  } else {
    log("VST2Host::openVST REPLACING NOT VALID.");
    canDoReplacing = false;
  }
  
  if (plugin->flags & effFlagsCanDoubleReplacing) {
    log("VST2Host::openVST DOUBLE REPLACING VALID.");
    canDoDoubleReplacing = true;
  } else {
    log("VST2Host::openVST DOUBLE REPLACING NOT VALID.");
    canDoDoubleReplacing = false;
  }
  
  if (getAudioEffect()->canHostDo((char *)"receiveVstEvents") == 1) {
    log("VST2Host::openVST Can receive VST Events.");
  }
  if (getAudioEffect()->canHostDo((char *)"receiveVstMidiEvent") == 1) {
    log("VST2Host::openVST Can receive VST MIDI Events.");
  }
  
  // These buffers are used in the rendering process.  Allocate once only.
  inbase = (float **) malloc(plugin->numInputs * sizeof(float *));
  for (int i = 0; i < plugin->numInputs; ++i) {
    inbase[i] = (float *)malloc(kBlockSize * sizeof (float));
  }
  outbase = (float **)malloc(std::max(16, plugin->numOutputs) * sizeof(float *));
  for (int i = 0; i < std::max(16, plugin->numOutputs); ++i) {
    outbase[i] = (float *)malloc(kBlockSize * sizeof(float));
  }
  cleanRenderBuffers();
  
  log("VST2Host::openVST exiting.");
  
  return plugin != NULL;
}

//------------------------------------------------------------------------
extern "C" {
  VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, VstInt32 opcode, VstInt32 index, VstInt32 value, void *dataPtr, float opt) {
    VstIntPtr result = 0;
    
    //log("VST2Host:: Plugin called host dispatcher with %ld, %ld, %ld.", opcode, index, value);
    
    switch(opcode) {
      case audioMasterVersion:
        // VST 2.4 compatible
        // log("HostCallback - audioMasterVersion");
        return 2400;
        
      case audioMasterGetCurrentProcessLevel:
        //log("HostCallback - audioMasterGetCurrentProcessLevel");
        return kVstProcessLevelOffline;
        
      case audioMasterCurrentId:
        // log("HostCallback - audioMasterCurrentId");
        return 0;
        
      case audioMasterCanDo:
      {
        log("HostCallback - audioMasterCanDo");
        static const char* canDos[] = {
          "supplyIdle",
          "sendVstEvents",
          "sendVstMidiEvent",
          "sendVstTimeInfo",
          "receiveVstEvents",
          "receiveVstMidiEvent",
          "openFileSelector",
          "closeFileSelector",
          "receiveVstMidiEvent",
        };
        
        for (int i = 0; i < 8; ++i) {
          if (strcmp (canDos[i], (const char*) dataPtr) == 0) {
            return 1;
          }
        }
        
        return 0;
      }
        
      case audioMasterGetAutomationState:
        //0: not supported
        //1: off
        //2:read
        //3:write
        //4:read/write
        // log("HostCallback - audioMasterGetAutomationState");
        return 1;
        
      case audioMasterGetTime:
      {
        // log("HostCallback - audioMasterGetTime");
        VstTimeInfo * vstTimeInfo = (((VST2Host *)(effect->user))->getTimeInfo());      //(VstTimeInfo *)malloc(sizeof(VstTimeInfo));
        vstTimeInfo->flags = 0;
        vstTimeInfo->sampleRate = ((VST2Host *)(effect->user))->getCurrentSample();
        vstTimeInfo->samplePos = ((VST2Host *)(effect->user))->getCurrentSample();
        return reinterpret_cast<VstIntPtr>(vstTimeInfo);
      }
        
      case audioMasterGetLanguage:
        // log("HostCallback - audioMasterGetLanguage");
        return kVstLangEnglish;
        
      case audioMasterGetSampleRate:
        // log("HostCallback - audioMasterGetSampleRate");
        return ((VST2Host *)(effect->user))->getCurrentSample();
        
      case audioMasterGetBlockSize:
        // log("HostCallback - audioMasterGetBlockSize");
        return kBlockSize;
        
      case audioMasterProcessEvents:
      {
        // Plugin sents vst events to host
        log("HostCallback - audioMasterProcessEvents");
        VstEvents *vstes = (VstEvents *)dataPtr;
        return 0;
      }
        
      case audioMasterIdle:
      {
        // log("HostCallback - audioMasterIdle");
        return 1;
      }
        
      case 6:
        //((VST2Host *)(effect->user))->getPlugin()->
        //get_info()->n_inputs.set_midi (1);
        return 0;
        
      default:
        log("HostCallback - unrecognized opcode %d", opcode);
        break;
    }
    
    return result;
  }
}

//------------------------------------------------------------------------
void VST2Host::openEditWindow() {
  if ((plugin->flags & effFlagsHasEditor) == 0) {
    log("VST2Host::openEditWindow - Error: Plugin does not have an editor.");
    return;
  }
  
  ERect* eRect = 0;
  plugin->dispatcher (plugin, effEditGetRect, 0, 0, &eRect, 0);
  log("VST2Host::openEditWindow - Window (b,t,l,r) = (%d, %d, %d, %d)",
          eRect->bottom, eRect->top, eRect->left, eRect->right);
  
  void * cocoaWindow = BuildEffEditWindow(eRect->right - eRect->left, eRect->bottom - eRect->top,
                                          plugin, audioEffect, this);
  
  log("VST2Host::openEditWindow - calling effEditOpen.");
  VstIntPtr returnCode = audioEffect->dispatcher (effEditOpen, 0l, 0l, cocoaWindow, 0.0); //parent_window
  
  log("VST2Host::openEditWindow - Open editor return code %lld...", returnCode);
}

//------------------------------------------------------------------------
bool canPluginDo(AEffect *plugin, char *canDoString) {
  return (plugin->dispatcher(plugin, effCanDo, 0, 0, (void*)canDoString, 0.0f) > 0);
}

//------------------------------------------------------------------------
struct VstEvents* VST2Host::convertToVstMidi(MidiEvent** midiEvents, int numEvents) {
  int size = sizeof(struct VstEvents) + ((numEvents - 1) * sizeof(struct VstEvent*));
  struct VstEvents* vstMidiEvents = (struct VstEvents*)malloc(size);
  memset(vstMidiEvents, 0, size);
  
  vstMidiEvents->numEvents = numEvents;
  
  for (int i = 0; i < numEvents; ++i) {
    MidiEvent * midiEvent = midiEvents[i];
    struct VstMidiEvent* vstMidiEvent = (struct VstMidiEvent*)malloc(sizeof(VstMidiEvent));
    memset(vstMidiEvent, 0, sizeof(VstMidiEvent));
    vstMidiEvents->events[i] = (VstEvent *)vstMidiEvent;
    switch(midiEvent->eventType) {
      case MIDI_TYPE_REGULAR:
        vstMidiEvent->type = kVstMidiType;
        vstMidiEvent->byteSize = sizeof(VstMidiEvent);
        vstMidiEvent->deltaFrames = (VstInt32)midiEvent->deltaFrames;
        vstMidiEvent->midiData[0] = midiEvent->status;
        vstMidiEvent->midiData[1] = midiEvent->data1;
        vstMidiEvent->midiData[2] = midiEvent->data2;
        //vstMidiEvent->flags = kVstMidiEventIsRealtime;
        vstMidiEvent->reserved1 = 0;
        vstMidiEvent->reserved2 = 0;
        //vstMidiEvent->noteLength = 44100;
        //printf("vstmidievent status=%x deltaframes=%d\n", vstMidiEvent->midiData[0], vstMidiEvent->deltaFrames);
        break;
      case MIDI_TYPE_SYSEX:
        log("VST2Host::convertToVstMidi() VST2.x plugin sysex messages");
        break;
      case MIDI_TYPE_META:
        // Ignore, don't care
        break;
      default:
        log("VST2Host::convertToVstMidi Cannot convert MIDI event type '%d' to VstMidiEvent.", midiEvent->eventType);
        break;
    }
  }
  
  return vstMidiEvents;
}

//------------------------------------------------------------------------
bool VST2Host::beginEventRendering() {
  if (render_state == ACTIVE) {
    log("VST2Host::beginEventRendering - rendering already active.");
    return false;
  }
  plugin->dispatcher(plugin, effMainsChanged, 0, 1, 0, 0); // resume
  plugin->dispatcher(plugin, effStartProcess, 0, 0, 0, 0);
  deallocateCurrentList();
  event_base_count = 0;
  
  render_state = ACTIVE;
  return true;
}

//------------------------------------------------------------------------
bool VST2Host::endEventRendering() {
  if (render_state == INACTIVE) {
    log("VSTXInterface::endEventRendering - rendering already inactive.");
    return false;
  }
  plugin->dispatcher(plugin, effStopProcess, 0, 0, NULL, 0.0);
  plugin->dispatcher(plugin, effMainsChanged, 0, 0, 0, 0); // stop
  
  deallocateCurrentList();
  deallocateDiscardList();
  
  render_state = INACTIVE;
  return true;
}

//------------------------------------------------------------------------
void VST2Host::deallocateCurrentList() {
  if (!current_events.empty()) {
    list<VstMidiEvent *>::iterator oIter = current_events.begin();
    while (oIter != current_events.end()) {
      delete *oIter;
      ++oIter;
    }
    current_events.clear();
  }
  return;
}

//------------------------------------------------------------------------
void VST2Host::deallocateDiscardList() {
  if (!discard_list.empty()) {
    list<VstMidiEvent *>::iterator oIter = discard_list.begin();
    while (oIter != discard_list.end()) {
      delete *oIter;
      ++oIter;
    }
    discard_list.clear();
  }
  return;
}

//------------------------------------------------------------------------
void VST2Host::feedEvents(list<VstMidiEvent *> &events) {
  // append the events to the end.
  current_events.splice(current_events.end(), events);
}

//------------------------------------------------------------------------
long VST2Host::numberEventsInProcess() {
  return current_events.size();
}

//------------------------------------------------------------------------
void VST2Host::cleanRenderBuffers() {
  for (int i = 0; i < plugin->numInputs; ++i) {
    memset(inbase[i], 0, kBlockSize * sizeof(float));
  }
  for (int i = 0; i < plugin->numOutputs; ++i) {
    memset(outbase[i], 0, kBlockSize * sizeof(float));
  }
}

//------------------------------------------------------------------------
bool VST2Host::processEvents(int playTimeInMillisec, long frameBaseLine,
                             double ** leftBuffer, double ** rightBuffer, long &numSamplesProduced) {
  int totalSamples = (playTimeInMillisec * kSamplesPerMillisecond);  // To catch the note off
  long endFrame = frameBaseLine + totalSamples;
  
  log("VST2Host: processEvents() - entering, numEvent=%ld framebaseline=[%ld, %ld] playTimeInMillisec=%d", current_events.size(),
          frameBaseLine, endFrame, playTimeInMillisec);
  
  //  Counts the samples produced.
  current_sample = frameBaseLine;
  
  //  Number of blocks we intend to process.
  int numBlocks = totalSamples / kBlockSize + (totalSamples % kBlockSize ? 1 : 0);
  log("VST2Host::processEvents() - blocks=%d totalSamples=%d blockSize=%d", numBlocks, totalSamples, kBlockSize);

  // Allocate return buffers.
  double * lbuffer = *leftBuffer = (double *)malloc(totalSamples * sizeof(double));
  memset(lbuffer, 0, totalSamples * sizeof(double));
  double * rbuffer = *rightBuffer = (double *)malloc(totalSamples * sizeof(double));
  memset(rbuffer, 0, totalSamples * sizeof(double));
  
  int numEventsProcessed = 0;
  try {
    // Loop over rendering cycles.
    for (int counter = 0; counter < numBlocks; ++counter) {
      log("VST2Host::processEvents() - starting block %d at sample %ld", counter, current_sample);
      
      int sample_to_produce_this_cycle = std::min(totalSamples - counter * kBlockSize, kBlockSize);
      long cycle_base_index = current_sample;
      
      // Count the number of events we will use for this rendering cycle.
      int event_count = 0;
      for (std::list<VstMidiEvent *>::const_iterator iterator = current_events.begin(), end = current_events.end(); iterator != end; ++iterator) {
        if ((*iterator)->deltaFrames >= event_base_count + cycle_base_index && (*iterator)->deltaFrames < event_base_count + cycle_base_index + sample_to_produce_this_cycle) {
          event_count++;
        }
      }
      
      if (event_count != 0) {
        //  This structure is used to pass to effProcessEvents
        int vstEventsSize = sizeof(struct VstEvents) + ((event_count - 1) * sizeof(struct VstEvent*));
        struct VstEvents* currentEvents = (struct VstEvents*)malloc(vstEventsSize);
        int index = 0;
        for (std::list<VstMidiEvent *>::const_iterator iterator = current_events.begin(), end = current_events.end(); iterator != end; ++iterator) {
          if ((*iterator)->deltaFrames >= event_base_count + cycle_base_index && (*iterator)->deltaFrames < event_base_count + cycle_base_index + sample_to_produce_this_cycle) {
            currentEvents->events[index++] = (VstEvent *)*iterator;
          } else {
            //log("VST2Host:: EVENT NOT DIPATCHED deltaFrames=%d event_base_count=%ld cycle_base_inde=%ld sample_to_produce_this_cycle=%d\n", (*iterator)->deltaFrames, event_base_count, cycle_base_index, sample_to_produce_this_cycle);
          }
        }
        currentEvents->numEvents = event_count;
        numEventsProcessed += event_count;
        
        // Remove these events from the master list, current_events, to keep it current
        for (int i = 0; i < event_count; ++i) {
          current_events.remove((VstMidiEvent *)currentEvents->events[i]);
        }
      
        for (int i = 0; i < event_count; ++i) {
          if (currentEvents->events[i]->type == kVstSysExType) {
            log("VST2Host:: DISPATCHING VST SYSEX EVENT.");
            VstMidiSysexEvent* vevent = (VstMidiSysexEvent *)(currentEvents->events[i]);
            char * data = vevent->sysexDump;
            log("VST2Host:: VSTSYSEX 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x (%d) --> %d", (unsigned char)data[0],
                    data[1], data[2], data[3], data[4], data[5], data[6], (unsigned char)data[7], vevent->dumpBytes, vevent->deltaFrames);
            log("     size of ptr = %lu", sizeof(vevent->resvd1));
            break;
          }
        }
        log("VST2Host::processEvents() - Dispatching %d events", currentEvents->numEvents);

        plugin->dispatcher(plugin, effProcessEvents, 0, 0, currentEvents, 0.0);
      }
      
      cleanRenderBuffers();
      //cout << "B sample_to_produce_this_cycle=" << sample_to_produce_this_cycle << endl;
      //plugin->processDoubleReplacing(plugin, inbase, outbase, sample_to_produce_this_cycle);
      log("VST2Host::processEvents - call processReplacing on block=%d sample_to_produce=%d", counter, sample_to_produce_this_cycle);
      
      plugin->processReplacing(plugin, inbase, outbase, sample_to_produce_this_cycle);
      
      log("VST2Host::processEvents - finished processReplacing");
      
      for (int i = 0; i < sample_to_produce_this_cycle; ++i) {
        double lvalue = outbase[0][i];
        lbuffer[current_sample - frameBaseLine] = lvalue;
        double rvalue = outbase[1][i];
        rbuffer[current_sample - frameBaseLine] = rvalue;
        current_sample++;
      }
      
      // We should deallocate some number of VstMidiEvents, no?  which ones?
    }
    numSamplesProduced = current_sample - frameBaseLine;
  } catch(exception& e) {
    string error = string("VST2Host::processEvents() - Exception processing midi events: ") + e.what();
    log("%s", error.c_str());
    return false;
  }
  
  log("VST2Host::processEvents - exiting processed %d events.", numEventsProcessed);
  return true;
}

//------------------------------------------------------------------------
bool VST2Host::loadBank(string fileName) {
  log("VST2Host::loadBank - loading file %s", fileName.c_str());
  
  ifstream file (fileName.c_str(), ios::in|ios::binary|ios::ate);
  if (!file || !file.good()) {
    log("VST2Host::loadBank - file is not good: %s", fileName.c_str());
    return false;
  }
  int size = (int)file.tellg();
  char * memblock = new char [size];
  file.seekg (0, ios::beg);
  file.read (memblock, size);
  file.close();
  
  VstPatchChunkInfo* pci = (VstPatchChunkInfo *)malloc(sizeof(VstPatchChunkInfo));
  memset(pci, 0, sizeof(VstPatchChunkInfo));
  pci->version = 1;
  pci->pluginUniqueID = plugin->uniqueID;
  pci->pluginVersion = plugin->version;
  pci->numElements = 0;
  
  VstInt32 rc;
  
  rc = getAudioEffect()->beginLoadBank(pci);
  log("VST2Host::loadBank - vst load bank rc = %d\n", rc);
  
  rc = getAudioEffect()->setChunk((void *)memblock, size, false);
  
  log("VST2Host::loadBank - setChunk rc=%d bytes=%d.", rc, size);

  return true;
}

//------------------------------------------------------------------------
bool VST2Host::setBank(int length, void *bankData) {
  log("VST2Host::setBank - setting bank information");

  VstPatchChunkInfo* pci = (VstPatchChunkInfo *)malloc(sizeof(VstPatchChunkInfo));
  memset(pci, 0, sizeof(VstPatchChunkInfo));
  pci->version = 1;
  pci->pluginUniqueID = plugin->uniqueID;
  pci->pluginVersion = plugin->version;
  pci->numElements = 0;
  
  VstInt32 rc1;
  
  rc1 = getAudioEffect()->beginLoadBank(pci);
  log("VST2Host:loadBank - vst set bank rc = %d", rc1);
  
  VstInt32 rc = getAudioEffect()->setChunk((void *)bankData, length, false);
  log("VST2Host::setBank rc = %d", rc);
  
  return true;
}

//------------------------------------------------------------------------
bool VST2Host::saveBank(string fileName) {
  void * data;
  int len = getAudioEffect()->getChunk(&data, false);
  
  log("VST2Host::saveBank - obtained %d bytes to save.", len);
  
  ofstream ofile (fileName.c_str(), ios::out|ios::binary|ios::ate);
  if (!ofile || !ofile.good()) {
    log("VST2Host::saveBank - file is not good: %s", fileName.c_str());
    return false;
  }
  ofile.write((char *)data, len);
  ofile.close();
  return true;
}

//------------------------------------------------------------------------
bool VST2Host::getBank(int &dataLength, void ** returnData) {
  void * data;
  int len = getAudioEffect()->getChunk(&data, false);
  
  log("VST2Host::getBank - obtained %d bytes or bank data.", len);
  
  dataLength = len;
  *returnData = data;
  
  return true;
}

//------------------------------------------------------------------------
void VST2Host::closeEditWindow() {
	 log("VST2Host::closeEditWindow - Closing plugin edit window.");
	 plugin->dispatcher(plugin, effEditClose, 0, 0, NULL, 0.0);
	 log("VST2Host::closeEditWindow - Finished closing plugin edit window with effEditClose.");
}

//------------------------------------------------------------------------
void VST2Host::displayData() {
  printf("----------  DISPLAY DATA  ----------\n");
  AudioEffectX *audioEffectX = getAudioEffect();
  
  char message[MAX_MESSAGE_SIZE];
  memset(message, 0, MAX_MESSAGE_SIZE);
  
  PluginType pluginType;
  if (plugin->flags & effFlagsIsSynth) {
    pluginType = PLUGIN_TYPE_INSTRUMENT;
  }
  else {
    pluginType = PLUGIN_TYPE_EFFECT;
  }
  
  bool isPluginShell;
  VstInt32 shellPluginId = 0;
  if (plugin->dispatcher(plugin, effGetPlugCategory, 0, 0, NULL, 0.0f) == kPlugCategShell) {
    shellPluginId = (VstInt32)plugin->dispatcher(plugin, effShellGetNextPlugin, 0, 0, message, 0.0f);
    printf("VST is a shell plugin, sub-plugin ID '%d'", shellPluginId);
    isPluginShell = true;
  }
  
  memset(message, 0, MAX_MESSAGE_SIZE);
  audioEffectX->getEffectName(message);
  printf("Plugin Name: %s\n", message);
  
  memset(message, 0, MAX_MESSAGE_SIZE);
  audioEffectX->getProductString(message);
  printf("Product: %s\n", message);
  
  memset(message, 0, MAX_MESSAGE_SIZE);
  audioEffectX->getVendorString(message);
  printf("Vendor: %s\n", message);
  
  printf("Vendor Version: %d\n", audioEffectX->getVendorVersion());
  
  VstInt32 pluginCategory = (VstInt32)plugin->dispatcher(plugin, effGetPlugCategory, 0, 0, NULL, 0.0f);
  switch(pluginType) {
    case PLUGIN_TYPE_EFFECT:
      printf("Plugin type: effect, category %d\n", pluginCategory);
      break;
    case PLUGIN_TYPE_INSTRUMENT:
      printf("Plugin type: instrument, category %d\n", pluginCategory);
      break;
    default:
      printf("Plugin type: other, category %d\n", pluginCategory);
      break;
  }
  
  printf("Plugin ID: %d\n", plugin->uniqueID);
  
  printf("Plugin Version: %d\n", plugin->version);
  printf("I/O: %d/%d\n", plugin->numInputs, plugin->numOutputs);
  
  printf("VST Version: %d\n", audioEffectX->getMasterVersion()); // 2400 called host callback
  
  VstInt32 blocksize = audioEffectX->getBlockSize();
  printf("Block Size: %d\n", blocksize);
  
  printf("Parameter Count: %d\n", plugin->numParams);
  for (unsigned int i = 0; i < plugin->numParams; i++) {
    float value = plugin->getParameter(plugin, i);
    memset(message, 0, MAX_MESSAGE_SIZE);
    plugin->dispatcher(plugin, effGetParamName, i, 0, message, 0.0f);
    printf("  %d: '%s' (%f)\n", i, message, value);
  }
  
  printf("Program Count: %d\n", plugin->numPrograms);
  for (int i = 0; i < plugin->numPrograms; i++) {
    memset(message, 0, MAX_MESSAGE_SIZE);
    plugin->dispatcher(plugin, effGetProgramNameIndexed, i, 0, message, 0.0f);
    printf("  %d: '%s'\n", i, message);
  }
  printf("Current program id: %d\n", audioEffectX->getProgram());
  memset(message, 0, MAX_MESSAGE_SIZE);
  audioEffectX->getProgramName(message);
  printf("Current program name: '%s'\n", message);
  
  bool cando = canPluginDo(plugin, (char *)PlugCanDos::canDoReceiveVstEvents);
  
  log("VST2Host::Plugin can receive vst events: %s", (cando ? "yes" : "no"));
  
  cando = (plugin->flags & effFlagsProgramChunks) == effFlagsProgramChunks;
  log("VST2Host::Plugin processes programs in chunks: %s\n", cando ? "yes" : "no");
  
  cando = canPluginDo(plugin, (char *)PlugCanDos::canDoReceiveVstMidiEvent);
  log("VS2Host::Plugin can receive vst midi events: %s\n", cando ? "yes" : "no");
  
  memset(message, 0, MAX_MESSAGE_SIZE);
  
  AEffEditor* editor = audioEffectX->getEditor ();
  if (editor != NULL) {
    printf("We have an editor\n");
  } else {
    printf("We do not have an editor\n");
  }
  
  printf("-- audioMasterAutomate = %d\n", audioMasterAutomate);
  printf("-- audioMasterVersion = %d\n", audioMasterVersion);
  printf("-- audioMasterCurrentId = %d\n", audioMasterCurrentId);
  printf("-- audioMasterIdle = %d\n", audioMasterIdle);
  printf("-- audioMasterGetTime = %d\n", audioMasterGetTime);
  printf("-- audioMasterProcessEvents = %d\n", audioMasterProcessEvents);
  printf("-- audioMasterIOChanged = %d\n", audioMasterIOChanged);
  printf("-- audioMasterGetCurrentProcessLevel = %d\n", audioMasterGetCurrentProcessLevel);
  printf("-- audioMasterGetVendorString = %d\n", audioMasterGetVendorString);
  printf("-- audioMasterGetSampleRate = %d\n", audioMasterGetSampleRate);
  
  printf("----------  END DISPLAY DATA  ----------\n");
}

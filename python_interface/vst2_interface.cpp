#include "vst2_interface.hpp"
#include <iostream>

#include "../vst23host/hostutil.h"

using namespace std;

VST2Host* vstxinterface = nullptr;

extern "C" void connect_vst2(char * path)
{
  log("vst2_interface::connect_vst2() path[%s]", path);
  
  vstxinterface  = new VST2Host();
  bool tf = vstxinterface->openVST(path);
  if (!tf) {
    log("vst2_interface::connect_vst2() - Error opening vst.");
    return;
  }
  
  log("vst2_interface::connect_vst2() exit");
}

extern "C" void view_and_show2() {
  
  log("vst2_interface::view_and_show2() ...");
  
  vstxinterface->openEditWindow();
  
  log("vst2_interface::view_and_show2() exiting.");
}

extern "C" void feed_events2(PyEvent* event_array, int32_t num_events) {
  
  log("vst2_interface::FeedEvents2() ...");
  
  list<VstMidiEvent *> eventsList;
  for (int i = 0; i < num_events; ++i) {
      PyEvent message = event_array[i];
      
      VstMidiEvent* vstMidiEvent = (VstMidiEvent *)malloc(sizeof(VstMidiEvent));
      memset(vstMidiEvent, 0, sizeof(VstMidiEvent));
      vstMidiEvent->type = kVstMidiType;
      vstMidiEvent->byteSize = sizeof(VstMidiEvent);
      vstMidiEvent->deltaFrames = (int)message.abs_frame_time;
      vstMidiEvent->midiData[0] = message.msg_type;
      vstMidiEvent->midiData[1] = message.data1;
      vstMidiEvent->midiData[2] = message.data2;
      eventsList.push_back(vstMidiEvent);
    
      //log("   Event[%d] msg_type=%d data1=%d data2=%d deltaFrames=%d", i,
      //    (int)(vstMidiEvent->midiData[0]), (int)(vstMidiEvent->midiData[1]), (int)(vstMidiEvent->midiData[2]), vstMidiEvent->deltaFrames);
  }
  
  vstxinterface->feedEvents(eventsList);
  log("vst2_interface::feed_events2() - exiting.");
  return;
}

extern "C" PyObject* process_events2(int play_time_in_ms) {
  
  log("vst2_interface::process_events2() ...");
  
  double *left_buffer = NULL;
  double *right_buffer = NULL;
  long numFrames = 0;
  bool tf = vstxinterface->processEvents(play_time_in_ms, 0, &left_buffer, &right_buffer, numFrames);
  
  log("vst2_interface::process_events2(): number of frames = %ld.", numFrames);
  
  PyObject* left_array = PyList_New(numFrames);
  PyObject* right_array = PyList_New(numFrames);
  
  for (int i = 0; i < numFrames; ++i) {
    PyObject* item = PyFloat_FromDouble(left_buffer[i]);
    PyList_SetItem(left_array, (Py_ssize_t)i, item);
    item = PyFloat_FromDouble(right_buffer[i]);
    PyList_SetItem(right_array, (Py_ssize_t)i, item);
  }
  
  PyObject* data = Py_BuildValue( "(OO)", left_array, right_array);
  
  delete [] left_buffer;
  delete [] right_buffer;
  
  log("vst2_interface::process_events2 exiting.");
  return data;
}

/*
 * BeginEventRendering() - initialize to rendering audio.
 */
extern "C" bool begin_event_rendering2() {
  
  log("vst2_interface::begin_event_rendering2()");
  
  if (vstxinterface == NULL) {
    log("vst2_interface::begin_event_rendering2() - vst information could not be found.");
    return false;
  }
  
  bool tf = vstxinterface->beginEventRendering();
  
  log("vst2_interface::begin_event_rendering2() - exiting.");
  
  return tf;
}

/*
 * EndEventRendering() - stop rendering audio.
 */
extern "C" bool end_event_rendering2() {
  log("vst2_interface::end_event_rendering2()");
  
  if (vstxinterface == NULL) {
    log("vst2_interface::end_event_rendering2() - vst information could not be found.");
    return false;
  }
  
  bool tf = vstxinterface->endEventRendering();
  
  log("vst2_interface::end_event_rendering2() - exiting.");
  
  return tf;
}

extern "C" void save_bank(char * filename) {
  log("vst2_interface::save_bank()");
  
  if (vstxinterface == NULL) {
    log("vst2_interface::save_bank() - vst information could not be found.");
    return false;
  }
  
  bool tf = vstxinterface->saveBank(filename);
  
  log("vst2_interface::save_bank() - exiting.");
  
  return tf;
}

extern "C" void load_bank(char * filename) {
  log("vst2_interface::load_bank()");
  
  if (vstxinterface == NULL) {
    log("vst2_interface::load_bank() - vst information could not be found.");
    return false;
  }
  
  bool tf = vstxinterface->loadBank(filename);
  
  log("vst2_interface::load_bank() - exiting.");
  
  return tf;
}

extern "C"  void close_vst2() {
  log("vst2_interface::close_vst2()");
  
  if (vstxinterface == NULL) {
    log("vst2_interface::close_vst2() - vst information could not be found.");
    return false;
  }
  
  vstxinterface->closeEditWindow();
  delete vstxinterface;
  vstxinterface = nullptr;
  
  log("vst2_interface::close_vst2() - exiting.");
}

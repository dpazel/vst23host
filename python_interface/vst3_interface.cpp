#include "vst3_interface.hpp"

#include <iostream>
#include<future>
#include <chrono>
#include <thread>

#include "../vst23host/vst3host.hpp"

#include "../vst23host/abstractwindow.h"
#include "../vst23host/hostutil.h"

using namespace std;

Steinberg::Vst::VST3Host::VST3Host* vst3host=nullptr;

extern "C" void connect_vst3(char * path)
{
  log("vst3_interface::connect_vst() path[%s]", path);
  
  vst3host = new Steinberg::Vst::VST3Host::VST3Host(path);
  
  log("vst3_interface::connect_vst() exit");
}

extern "C" void view_and_show()
{
  log("vst3_interface::view_and_show() ...");
  if (vst3host == nullptr) {
    log("vst3_interface::vst not connected. Issue connect_vst.");
    return;
  }
  vst3host->createViewAndShow();
  
  log("vst3_interface::view_and_show() exit");
}

void deferred_delete_VST3Host (Steinberg::Vst::VST3Host::VST3Host*  vst3host)
{
  log("vst3_interface::deferred_delete_VST3Host() - deferred delete vst3host.");
  //std::this_thread::sleep_for(0.5s);
  delete vst3host;
  vst3host = nullptr;
  log("vst3_interface::deferred_delete_VST3Host() - exit.");
}

extern "C" void close_vst()
{
  log("vst3_interface::close_vst() ...");;
  if (vst3host != nullptr) {
    if (vst3host->getWindow()) {
      log("vst3_interface::close_vst(): closing window");
      vst3host->getWindow()->close();
      // Defer execution for deleting vst3host, so that close window can finish.
      std::future<void> v = std::async(std::launch::async, deferred_delete_VST3Host, vst3host);
      
      // We need a way to wait on the deferred delete, so we do not exit python before 
      v.get();  // This is a wait on the thread, wherein the meantime, the close can complete.
    } else {
        delete vst3host;
        vst3host = nullptr;
    }
  }
  log("vst3_interface::close_vst() exit");
}

extern "C" void feed_events(PyEvent* event_array, int32_t num_events)
{
  log("vst3_interface::feed_events() ...");
  if (vst3host == nullptr) {
    log("vst3_interface::feed_events(): vst not connected. Issue connect_vst.");
    return;
  }
  
  vector<Steinberg::Vst::VST3Host::EventRepresentation *> event_list;
  for (int i = 0; i < num_events; ++i) {
    PyEvent message = event_array[i];
    Steinberg::Vst::VST3Host::EventRepresentation* event =
      new Steinberg::Vst::VST3Host::EventRepresentation(message.msg_type,
                                                        message.channel,
                                                        message.data1,
                                                        message.data2,
                                                        message.abs_frame_time);
    event_list.push_back(event);
  }
  
  vst3host->feedEvents(event_list);
  log("vst3_interface::feed_events() exit");
  return;
}

extern "C" void clear_feed_events() {
  vst3host->clearFeedEvents();
}

extern "C" PyObject* process_events(int play_time_in_ms) {
  log("vst3_interface::process_events() ...");
  if (vst3host == nullptr) {
    cout << "vst3_interface::process_events(): vst not connected. Issue connect_vst." << endl;
    return nullptr;
  }
  
  long numSamplesProduced = 0;
  
  float* left_buffer = nullptr;
  float* right_buffer = nullptr;

  vst3host->processEvents(play_time_in_ms, 0, &left_buffer, &right_buffer, numSamplesProduced);
  
  PyObject* left_array = PyList_New(numSamplesProduced);
  PyObject* right_array = PyList_New(numSamplesProduced);
  
  for (int i = 0; i < numSamplesProduced; ++i) {
    PyObject* item = PyFloat_FromDouble(left_buffer[i]);
    PyList_SetItem(left_array, (Py_ssize_t)i, item);
    item = PyFloat_FromDouble(right_buffer[i]);
    PyList_SetItem(right_array, (Py_ssize_t)i, item);
  }
  
  PyObject* data = Py_BuildValue( "(OO)", left_array, right_array);
  
  delete [] left_buffer;
  delete [] right_buffer;
  
  log("vst3_interface::process_events() exit");
  return data;
}

extern "C" void save_preset(char * fileName){
  log("vst3_interface::save_preset() ..." );
  if (vst3host == nullptr) {
    log("vst3_interface::save_preset(): vst not connected. Issue connect_vst.");
    return nullptr;
  }
  
  log("vst3_interface::save_preset() NOT IMPLEMENTED.");
  
  log("vst3_interface::save_preset() exit ...");
}

extern "C" void load_preset(char * fileName) {
  log("vst3_interface::load_preset() ...");
  if (vst3host == nullptr) {
    log("vst3_interface::load_preset(): vst not connected. Issue connect_vst.");
    return nullptr;
  }
  
  log("vst3_interface::load_preset() NOT IMPLEMENTED.");
  
  log("vst3_interface::load_preset() exit ...");
}

#ifndef vst3_interface_hpp
#define vst3_interface_hpp

#include "Python.h"
#include <stdio.h>

extern "C" {

  typedef struct PyEvent {
    int32_t msg_type;
    int32_t channel;
    int32_t data1;
    int32_t data2;
    int32_t rel_frame_time;
    int32_t abs_frame_time;
  } PyEvent;

  void connect_vst3(char * path);
  void connect_vstview_and_show(char * path);
  void view_and_show();
  void close_vst();
  
  void feed_events(PyEvent* event_array, int32_t num_events);
  void clear_feed_events();
  PyObject* process_events(int play_time_in_ms);
  void save_preset(char * fileName);
  void load_preset(char * fileName);
  
}

#endif /* vst3_interface_hpp */

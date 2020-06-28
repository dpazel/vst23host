#ifndef vst2_interface_hpp
#define vst2_interface_hpp

#include "../vst23host/vst2host.h"

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
  
  void connect_vst2(char * path);
  void view_and_show2();
  void feed_events2(PyEvent* event_array, int32_t num_events);
  void close_vst2();
  PyObject* process_events2(int play_time_in_ms);
  bool begin_event_rendering2();
  bool end_event_rendering2();
  void save_bank(char * filename);
  void load_bank(char * filename);
}
#endif /* vst2_interface_hpp */

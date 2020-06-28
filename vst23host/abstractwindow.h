#ifndef abstractwindow_h
#define abstractwindow_h

#include <string>

#include "iwindow.h"

namespace Steinberg {
namespace Vst {
namespace VST3Host {

/*
  AbstractWindow's role is to maintain a pointer to IWindow which is based indirectly to Window.
  We cannot use Window directly as it explicitly holds a point to NSWindow - which is an ObjectiveC artifact, not a
  C++ artifact. [Note: including window.h directly in C++ has typing issues.]
  */
class AbstractWindow {

public:
  static AbstractWindow* make (const std::string& name,
                               Size size,
                                bool resizeable,
                                const WindowControllerPtr& controller);
  AbstractWindow(WindowPtr w);
  void show () ;
  void close () ;
  void resize (Size newSize);
  Size getContentSize () ;
        
private:
  WindowPtr window;
};

} // VST3Host}
} // Vst
} // Steinberg

#endif /* abstractwindow_h */

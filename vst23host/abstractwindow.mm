#include <string>
#import <Foundation/Foundation.h>

#include "abstractwindow.h"
#include "window.h"
#include "hostutil.h"

namespace Steinberg {
namespace Vst {
namespace VST3Host {
      
//------------------------------------------------------------------------
AbstractWindow* AbstractWindow::make (const std::string& name,
                                      Size size,
                                      bool resizeable,
                                      const WindowControllerPtr& controller)
{
  log("Entering AbstractWindow make.");
  WindowPtr window1 = Window::make(name, size, resizeable, controller);
  log("built window for AbstractWindow::make.");
  AbstractWindow* abstractwindow = new AbstractWindow(window1);
  return abstractwindow;
}
      
//------------------------------------------------------------------------
AbstractWindow::AbstractWindow(WindowPtr w) {
  this->window = w;
}

//------------------------------------------------------------------------
void AbstractWindow::show () {
  window->show();
}

//------------------------------------------------------------------------
void AbstractWindow::close () {
  window->close();
}

//------------------------------------------------------------------------
void AbstractWindow::resize (Size newSize) {
  window->resize(newSize);
}

//------------------------------------------------------------------------
Size AbstractWindow::getContentSize ()  {
  return window->getContentSize();
}
  
} // VST3Host
} // Vst
} // Steinberg

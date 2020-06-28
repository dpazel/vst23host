#include "windowcontroller.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "vst3host.hpp"
#include "hostutil.h"

#include <cstdio>
#include <iostream>

namespace Steinberg {

//------------------------------------------------------------------------
inline bool operator== (const Steinberg::ViewRect& r1, const Steinberg:: ViewRect& r2)
{
  return memcmp (&r1, &r2, sizeof (Steinberg::ViewRect)) == 0;
}

//------------------------------------------------------------------------
inline bool operator!= (const Steinberg::ViewRect& r1, const Steinberg::ViewRect& r2)
{
  return !(r1 == r2);
}

namespace Vst {
namespace VST3Host {

//------------------------------------------------------------------------
WindowController::WindowController (const Steinberg::IPtr<Steinberg::IPlugView>& plugView, VST3Host* vst3hostparam) :
  plugView (plugView), vst3host(vst3hostparam)
{
}

//------------------------------------------------------------------------
WindowController::~WindowController () noexcept
{
}

//------------------------------------------------------------------------
void WindowController::onShow(IWindow& w) {
  window = &w;
  if (!plugView)
    return;
  
  NativePlatformWindow platformWindow = window->getNativePlatformWindow ();
  if (plugView->isPlatformTypeSupported (platformWindow.type) != Steinberg::kResultTrue)
  {
    *(VST3Host::getErrorStream()) << "PlugView does not support platform type:" << platformWindow.type << endl;
    return;
  }

  plugView->setFrame (this);
  log("Windowcontrolle::setFrame inside onShow().");

  if (plugView->attached (platformWindow.ptr, platformWindow.type) != Steinberg::kResultTrue)
  {
    *(VST3Host::getErrorStream()) << "Windowcontroller::onShow attaching plugview failed." << endl;
  }
}

//------------------------------------------------------------------------
void WindowController::onClose (Steinberg::Vst::VST3Host::IWindow& w) {
  if (plugView)
  {
    if (plugView->removed () != Steinberg::kResultTrue)
    {
      *(VST3Host::getErrorStream()) <<  "WindowController::onClose - Removing PlugView failed." << endl;
      return;
    }
    plugView->setFrame (nullptr);

    plugView.reset();
  }
  
  window = nullptr;
  vst3host->setWindow(NULL);
}

//------------------------------------------------------------------------
void WindowController::onResize (Steinberg::Vst::VST3Host::IWindow& w, Steinberg::Vst::VST3Host::Size newSize) {
  if (plugView)
  {
    Steinberg::ViewRect r {};
    r.right = newSize.width;
    r.bottom = newSize.height;
    Steinberg::ViewRect r2 {};
    if (plugView->getSize (&r2) == Steinberg::kResultTrue && r != r2)
      plugView->onSize (&r);
  }
}

//------------------------------------------------------------------------
Steinberg::Vst::VST3Host::Size WindowController::constrainSize (Steinberg::Vst::VST3Host::IWindow& w, Steinberg::Vst::VST3Host::Size requestedSize)
{
  Steinberg::ViewRect r {};
  r.right = requestedSize.width;
  r.bottom = requestedSize.height;
  if (plugView && plugView->checkSizeConstraint (&r) != Steinberg::kResultTrue)
  {
    plugView->getSize (&r);
  }
  requestedSize.width = r.right - r.left;
  requestedSize.height = r.bottom - r.top;
  return requestedSize;
}

//------------------------------------------------------------------------
void WindowController::onContentScaleFactorChanged (Steinberg::Vst::VST3Host::IWindow& window, float newScaleFactor) {
  Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> css (plugView);
  if (css)
  {
    css->setContentScaleFactor (newScaleFactor);
  }
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API WindowController::resizeView (Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) {
  if (newSize == nullptr || view == nullptr || view != plugView)
    return Steinberg::kInvalidArgument;
  if (!window)
    return Steinberg::kInternalError;
  if (resizeViewRecursionGard)
    return Steinberg::kResultFalse;
  Steinberg::ViewRect r;
  if (plugView->getSize (&r) != Steinberg::kResultTrue)
    return Steinberg::kInternalError;
  if (r == *newSize)
    return Steinberg::kResultTrue;
  
  resizeViewRecursionGard = true;
  Steinberg::Vst::VST3Host::Size size {newSize->right - newSize->left, newSize->bottom - newSize->top};
  window->resize (size);
  resizeViewRecursionGard = false;
  if (plugView->getSize (&r) != Steinberg::kResultTrue)
    return Steinberg::kInternalError;
  if (r != *newSize)
    plugView->onSize (newSize);
  return Steinberg::kResultTrue;
}

}
}
}

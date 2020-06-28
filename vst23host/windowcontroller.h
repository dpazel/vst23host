#ifndef windowcontroller_h
#define windowcontroller_h

#include "pluginterfaces/gui/iplugview.h"
#include "iwindow.h"

namespace Steinberg {
namespace Vst {
namespace VST3Host {
      
class VST3Host;

class WindowController : public IWindowController, public IPlugFrame {
public:
  WindowController (const Steinberg::IPtr<Steinberg::IPlugView>& plugView, VST3Host* vst3host);
  ~WindowController () noexcept override;
  
  void onShow (IWindow& w) override;
  void onClose (IWindow& w) override;
  void onResize (IWindow& w, Steinberg::Vst::VST3Host::Size newSize) override;
  Steinberg::Vst::VST3Host::Size constrainSize (Steinberg::Vst::VST3Host::IWindow& w, Steinberg::Vst::VST3Host::Size requestedSize) override;
  void onContentScaleFactorChanged (Steinberg::Vst::VST3Host::IWindow& window, float newScaleFactor) override;
  
  Steinberg::tresult PLUGIN_API resizeView (Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override;
  
private:
  Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID _iid, void** obj) override
  {
    if (Steinberg::FUnknownPrivate::iidEqual (_iid, IPlugFrame::iid) ||
        Steinberg::FUnknownPrivate::iidEqual (_iid, FUnknown::iid))
    {
      *obj = this;
      addRef ();
      return Steinberg::kResultTrue;
    }
    if (window)
      return window->queryInterface (_iid, obj);
    return Steinberg::kNoInterface;
  }
  uint32 PLUGIN_API addRef () override { return 1000; }
  uint32 PLUGIN_API release () override { return 1000; }
  
  IPtr<Steinberg::IPlugView> plugView;
  IWindow* window {nullptr};
  bool resizeViewRecursionGard {false};
  
  VST3Host* vst3host {nullptr};
};
      
}
}
}


#endif /* windowcontroller_h */

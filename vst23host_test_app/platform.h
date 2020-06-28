#ifndef platform_h
#define platform_h

#include "iplatform.h"

namespace Steinberg {
namespace Vst {
namespace VSTPlayerTestApp {
      
//------------------------------------------------------------------------
class Platform : public IPlatform
{
public:

  static Platform& instance ();
        
  Platform ();
  void setApplication (ApplicationPtr&& app) override;
  VST3Host::WindowPtr createWindow (const std::string& title, VST3Host::Size size, bool resizeable,
                                    const VST3Host::WindowControllerPtr& controller) override;
  void quit () override;
  void kill (int resultCode, const std::string& reason) override;
        
  ApplicationPtr application;
  bool quitRequested {false};
};
  
} // VSTPlayerTestApp
} // Vst
} // Steinberg

#endif /* platform_h */

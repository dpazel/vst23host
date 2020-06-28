#pragma once

#include "iapplication.h"
#include "vst/hosting/module.h"
#include "vst/hosting/optional.h"
#include "vst/hosting/plugprovider.h"
#include "vst/hosting/hostclasses.h"

#include "../vst23host/vst3host.hpp"
#include "../vst23host/vst2host.h"

// "/Library/Application Support/Steinberg/Components/HALion Sonic SE.vst3"
// "/Library/Audio/Plug-Ins/VST3/Play.vst3"
// "/Library/Audio/Plug-Ins/VST/Aria Player VST.vst"
// "/Library/Audio/Plug-Ins/VST/Aria Player VST Multi.vst"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace VSTPlayerTestApp {
      
//------------------------------------------------------------------------
class App : public IApplication
{
public:
  ~App () noexcept override;
  void init (const std::vector<std::string>& cmdArgs) override;
  
  VST2Host * getVST2Host() { return vst2host; }
  VST3Host::VST3Host* getVST3Host() { return vst3host.get(); }
  bool isVst2() { return isvst2; }
        
  void vst2_test(bool doPrint);
  void vst3_test(bool doPrint);
        
private:
  enum OpenFlags
  {
    kSetComponentHandler = 1 << 0,
  };
  void openEditor ( VST3::Optional<VST3::UID> effectID);
  void createViewAndShow (IEditController* controller);
        
  VST3::Hosting::Module::Ptr module {nullptr};
  IPtr<PlugProvider> plugProvider {nullptr};
        
  bool isvst2;
  VST2Host * vst2host;
  IPtr<VST3Host::VST3Host> vst3host {nullptr};
};
      
//------------------------------------------------------------------------
} // VSTPlayerTestApp
} // Vst
} // Steinberg

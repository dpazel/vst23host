#pragma once

#include "iapplication.h"
#include "iplatform.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace VSTPlayerTestApp {
      
  //------------------------------------------------------------------------
  struct AppInit
  {
    explicit AppInit (ApplicationPtr&& app)
    {
      IPlatform::instance ().setApplication (std::move (app));
    }
  };
  
} // VSTPlayerTestApp
} // Vst
} // Steinberg

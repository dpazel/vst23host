# vst23host
A small scale vst host accommodating version 2 and 3 of vst.

This is a minimal vst host, that accommodates vst2 and/or vst3, that can be used with the 'music_rep' project on:  www.github.com/dpazel/music_rep

## BUILD:

We only provide a build for macos, and presently uses the base SDK macOS 10.15. Presently there is not makefile, on a version of the xcode project build file.

The build generates 3 binaries:
- vst23host: a library that load the host.
- VSThost_test_app: A C++ app that loads vst23host for testing.
- VSTUnitTest: Should be deprecated or exanded test module.

The build relies on both VST2 and VST3 development kits are accessible to the build. There are 4 user-defined symbols which should be set to specific directories in these kits. Specifically,

- VST2_SDK_PATH: Points to vst3/VST3 SDK 2
- VST2_SDK_PATH_PUBLIC: Points to vst3/VST3 SDK 2/public.sdk/source
- VST3_SDK_PATH: Points to vst3sdk root
- VST3_SDK_PATH_PUBLIC: Points to vst3sdk/public.sdk.source

These must be set correctly for the build to work. These can be found in one place if you can get vstsdk3612_03_12_2018 _build_67.zip.

Also in the header search paths, point to the C++ paths for Python. (We recomment python3 - the checked in code uses python 3.6).  For example, depending on installation, you might have:
.../python3/3.6.1/Frameworks/Python.framework/Versions/3.6/include/python3.6m

If you are using the generated library for 'music_rep', be very certain to include headers from the identical version of Python used there.


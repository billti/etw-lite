# Comment out the below to build with MSVC
CXX="C:\Program Files\LLVM\bin\clang-cl.exe"
CXXFLAGS=/O2 /MD /Zi /std:c++14
LINKFLAGS=/debug /opt:ref,icf

# Include /DNO_ETW to remove ETW from the build
DEFS=/DNDEBUG # /DNO_ETW

APP_CPP=lib/etw-provider.cpp src/app.cpp src/foo-provider.cpp
APP_H=lib/etw-metadata.h lib/etw-provider.h src/foo-provider.h

app.exe: $(APP_CPP) $(APP_H)
	$(CXX) $(CXXFLAGS) $(APP_CPP) advapi32.lib $(DEFS) /link $(LINKFLAGS)

clean:
	del *.obj *.exe *.ilk *.pdb

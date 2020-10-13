# Comment out the below to build with MSVC
CXX="C:\Program Files\LLVM\bin\clang-cl.exe"
CXXFLAGS=/O2 /MD /Zi /std:c++14
LINKFLAGS=/debug /opt:ref,icf
DEFINES=/DNDEBUG

APP_SLIM_CPP=src/app-lean.cpp src/lean-provider.cpp lib/etw-provider.cpp
APP_SLIM_H=lib/etw-metadata.h lib/etw-provider.h src/lean-provider.h

APP_THIC_CPP=src/app-thic.cpp lib/etw-provider.cpp
APP_THIC_H=lib/etw-metadata.h lib/etw-provider.h src/thic-provider.h

all: app-lean.exe app-thic.exe app-none.exe activities.exe

app-lean.exe: $(APP_SLIM_CPP) $(APP_SLIM_H)
	$(CXX) $(CXXFLAGS) $(APP_SLIM_CPP) advapi32.lib $(DEFINES) /link $(LINKFLAGS)

app-thic.exe: $(APP_THIC_CPP) $(APP_THIC_H)
	$(CXX) $(CXXFLAGS) $(APP_THIC_CPP) advapi32.lib $(DEFINES) /link $(LINKFLAGS)

# Build the "lean" provider with NO_ETW defined (i.e. ETW code should get removed)
app-none.exe: $(APP_SLIM_CPP) $(APP_SLIM_H)
	$(CXX) $(CXXFLAGS) $(APP_SLIM_CPP) advapi32.lib $(DEFINES) /DNO_ETW /link $(LINKFLAGS) /OUT:"$@"

activities.exe: src/activities.cpp
	$(CXX) /Od /Zi /MDd /std:c++17 $** advapi32.lib

clean:
	del *.obj *.exe *.ilk *.pdb

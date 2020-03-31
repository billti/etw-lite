# Uncomment the below to build with clang (if LLVM is installed to default dir)
# CXX="C:\Program Files\LLVM\bin\clang-cl.exe"
CXXFLAGS=/O2 /MD /Zi /std:c++14
LINKFLAGS=/debug /opt:ref,icf

# Include /DNO_ETW to remove ETW from the build
DEFS=/DNDEBUG # /DNO_ETW

APP_CPP=app.cpp etw-provider.cpp foo-provider.cpp
APP_H=etw-metadata.h etw-provider.h foo-provider.h

app.exe: $(APP_CPP) $(APP_H)
	$(CXX) $(CXXFLAGS) $(APP_CPP) advapi32.lib $(DEFS) /link $(LINKFLAGS)

clean:
	del *.obj *.exe *.ilk *.pdb

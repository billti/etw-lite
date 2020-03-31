// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides templates and functions to enable the `constexpr` declaration
// of metadata for ETW events.

#include "etw-metadata.h"
#include "foo-provider.h"

namespace etw {
namespace foo {

#if !defined(NO_ETW)
void FooProvider::LogAppLaunched() {
  constexpr static auto event_desc = EventDescriptor(AppLaunchedEvent);
  constexpr static auto event_meta = EventMetadata("AppLaunched");

  LogEventData(State(), &event_desc, &event_meta);
}

void FooProvider::LogParsingStart(const char* fileName, int offset) {
  constexpr static auto event_desc = EventDescriptor(ParsingStartEvent);
  constexpr static auto event_meta = EventMetadata("ParsingStart", 
      Field("Filename", kTypeAnsiStr),
      Field("Offset", kTypeInt32));
  
  LogEventData(State(), &event_desc, &event_meta, fileName, offset);
}
#endif

} //  namespace foo

// Create the global "etw::Foo" that is the instance of the provider
foo::FooProvider Foo{};

}  // namespace etw
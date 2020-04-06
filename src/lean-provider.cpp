// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lean-provider.h"

#include <type_traits>

#include "../lib/etw-metadata.h"

namespace etw {
namespace lean {

#if !defined(NO_ETW)
void LeanProvider::LogAppLaunched() {
  constexpr static auto event_desc = EventDescriptor(AppLaunchedEvent);
  constexpr static auto event_meta = EventMetadata("AppLaunched");

  LogEventData(State(), &event_desc, &event_meta);
}

void LeanProvider::LogParsingStart(const char* fileName, int offset) {
  constexpr static auto event_desc = EventDescriptor(ParsingStartEvent);
  constexpr static auto event_meta = EventMetadata("ParsingStart", 
      Field("Filename", kTypeAnsiStr),
      Field("Offset", kTypeInt32));
  
  LogEventData(State(), &event_desc, &event_meta, fileName, offset);
}
#endif

static_assert(std::is_trivial<LeanProvider>::value, "LeanProvider is not trivial");

} //  namespace lean

// Create the global "etw::Lean" that is the instance of the provider
lean::LeanProvider Lean{};

}  // namespace etw

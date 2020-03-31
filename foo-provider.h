// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "etw-provider.h"

namespace etw {
namespace foo {

constexpr char ProviderName[] = "FooProvider";
static_assert(sizeof(ProviderName) < kMaxTraitSize - 3);

// Define the event descriptor data for each event
// Note: Order of fields is: eventId, level, opcode, task, keyword
constexpr EventInfo AppLaunchedEvent {100, kLevelInfo};
constexpr EventInfo ParsingStartEvent{101, kLevelVerbose, kOpCodeStart};
constexpr EventInfo ParsingStopEvent {102, kLevelVerbose, kOpCodeStop};


class FooProvider : public EtwProvider {
 public:

#if defined(NO_ETW)

// For NO_ETW, just provide inlined no-op versions of the public APIs
  void AppLaunched(){}
  void ParsingStart(const char* filename, int offset) {}

#else  // defined(NO_ETW)

// For the "real" implementation, override the pure virtual functions
  const GUID* Guid() final {
    constexpr static GUID id = {0x12345678, 0xFFFF, 0xFFFF, {1, 2, 3, 4, 5, 6, 7, 8}};
    return &id;
  }

  const char* Name() final {
    return ProviderName;
  }

  // The public APIs to log the events should all be inline wrappers that call
  // to internal implementations. You can check if a session is listening first
  // for optimal efficiency. That state is maintained by the base class.

  void AppLaunched() {
    // For infrequent and cheap events, just check if the provider is enabled.
    if (IsEnabled()) LogAppLaunched();
  }

  void ParsingStart(const char* fileName, int offset) {
    // For verbose or costly events, check the level and keywords are enabled
    // before writing the event.
    if (IsEnabled(ParsingStartEvent)) LogParsingStart(fileName, offset);
  }

 private:
  // All the implementation complexity lives outside the header (and doesn't
  // exist for NO_ETW builds).
  void LogAppLaunched();
  void LogParsingStart(const char* fileName, int offset);

#endif  // defined(NO_ETW)

};

}  // namespace foo

// Declare the global "etw::Foo" that is the instance of the provider
extern foo::FooProvider Foo;

}  // namespace etw

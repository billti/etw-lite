// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
Provide name and GUID generated from it are:

    "billti-example",
    // {c212d3ce-dfb9-5469-08f5-f477b0d92305}
    (0xc212d3ce,0xdfb9,0x5469,0x08,0xf5,0xf4,0x77,0xb0,0xd9,0x23,0x05));

Note: Below should be run from an admin prompt.

For simple testing, use "logman" to create a trace for this provider via:

  logman create trace -n example -o example.etl -p {c212d3ce-dfb9-5469-08f5-f477b0d92305}

After the provider GUID, you can optionally specificy keywords and level, e.g.

  -p {c212d3ce-dfb9-5469-08f5-f477b0d92305} 0xBEEF 0x05

To capture events, start/stop the trace via:
  logman start example
  logman stop example

When finished recording, remove the configured trace via:

  logman delete example

Alternatively, use a tool such as PerfView or WPR to configure and record traces.
*/

#pragma once

#include "../lib/etw-provider.h"

namespace etw {
namespace lean {

constexpr char ProviderName[] = "billti-example";
constexpr GUID ProviderGuid = {0xc212d3ce,0xdfb9,0x5469,{0x08,0xf5,0xf4,0x77,0xb0,0xd9,0x23,0x05}};

// Define the event descriptor data for each event
// Note: Order of fields is: eventId, level, opcode, task, keyword
constexpr EventInfo AppLaunchedEvent {100, kLevelInfo, 0, 0, 0};
constexpr EventInfo ParsingStartEvent{101, kLevelVerbose, kOpCodeStart, 0, 0};
constexpr EventInfo ParsingStopEvent {102, kLevelVerbose, kOpCodeStop, 0, 0};


class LeanProvider : public EtwProvider {
 public:

#if defined(NO_ETW)

// For NO_ETW, just provide inlined no-op versions of the public APIs
  void RegisterProvider(){}
  void UnregisterProvider(){}
  void Initialize(){}
  void AppLaunched(){}
  void ParsingStart(const char* filename, int offset) {}

#else  // defined(NO_ETW)

  void RegisterProvider() {
    Register(ProviderGuid, ProviderName);
  }

  void UnregisterProvider() {
    Unregister();
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

}  // namespace lean

// Declare the global "etw::Lean" that is the instance of the provider
extern lean::LeanProvider Lean;

}  // namespace etw

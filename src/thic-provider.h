// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "../lib/etw-provider.h"
#include "../lib/etw-metadata.h"

namespace etw {
namespace thic {

// logman create trace -n thic -o thic.etl -p {1b0d5501-a5fb-5d95-d960-4647bec69d41}
constexpr char ProviderName[] = "billti-thic";
constexpr GUID ProviderGuid = {0x1b0d5501,0xa5fb,0x5d95,{0xd9,0x60,0x46,0x47,0xbe,0xc6,0x9d,0x41}};

// Define the event descriptor data for each event
// Note: Order of fields is: eventId, level, opcode, task, keyword
constexpr EventInfo AppLaunchedEvent {100, kLevelInfo, 0, 0, 0};
constexpr EventInfo ParsingStartEvent{101, kLevelVerbose, kOpCodeStart, 0, 0};
constexpr EventInfo ParsingStopEvent {102, kLevelVerbose, kOpCodeStop, 0, 0};


class ThicProvider : public EtwProvider {
 public:
  void RegisterProvider() {
    // To be called before use. Put any singleton init work here.
    Register(ProviderGuid, ProviderName);
  }

  void UnregisterProvider() {
    // Put any clean-up here.
    Unregister();
  }

  void AppLaunched() {
    if (!IsEnabled()) return;
    constexpr static auto event_desc = EventDescriptor(AppLaunchedEvent);
    constexpr static auto event_meta = EventMetadata("AppLaunched");

    LogEventData(State(), &event_desc, &event_meta);
  }

  void ParsingStart(const char* fileName, int offset) {
    if (!IsEnabled(ParsingStartEvent)) return;
    constexpr static auto event_desc = EventDescriptor(ParsingStartEvent);
    constexpr static auto event_meta = EventMetadata("ParsingStart", 
        Field("Filename", kTypeAnsiStr),
        Field("Offset", kTypeInt32));
    
    LogEventData(State(), &event_desc, &event_meta, fileName, offset);
  }
};

}  // namespace thic

// Declare the global "etw::Thic" that is the instance of the provider
extern thic::ThicProvider Thic;

}  // namespace etw

// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

// Minimize dependencies. No platform specific inclusions (e.g. "Windows.h")
#include <cstdint>

// Optimized inline checks to minimize cost if the provider isn't enabled
#if defined(__GNUC__)
#define LIKELY(x)    __builtin_expect(!!(x), 1)
#else
#define LIKELY(x) x
#endif

namespace etw {

// Redefine the structure here to avoid including Windows headers
struct GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
};

constexpr int kMaxTraitSize = 40;  // Provider name can be max 37 chars
struct ProviderState {
  uint64_t regHandle;
  uint32_t enabled;
  uint8_t  level;
  uint64_t keywords;
  char     provider_trait[kMaxTraitSize];
};

struct EventInfo {
  uint16_t id;
  uint8_t  level;
  uint8_t  opcode;
  uint16_t task;
  uint64_t keywords;
};

// Taken from the TRACE_LEVEL_* macros in <evntrace.h>
const uint8_t kLevelNone = 0;
const uint8_t kLevelFatal = 1;
const uint8_t kLevelError = 2;
const uint8_t kLevelWarning = 3;
const uint8_t kLevelInfo = 4;
const uint8_t kLevelVerbose = 5;

// Taken from the EVENT_TRACE_TYPE_* macros in <evntrace.h>
const uint8_t kOpCodeInfo = 0;
const uint8_t kOpCodeStart = 1;
const uint8_t kOpCodeStop = 2;

// Event field data types. See "enum TlgIn_t" in <TraceLoggingProvider.h>
const uint8_t kTypeUnicodeStr = 1;  // WCHARs (wchar_t on Windows)
const uint8_t kTypeAnsiStr    = 2;  // CHARs  (char on Windows)
const uint8_t kTypeInt8       = 3;
const uint8_t kTypeUInt8      = 4;
const uint8_t kTypeInt16      = 5;
const uint8_t kTypeUInt16     = 6;
const uint8_t kTypeInt32      = 7;
const uint8_t kTypeUInt32     = 8;
const uint8_t kTypeInt64      = 9;
const uint8_t kTypeUInt64     = 10;
const uint8_t kTypeFloat      = 11;
const uint8_t kTypeDouble     = 12;
const uint8_t kTypeBool32     = 13;

const uint8_t kTypeHexInt32   = 20;
const uint8_t kTypeHexInt64   = 21;
const uint8_t kTypePointer    = (sizeof(void*) == 8) ? kTypeHexInt64 : kTypeHexInt32;

// All "manifest-free" events should go to channel 11 by default
const uint8_t kManifestFreeChannel = 11;

// The base class for providers
class EtwProvider {
 public:

#if defined(NO_ETW)

// For NO_ETW, the class is just the public APIs as inlined no-ops
  uint8_t  Level()     { return 0; }
  uint64_t Keywords()  { return 0; }

  bool IsEnabled() { return false; }
  bool IsEnabled(const EventInfo& event) { return false; }

#else  // defined(NO_ETW)

  // Inline any calls to check the provider state
  uint8_t  Level()     { return state.level; }
  uint64_t Keywords()  { return state.keywords; }

  bool IsEnabled() { 
    if (LIKELY(!state.enabled)) return false;
    return true;
  }

  bool IsEnabled(const EventInfo& event) {
    if (LIKELY(!state.enabled)) return false;
    if (event.level > state.level) return false;
    return event.keywords == 0 || (event.keywords & state.keywords);
  }

 protected:
  uint32_t Register(const GUID& providerGuid, const char* providerName);
  void Unregister();

  // Derived classes need access to read the state for the logging calls
  const ProviderState& State() { return state; }

 private:
  static void Callback(const GUID* srcId, uint32_t providerState, uint8_t level,
      uint64_t anyKeyword, uint64_t allKeyword, void* filter, void* context);

  void UpdateState(bool isEnabled, uint8_t level, uint64_t keywords) {
    state.level = level;
    state.keywords = keywords;
    state.enabled = isEnabled;
  }
  uint64_t RegHandle() { return state.regHandle; }

  ProviderState state;

#endif  // defined(NO_ETW)

};  //  class EtwProvider

}  // namespace etw

#undef LIKELY

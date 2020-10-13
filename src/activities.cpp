// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
This file shows at a low-level how to log manifest-free events directly using the
Win32 API. It is also used to examine the behavior of activities across threads.
*/

#include <Windows.h>
#include <evntprov.h>

#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace std;

// logman create trace -n thic -o thic.etl -p {1b0d5501-a5fb-5d95-d960-4647bec69d41}
constexpr char ProviderName[] = "billti-thic";
constexpr GUID ProviderGuid = {0x1b0d5501,0xa5fb,0x5d95,{0xd9,0x60,0x46,0x47,0xbe,0xc6,0x9d,0x41}};

// ETW constants
constexpr UCHAR kManifestFree = 11;
constexpr UCHAR kLevelVerbose = 5;
constexpr UCHAR kOpInfo       = 0;
constexpr UCHAR kOpStart      = 1;
constexpr UCHAR kOpStop       = 2;
constexpr UCHAR kTypeAnsiStr  = 2;

// Id, version, channel, level, opcode, task, keyword
constexpr EVENT_DESCRIPTOR MsgEvent         = {100, 0, kManifestFree, kLevelVerbose, kOpInfo,  0, 0};
constexpr EVENT_DESCRIPTOR AppStartEvent    = {101, 0, kManifestFree, kLevelVerbose, kOpStart, 0, 0};
constexpr EVENT_DESCRIPTOR AppStopEvent     = {102, 0, kManifestFree, kLevelVerbose, kOpStop,  0, 0};
constexpr EVENT_DESCRIPTOR WorkerStartEvent = {103, 0, kManifestFree, kLevelVerbose, kOpStart, 0, 0};
constexpr EVENT_DESCRIPTOR WorkerStopEvent  = {104, 0, kManifestFree, kLevelVerbose, kOpStop,  0, 0};

REGHANDLE provider_handle;

// Make the below thread_locals as they will be used concurrently from different threads
thread_local EVENT_DATA_DESCRIPTOR DataDescs[3]; // 0 = traits, 1 = event metadata, 2 = event data

// Buffers to hold the metadata that the event descriptors will point to.
thread_local char traits_meta[0xFF];
thread_local char event_meta[0xFF];

// For tracking the current activityId
thread_local GUID activity_id = GUID_NULL;
thread_local bool in_activity = false;

void SetTraitsMetadata(const string& providerName) {
  // Traits is the null terminated provider name prefixed by a uint16 total_length.
  int trait_length = providerName.length() + 3;
  assert(trait_length <= 0xFF);
  memset(traits_meta, 0, 0xFF);
  traits_meta[0] = trait_length;
  providerName.copy(traits_meta + 2, providerName.length());
  EventDataDescCreate(&DataDescs[0], traits_meta, trait_length);
  DataDescs[0].Type = EVENT_DATA_DESCRIPTOR_TYPE_PROVIDER_METADATA;
}

void SetEventMetadata(const string& eventName) {
  static const string fieldName = "msg";
  // EventMetadata is the following fields tightly packed:
  //   uint16 total_length
  //   byte   tag (0)
  //   char[] event_name  // char[]s are null terminated
  //   char[] field_name  // This and below occur 0 or more times
  //   byte   field_type
  int meta_length = 3 + eventName.length() + 1 + fieldName.length() + 2;
  assert(meta_length <= 0xFF);
  memset(event_meta, 0, 0xFF);
  event_meta[0] = meta_length;
  eventName.copy(event_meta + 3, eventName.length());
  fieldName.copy(event_meta + 3 + eventName.length() + 1, fieldName.length());
  event_meta[meta_length -1] = kTypeAnsiStr;
  EventDataDescCreate(&DataDescs[1], event_meta, meta_length);
  DataDescs[1].Type = EVENT_DATA_DESCRIPTOR_TYPE_EVENT_METADATA;
}

ULONG LogEvent(PCEVENT_DESCRIPTOR desc, const string& eventName, const string& msg,
    const GUID* activityId = nullptr, const GUID* relatedActivityId = nullptr) {
  SetTraitsMetadata(ProviderName);
  SetEventMetadata(eventName);
  EventDataDescCreate(&DataDescs[2], msg.data(), msg.length() + 1);

  if (activityId == nullptr && in_activity) activityId = &activity_id;

  return EventWriteTransfer(provider_handle, desc, activityId, relatedActivityId, 3, DataDescs);
}

/*
The general process for starting an Activity should be:
 - Let 'relatedId' = currently ActivityId (CREATE_SET)
 - Let 'activityId' = a newly created Id  (GET)
 - Call a Start event with relatedId and activityId
 - ... do the activity, logging events with ActivityId
 - Call a Stop event with ActivityId
 - Restore the thread_local activityId to relatedId.

'relatedId' should be stored in a stack allocated variable to allow
for recursive calls creating new activities.

 Note for "worker threads", the 'relatedId' should be passed in, and
 the thread_local activity should be restored to GUID_NULL when done.
*/

// Create an automatic variable of the below to start a new activity.
// It will restore the old activityId automatically when it goes out of scope.
struct AutoActivity {
  AutoActivity() {
    // Save the current activityId in priorId, set a new activityId
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_SET_ID, &priorId);
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_GET_ID, &activity_id);
    in_activity = true;
  }
  ~AutoActivity() {
    // Restore the priorId as the current activityId
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_SET_ID, &priorId);
    activity_id = priorId;
    if (activity_id == GUID_NULL) in_activity = false;
  }
  GUID priorId;
};

void f1(GUID relatedId) {
  AutoActivity local_activity{};
  cout << "Running in f1" << endl;
  HRESULT hr = LogEvent(&WorkerStartEvent, "WorkerStart", "Starting worker", nullptr, &relatedId);
  LogEvent(&MsgEvent, "Msg", "Doing stuff in worker");
  this_thread::sleep_for(1s);
  LogEvent(&MsgEvent, "Msg", "Doing more stuff in worker");
  hr = LogEvent(&WorkerStopEvent, "WorkerStop", "Stopping worker");
  cout << "Exiting f1" << endl;
}

int main() {
  HRESULT hr = EventRegister(&ProviderGuid, NULL, NULL, &provider_handle);
  if (FAILED(hr)) return -1;

  LogEvent(&MsgEvent, "Msg", "App launched");

  {
    AutoActivity local_activity{};

    // First activity is unrelated to any prior
    hr = LogEvent(&AppStartEvent, "AppStart", "App start activity");
    if (FAILED(hr)) return -2;
    this_thread::sleep_for(500ms);

    // Pass the parent activity. This will show as a nested child activity.
    thread t1{f1, activity_id};
    cout << "Started thread t1" << endl;

    this_thread::sleep_for(100ms);

    // Pass NULL for the parent. This will show as an independent actitity.
    thread t2{f1, GUID_NULL};
    cout << "Started thread t2" << endl;
    // etc.
    LogEvent(&MsgEvent, "Msg", "Doing stuff in main");

    // On shutdown (optional)
    t1.join();
    t2.join();
    this_thread::sleep_for(200ms);

    cout << "Threads stopped" << endl;
    hr = LogEvent(&AppStopEvent, "AppStop", "App stop activity");
  }

  cout << "Done" << endl;
  LogEvent(&MsgEvent, "Msg", "App done");
  EventUnregister(provider_handle);
}

// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "thic-provider.h"

// Provide the definition for the global instance of the Thic provider somewhere
etw::thic::ThicProvider Thic{};

int main() {
  // At initialization
  Thic.RegisterProvider();

  // Logging during execution
  Thic.ParsingStart("ing", 55);
  // etc.

  // On shutdown (optional)
  Thic.UnregisterProvider();
}

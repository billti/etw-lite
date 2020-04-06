// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lean-provider.h"

int main() {
  // At initialization
  etw::Lean.RegisterProvider();

  // Logging during execution
  etw::Lean.AppLaunched();
  etw::Lean.ParsingStart("test", 0);
  // etc.

  // On shutdown (optional)
  etw::Lean.UnregisterProvider();
}

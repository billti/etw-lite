// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "foo-provider.h"

int main() {
  // At initialization
  etw::Foo.Initialize();

  // Logging during execution
  etw::Foo.AppLaunched();
  etw::Foo.ParsingStart("test", 0);
  // etc.

  // On shutdown (optional)
  etw::Foo.Unregister();
}

# etw-lite

The goal is to provide an ETW framework that is high performance (at both
runtime and compile-time) and easy to instrument code with. Specifically:

 - When ETW is not listening, instrumentation should be next to free.
 - No preprocessor directives (e.g. `#ifdef`) should be required to instrument
   application code, even if building without ETW (e.g. for non-Windows platforms).
 - Complex header files, such as Window.h or variadic templates, (or other compile-time 
   programming tricks), should not be transitively included in application code.
 - Log events via the "manifest-free" ETW support, making deploying ETW-enabled
   applications, and consuming ETW traces trivial.

## The implementation

The files that make up the basic framework are:

 - `etw-metadata.h`: Defines the variadic templates for constructing ETW metadata
 - `etw-provider.h`: Defines the EtwProvider base class to derive providers from
 - `etw-provider.cpp`: Implements calls to register the provider, maintain state, etc.

To implement a provider, follow the basic outline shown in the below files:

 - `foo-provider.h`: Per-provider class that derives from EtwProvider
 - `foo-provider.cc`: Provides the implementation to do the actual logging calls

The application code to be instrumented should then only include `foo-provider.h`
(which includes `etw-provider.h`). These files don't use any complex templates or
platform specific (e.g. Win32) headers. They do provide inline functions to check
the provider state safely and cheaply, making application code efficient.

To build without ETW support, such as for non-Windows builds, no application code
changes are needed (i.e. you can still leave in all the event tracing calls and
header includes). Simply define the `NO_ETW` macro for the build. (See the sample
`makefile` in the root, which is written to run with MSVC's `nmake`).

## The design

The provider implementations (e.g. `etw-provider.cpp` and `foo-provider.cpp` in
this sample) are the only translation units that will include any complex or Windows
specific headers (e.g. `<Windows.h>`, `<evntprov.h>`, and `etw-metadata.h`), in an
effort to keep both application code and compilation clean and efficient.

The Windows SDK header file for manifest-free ETW (`<TraceLoggingProvider.h>`) is
not used, as this introduces certain limitations and complexities (see <https://ticehurst.com/2019/11/04/manifest-free-etw-in-cpp.html>
for details). The `etw-metadata.h` header provides some similar functionality in
a cross-platform and cross-compiler compatible way, using only standard C++.

The EtwProvider base class maintains provider state via a callback it registers
to be notified of any change in sessions listening to the provider. This allows
for checking if the work to log an event should be done just by checking a variable
value, without any Win32 API calls. Thus the instrumentation is next to free if
nothing is listening for the events to be logged.

An instance of each ETW provider class is declared and defined as a global variable.
A global is used, as the provider should be a singleton in a binary, and this
allows checking if a provider is enabled to be a single `cmp byte ptr [foo+10], 0`
instruction. Having the class instance be a global of a trivial POD data-type also
means it will be zero initialized and will not require a constructor (or destructor)
to be invoked at process startup or shutdown. This helps avoid race conditions
with initialization (e.g. the `IsEnabled` flag will be 0 when the provider is
uninitialized), versus having a class instance that needs to be initialized by the
C-runtime via its constructor before being used, and destructed on cleanup.
(See the guidance at <https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables>
for more info on why this is desirable).

With the combination of zero-initialized global state, careful use of inline
functions in header files, and optimizing compilers, a lot of code and/or calls can be
elided. As a simple example, the below shows a simple function compiled with optimizations:

```cpp
int main() {
  etw::Foo.Register();   // Call at some point during app initialization.

  // While doing stuff..
  etw::Foo.AppLaunched();
  etw::Foo.ParsingStart("test", 0);
  // etc..

  etw::Foo.Unregister(); // Before shutdown (optionally, or just exit).
}
/* Compiles to:

; int main() {
00007ff6`66c71430 4883ec28       sub     rsp, 28h

;   etw::Foo.Register();  // Inlined call to RegisterEtwProvider with a 'this' argument
00007ff6`66c71434  lea     rcx, [app!Foo (00007ff6`66c7a000)]
00007ff6`66c7143b  call    app!ILT+25(?RegisterEtwProvider) (00007ff6`66c7101e)

;   etw::Foo.AppLaunched  // Inlined. Checks if enabled once, and jumps over both logging call if not!
00007ff6`66c71440  cmp     dword ptr [app!Foo+0x10 (00007ff6`66c7a010)], 0
00007ff6`66c71447  je      app!main+0x4d (00007ff6`66c7147d)
;   Inlined LogAppLaunched call takes just a 'this' argument
00007ff6`66c71449  lea     rcx, [app!Foo (00007ff6`66c7a000)]
00007ff6`66c71450  call    app!ILT+5(?LogAppLaunchedFooProvider) (00007ff6`66c7100a)

;   etw::Foo.ParsingStart and its 'IsEnabled(EventInfo)' call are inlined. Checks enabled and the logging level.
;   Interesting to note it elided the 'keywords == 0 || ...' check, as it knows that is always true!
00007ff6`66c71455  cmp     dword ptr [app!Foo+0x10 (00007ff6`66c7a010)], 0
00007ff6`66c7145c  je      app!main+0x4d (00007ff6`66c7147d)
00007ff6`66c7145e  cmp     byte ptr [app!Foo+0x14 (00007ff6`66c7a014)], 5
00007ff6`66c71465  jb      app!main+0x4d (00007ff6`66c7147d)

;   Inlined call to LogParsingStart after the above checks passed
00007ff6`66c71467  lea     rcx, [app!Foo (00007ff6`66c7a000)]
00007ff6`66c7146e  lea     rdx, [app!`string' (00007ff6`66c78cd0)]
00007ff6`66c71475  xor     r8d, r8d
00007ff6`66c71478  call    app!ILT+200(?LogParsingStartFooProvider) (00007ff6`66c710cd)

;   etw::Foo.Unregister()  // This is the target of above jumps when logging is disabled.
00007ff6`66c7147d  lea     rcx, [app!Foo (00007ff6`66c7a000)]
00007ff6`66c71484  call    app!ILT+205(?UnregisterEtwProvider) (00007ff6`66c710d2)

; }
00007ff6`66c71489  xor     eax, eax
00007ff6`66c7148b  add     rsp, 28h
00007ff6`66c7148f  ret    
*/
```

With `NO_ETW` defined for the build, the entire program is a no-op, and the `main` function compiles to:

```cpp
/*
; int main() {
00007ff6`36b61320  xor     eax,eax
00007ff6`36b61322  ret
; }
*/
```

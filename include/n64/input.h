#pragma once

namespace n64 {

class Controller;

// Reads the current real-time keyboard state and writes the corresponding
// button/stick state into `controller` (overwriting its previous state).
//
// This is a thin platform shim: on Windows it uses GetAsyncKeyState (matching
// the native Win32 video/file-dialog backend, no SDL), everywhere else it uses
// SDL_GetKeyboardState (the same SDL backend the video layer already links).
// It is deliberately kept out of n64core so the pure Controller model and the
// unit tests never depend on a platform input API.
void PollKeyboardInto(Controller& controller);

// Prints the default keyboard bindings to stdout. Call once at startup so the
// controls are discoverable without reading the source.
void PrintKeyBindings();

}  // namespace n64

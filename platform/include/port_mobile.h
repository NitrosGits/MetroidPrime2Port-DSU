#pragma once

// PORT_MOBILE: a touch-first phone or tablet (Android, iOS). Gates the touch UI (overlay defaults,
// inline help, no hotkey hints, no emulator/desktop-folder rows); Android-only APIs (JNI, storage
// picker, gallery export) keep `__ANDROID__`.
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#define PORT_MOBILE 1
#endif

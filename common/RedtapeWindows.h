// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// We require Windows 10+.
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0A00 // Windows 10

// WINVER and NTDDI have to agree with it. MSVC's SDK derives what it needs,
// but mingw-w64 guards the types of an API on WINVER and its functions on
// NTDDI_VERSION, so a mismatch leaves the functions declared with types that
// were never defined - VirtualAlloc2 and MapViewOfFile3, which the fastmem
// mapping uses, need the NTDDI floor.
#ifdef WINVER
#undef WINVER
#endif
#define WINVER 0x0A00
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000006 // NTDDI_WIN10_RS5
#endif

// winnt.h and the intrinsics header it pulls in declare functions with
// __forceinline and expect mingw's own spelling of it, which is an inline
// definition. PCSX2 redefines the macro without the inline keyword (see
// common/Pcsx2Defs.h), and every one of those declarations would then emit a
// non-inline definition in each translation unit that includes this header -
// the linker sees SetThreadpoolCallbackPersistent, _InterlockedIncrement and
// friends defined many times over. Give the system headers the definition
// they were written against.
#if defined(__MINGW32__)
// _mingw.h defines __forceinline, and push_macro can only restore what is
// defined at the time it runs - in a translation unit that reaches this header
// before anything else, that is nothing at all, and the pop below would leave
// the name undefined for every later system header (winsock2.h among them).
#include <_mingw.h>
#pragma push_macro("__forceinline")
#undef __forceinline
// _mingw.h will not define it again for us on the way back in, so spell out
// what it uses.
#define __forceinline extern __inline__ __attribute__((__always_inline__, __gnu_inline__))
#endif

#include <windows.h>

#if defined(__MINGW32__)
#pragma pop_macro("__forceinline")
#endif

#endif

// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/Pcsx2Defs.h"

// Include this after the system headers in a .cpp that defines functions with
// __fi (or __emitinline) which the rest of the tree calls through an extern
// declaration - the emitter's xWrite8, xPUSH, xGetPtr, SimdPrefix and friends,
// or spu2Output. mingw's __forceinline is an inline definition, and GCC emits
// no out-of-line copy of one, so those references would not link. Nothing
// below this point may include a system header: they want mingw's spelling.

#if defined(__MINGW32__)
#undef __forceinline
#define __forceinline __attribute__((always_inline, unused))
#endif

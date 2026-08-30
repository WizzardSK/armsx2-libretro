// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#ifdef _WIN32

#include "common/RedtapeWindows.h"

#if defined(__MINGW32__)

// wil/com.h wants the Windows SDK's WeakReference.h, which mingw-w64 does not
// ship. common/MinGWWil.h carries the subset this tree uses instead.
#include "common/MinGWWil.h"

#else

// warning : variable 's_hrErrorLast' set but not used [-Wunused-but-set-variable]
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#include <wil/com.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif // !__MINGW32__

#endif // _WIN32

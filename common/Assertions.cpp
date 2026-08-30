// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "Assertions.h"
#include "CrashHandler.h"
#include "HostSys.h"
#include "Threading.h"

#include <cstring>
#include <mutex>

#ifdef _WIN32
#include "RedtapeWindows.h"
// See common/BitUtils.h: mingw's intrin.h wants the platform's __forceinline.
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
#include <intrin.h>
#if defined(__MINGW32__)
#pragma pop_macro("__forceinline")
#endif
#include <tlhelp32.h>
#endif

#ifdef __UNIX__
#include <signal.h>
#endif

static std::mutex s_assertion_failed_mutex;

static inline void FreezeThreads(void** handle)
{
#if defined(_WIN32)
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 threadEntry;
		if (Thread32First(snapshot, &threadEntry))
		{
			do
			{
				if (threadEntry.th32ThreadID == GetCurrentThreadId())
					continue;

				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
				if (hThread != nullptr)
				{
					SuspendThread(hThread);
					CloseHandle(hThread);
				}
			} while (Thread32Next(snapshot, &threadEntry));
		}
	}

	*handle = static_cast<void*>(snapshot);
#else
	* handle = nullptr;
#endif
}

static inline void ResumeThreads(void* handle)
{
#if defined(_WIN32)
	if (handle != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 threadEntry;
		if (Thread32First(reinterpret_cast<HANDLE>(handle), &threadEntry))
		{
			do
			{
				if (threadEntry.th32ThreadID == GetCurrentThreadId())
					continue;

				HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
				if (hThread != nullptr)
				{
					ResumeThread(hThread);
					CloseHandle(hThread);
				}
			} while (Thread32Next(reinterpret_cast<HANDLE>(handle), &threadEntry));
		}
		CloseHandle(reinterpret_cast<HANDLE>(handle));
	}
#else
#endif
}

void pxOnAssertFail(const char* file, int line, const char* func, const char* msg)
{
	std::unique_lock guard(s_assertion_failed_mutex);

	void* handle;
	FreezeThreads(&handle);

	char full_msg[512];
	std::snprintf(full_msg, sizeof(full_msg), "%s:%d: assertion failed in function %s: %s\n", file, line, func, msg);

#if defined(_WIN32)
	HANDLE error_handle = GetStdHandle(STD_ERROR_HANDLE);
	if (error_handle != INVALID_HANDLE_VALUE)
		WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), full_msg, static_cast<DWORD>(std::strlen(full_msg)), NULL, NULL);
	OutputDebugStringA(full_msg);

	std::snprintf(
		full_msg, sizeof(full_msg),
		"Assertion failed in function %s (%s:%d):\n\n%s\n\nPress Abort to exit, Retry to break to debugger, or Ignore to attempt to continue.",
		func, file, line, msg);

	int result = MessageBoxA(NULL, full_msg, NULL, MB_ABORTRETRYIGNORE | MB_ICONERROR);
	if (result == IDRETRY)
	{
		__debugbreak();
	}
	else if (result != IDIGNORE)
	{
		// try to save a crash dump before exiting
		CrashHandler::WriteDumpForCaller();
		TerminateProcess(GetCurrentProcess(), 0xBAADC0DE);
	}
#else
	fputs(full_msg, stderr);
	fputs("\nAborting application.\n", stderr);
	fflush(stderr);
	AbortWithMessage(full_msg);
#endif

	ResumeThreads(handle);
}

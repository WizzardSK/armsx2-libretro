// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "common/RedtapeWindows.h"

#if defined(__MINGW32__)

// VirtualAlloc2, MapViewOfFile3 and UnmapViewOfFile2 are reached through the
// OneCore umbrella import library on MSVC, and mingw-w64 ships no equivalent.
// They are ordinary kernelbase.dll exports on every Windows this builds for,
// so resolve them once at first use instead of linking an import library.
// Every translation unit that calls them - the fastmem mapping in WinHostSys
// and the GS one in GS.cpp - has to include this.

namespace detail
{
	template <typename T>
	static T LoadKernelBaseFunction(const char* name)
	{
		static HMODULE kernelbase = ::GetModuleHandleW(L"kernelbase.dll");
		return kernelbase ? reinterpret_cast<T>(::GetProcAddress(kernelbase, name)) : nullptr;
	}
} // namespace detail

static inline PVOID MinGWVirtualAlloc2(HANDLE process, PVOID base, SIZE_T size, ULONG type, ULONG protection,
	MEM_EXTENDED_PARAMETER* parameters, ULONG parameter_count)
{
	using FunctionType = PVOID(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
	static const FunctionType function = detail::LoadKernelBaseFunction<FunctionType>("VirtualAlloc2");
	return function ? function(process, base, size, type, protection, parameters, parameter_count) : nullptr;
}

static inline PVOID MinGWMapViewOfFile3(HANDLE file_mapping, HANDLE process, PVOID base, ULONG64 offset,
	SIZE_T view_size, ULONG allocation_type, ULONG protection, MEM_EXTENDED_PARAMETER* parameters,
	ULONG parameter_count)
{
	using FunctionType = PVOID(WINAPI*)(
		HANDLE, HANDLE, PVOID, ULONG64, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
	static const FunctionType function = detail::LoadKernelBaseFunction<FunctionType>("MapViewOfFile3");
	return function ? function(file_mapping, process, base, offset, view_size, allocation_type, protection,
							parameters, parameter_count) :
					  nullptr;
}

static inline BOOL MinGWUnmapViewOfFile2(HANDLE process, PVOID base, ULONG flags)
{
	using FunctionType = BOOL(WINAPI*)(HANDLE, PVOID, ULONG);
	static const FunctionType function = detail::LoadKernelBaseFunction<FunctionType>("UnmapViewOfFile2");
	return function ? function(process, base, flags) : FALSE;
}

#define VirtualAlloc2 MinGWVirtualAlloc2
#define MapViewOfFile3 MinGWMapViewOfFile3
#define UnmapViewOfFile2 MinGWUnmapViewOfFile2

#endif

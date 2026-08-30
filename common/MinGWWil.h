// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

// The subset of WIL this tree uses, for MinGW.
//
// wil/com.h includes <WeakReference.h>, which comes with the Windows SDK and
// which mingw-w64 does not ship, so a MinGW build cannot use WIL at all. What
// it uses of WIL is small and mechanical - a COM pointer, a few unique handles
// and the COM-init guard - so it is spelled out here rather than the callers
// being rewritten, which would leave the MSVC build looking different for no
// reason. Include it through common/RedtapeWilCom.h, not directly.

#include "common/RedtapeWindows.h"

#include <objbase.h>
#include <oleauto.h>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace wil
{
	/// COM pointer that never throws. put() hands out the address for an API to
	/// fill in and releases whatever was held, matching WIL's semantics.
	template <typename T>
	class com_ptr_nothrow
	{
	public:
		com_ptr_nothrow() = default;
		com_ptr_nothrow(const com_ptr_nothrow&) = delete;
		com_ptr_nothrow& operator=(const com_ptr_nothrow&) = delete;

		com_ptr_nothrow(com_ptr_nothrow&& other) noexcept
			: m_ptr(std::exchange(other.m_ptr, nullptr))
		{
		}

		com_ptr_nothrow& operator=(com_ptr_nothrow&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				reset();
				m_ptr = std::exchange(other.m_ptr, nullptr);
			}
			return *this;
		}

		~com_ptr_nothrow() { reset(); }

		T* get() const { return m_ptr; }
		T* operator->() const { return m_ptr; }
		explicit operator bool() const { return m_ptr != nullptr; }

		bool operator==(std::nullptr_t) const { return m_ptr == nullptr; }
		bool operator!=(std::nullptr_t) const { return m_ptr != nullptr; }

		/// `ptr = nullptr;` is how the DirectShow code releases these.
		com_ptr_nothrow& operator=(std::nullptr_t)
		{
			reset();
			return *this;
		}

		T** put()
		{
			reset();
			return &m_ptr;
		}

		void** put_void()
		{
			reset();
			return reinterpret_cast<void**>(&m_ptr);
		}

		T** addressof() { return &m_ptr; }

		/// IID_PPV_ARGS(&ptr) dereferences what it is given, so the address-of
		/// operator has to hand out the slot the way put() does.
		T** operator&() { return put(); }

		/// QueryInterface that reports failure by coming back empty.
		template <typename U>
		com_ptr_nothrow<U> try_query() const
		{
			com_ptr_nothrow<U> result;
			if (m_ptr && FAILED(m_ptr->QueryInterface(__uuidof(U), result.put_void())))
				result.reset();
			return result;
		}

		void reset(T* ptr)
		{
			reset();
			m_ptr = ptr;
		}

		void reset()
		{
			if (m_ptr)
			{
				m_ptr->Release();
				m_ptr = nullptr;
			}
		}

		T* detach() { return std::exchange(m_ptr, nullptr); }

	private:
		T* m_ptr = nullptr;
	};

	/// A handle with a deleter, in the shape WIL's unique_any is used here:
	/// unique_any<GUID*, decltype(&::LocalFree), ::LocalFree>.
	template <typename T, typename DeleterType, DeleterType Deleter>
	class unique_any
	{
	public:
		unique_any() = default;
		explicit unique_any(T value)
			: m_value(value)
		{
		}
		unique_any(const unique_any&) = delete;
		unique_any& operator=(const unique_any&) = delete;
		~unique_any() { reset(); }

		T get() const { return m_value; }
		explicit operator bool() const { return m_value != T{}; }

		T* put()
		{
			reset();
			return &m_value;
		}

		void reset()
		{
			if (m_value != T{})
			{
				Deleter(m_value);
				m_value = T{};
			}
		}

		void reset(T value)
		{
			reset();
			m_value = value;
		}

		T release() { return std::exchange(m_value, T{}); }

	private:
		T m_value{};
	};

	namespace details
	{
		inline void CloseRegKey(HKEY h) { ::RegCloseKey(h); }
		inline void CloseFileHandle(HANDLE h)
		{
			if (h != INVALID_HANDLE_VALUE)
				::CloseHandle(h);
		}
		inline void FreeLibraryHandle(HMODULE h) { ::FreeLibrary(h); }
		inline void FreeCoTaskMem(void* p) { ::CoTaskMemFree(p); }
	} // namespace details

	using unique_hkey = unique_any<HKEY, decltype(&details::CloseRegKey), details::CloseRegKey>;
	using unique_hmodule = unique_any<HMODULE, decltype(&details::FreeLibraryHandle), details::FreeLibraryHandle>;

	/// Same shape as the others, but INVALID_HANDLE_VALUE - not null - is the
	/// empty value a Win32 file handle uses.
	class unique_hfile
	{
	public:
		unique_hfile() = default;
		explicit unique_hfile(HANDLE handle)
			: m_handle(handle)
		{
		}
		unique_hfile(const unique_hfile&) = delete;
		unique_hfile& operator=(const unique_hfile&) = delete;
		~unique_hfile() { reset(); }

		HANDLE get() const { return m_handle; }
		bool is_valid() const { return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr; }
		explicit operator bool() const { return is_valid(); }

		HANDLE* put()
		{
			reset();
			return &m_handle;
		}

		void reset()
		{
			if (is_valid())
				::CloseHandle(m_handle);
			m_handle = INVALID_HANDLE_VALUE;
		}

		HANDLE release() { return std::exchange(m_handle, INVALID_HANDLE_VALUE); }

	private:
		HANDLE m_handle = INVALID_HANDLE_VALUE;
	};

	class unique_cotaskmem_string
	{
	public:
		unique_cotaskmem_string() = default;
		unique_cotaskmem_string(const unique_cotaskmem_string&) = delete;
		unique_cotaskmem_string& operator=(const unique_cotaskmem_string&) = delete;
		~unique_cotaskmem_string() { reset(); }

		LPWSTR get() const { return m_str; }
		explicit operator bool() const { return m_str != nullptr; }

		LPWSTR* put()
		{
			reset();
			return &m_str;
		}

		void reset()
		{
			if (m_str)
			{
				details::FreeCoTaskMem(m_str);
				m_str = nullptr;
			}
		}

	private:
		LPWSTR m_str = nullptr;
	};

	/// A VARIANT that clears itself, used exactly as the raw struct is.
	class unique_variant : public VARIANT
	{
	public:
		unique_variant() { ::VariantInit(this); }
		unique_variant(const unique_variant&) = delete;
		unique_variant& operator=(const unique_variant&) = delete;
		~unique_variant() { ::VariantClear(this); }

		VARIANT* operator&() { return this; }
		void reset() { ::VariantClear(this); ::VariantInit(this); }
	};

	/// CoInitializeEx on construction, CoUninitialize on destruction. The bool
	/// says whether to initialise now; WIL's default is true.
	class unique_couninitialize_call
	{
	public:
		explicit unique_couninitialize_call(bool initialize = true)
		{
			if (initialize)
				m_initialized = SUCCEEDED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
		}

		unique_couninitialize_call(const unique_couninitialize_call&) = delete;
		unique_couninitialize_call& operator=(const unique_couninitialize_call&) = delete;

		unique_couninitialize_call(unique_couninitialize_call&& other) noexcept
			: m_initialized(std::exchange(other.m_initialized, false))
		{
		}

		unique_couninitialize_call& operator=(unique_couninitialize_call&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				reset();
				m_initialized = std::exchange(other.m_initialized, false);
			}
			return *this;
		}

		~unique_couninitialize_call() { reset(); }

		/// Gives up the CoUninitialize duty without performing it, for the
		/// callers that hand COM's lifetime to something else.
		void release() { m_initialized = false; }

		/// Ends the COM lifetime early; harmless when there is none.
		void reset()
		{
			if (m_initialized)
			{
				::CoUninitialize();
				m_initialized = false;
			}
		}

	private:
		bool m_initialized = false;
	};

	/// Returns an empty pointer when creation fails, so callers can test it.
	template <typename T>
	com_ptr_nothrow<T> CoCreateInstanceNoThrow(REFCLSID clsid, DWORD context = CLSCTX_INPROC_SERVER)
	{
		com_ptr_nothrow<T> result;
		if (FAILED(::CoCreateInstance(clsid, nullptr, context, __uuidof(T), result.put_void())))
			result.reset();
		return result;
	}
} // namespace wil

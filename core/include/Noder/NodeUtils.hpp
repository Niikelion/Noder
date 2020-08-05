#pragma once

#include <type_traits>
#include <exception>
#include <typeindex>
#include <string>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef _WIN32
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLIMPORT
#endif

#ifdef nodercore_EXPORTS
#define DLLACTION DLLEXPORT
#else
#define DLLACTION DLLIMPORT
#endif

namespace Noder
{
	class InvalidPointerException : public std::exception
	{
	private:
		std::string msg;
	public:
		virtual const char* what() const noexcept
		{
			return msg.c_str();
		}

		InvalidPointerException(const std::type_index& type) : msg(std::string("Attempt to access null pointer of type: ")+type.name()) {};
	};

	template<typename T> class Pointer
	{
	private:
		using Type = typename std::remove_reference<T>::type;
		using PointerT = Type*;
		PointerT pointer;
	public:
		inline PointerT get()
		{
			if (pointer == nullptr)
				throw InvalidPointerException(typeid(Type));
			return pointer;
		}

		inline PointerT operator -> ()
		{
			return get();
		}
		inline Type& operator * ()
		{
			return *get();
		}

		inline operator PointerT ()
		{
			return get();
		}

		Pointer<T>& operator = (const Pointer<T>&) = default;
		Pointer<T>& operator = (PointerT t) noexcept
		{
			pointer = t;
			return *this;
		}
		void operator = (Type& t) noexcept
		{
			pointer = &t;
		}

		Pointer() : pointer(nullptr) {}
		Pointer(PointerT t) : pointer(t) {}
		Pointer(const Pointer<T>&) = default;
		Pointer(Pointer<T>&&) noexcept = default;

		~Pointer() = default;
	};
}
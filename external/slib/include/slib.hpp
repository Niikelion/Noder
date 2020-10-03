#ifndef SLIB_H
#define SLIB_H

#include <string>

#if defined(_MSC_VER)
#define SLIB_CDECL __cdecl
#define SLIB_STDCALL __stdcall
#define SLIB_FASTCALL __fastcall
#elif defined(__GNUC__)
#define SLIB_CDECL __attribute__((cdecl))
#define SLIB_STDCALL __attribute__((stdcall))
#define SLIB_FASTCALL __attribute__((fastcall))
#else
#define SLIB_CDECL
#define SLIB_STDCALL
#define SLIB_FASTCALL
#endif

namespace Nlib
{
    class SharedLib
    {
    private:
        void* handle;
    public:
        void* getAddress(const std::string& name) const;
        template<typename T, typename ... Args> auto getFunction(const std::string& name) const->T(SLIB_CDECL*)(Args...)
        {
            return reinterpret_cast<T(SLIB_CDECL *)(Args...)>(getAddress(name));
        }

        template<typename T, typename ... Args> auto getFunctionStd(const std::string& name) const->T(SLIB_STDCALL*)(Args...)
        {
            return reinterpret_cast<T(SLIB_STDCALL *)(Args...) > (getAddress(name));
        }

        template<typename T, typename ... Args> auto getFunctionFast(const std::string& name) const->T(SLIB_FASTCALL*)(Args...)
        {
            return reinterpret_cast<T(SLIB_FASTCALL *)(Args...)>(getAddress(name));
        }

        std::string getFileSuffix() const;
        bool load(const std::string& path);
        void free();

        SharedLib();
        SharedLib(const SharedLib&) = delete;
        SharedLib(SharedLib&&) noexcept;
        ~SharedLib();
    };
}

#endif // SLIB_H

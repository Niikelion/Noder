#ifndef SLIB_H
#define SLIB_H

#include <string>

#ifdef _MSC_VER
#define SLIB_CDECL __cdecl
#define SLIB_STDCALL __stdcall
#define SLIB_FASTCALL __fastcall
#else
#define SLIB_CDECL __attribute__(cdecl)
#define SLIB_STDCALL __attribute__(stdcall)
#define SLIB_FASTCALL __attribute__(fastcall)
#endif

namespace Nlib
{
    class SharedLib
    {
    private:
        void* handle;
    public:
        void* getAddress(const std::string& name) const;
        template<typename T, typename ... Args> auto getFunction(const std::string& name) const->T(__cdecl*)(Args...)
        {
            return reinterpret_cast<T(SLIB_CDECL *)(Args...)>(getAddress(name));
        }

        template<typename T, typename ... Args> auto getFunctionStd(const std::string& name) const->T(__stdcall*)(Args...)
        {
            return reinterpret_cast<T(SLIB_STDCALL *)(Args...) > (getAddress(name));
        }

        template<typename T, typename ... Args> auto getFunctionFast(const std::string& name) const->T(__fastcall*)(Args...)
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

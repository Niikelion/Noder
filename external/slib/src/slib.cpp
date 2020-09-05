#include <slib.hpp>
#include <config.hpp>

namespace Nlib
{

#ifdef _WIN32
#include <windows.h>

    void* SharedLib::getAddress(const std::string& name) const
    {
        if (handle != nullptr)
            return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HINSTANCE>(handle), TEXT(name.c_str())));
        return nullptr;
    }

    bool SharedLib::load(const std::string& path)
    {
        if (handle != nullptr)
            free();
        handle = LoadLibrary(TEXT(path.c_str()));
        if (!handle)
        {
            handle = nullptr;
            return false;
        }
        return true;
    }
    void SharedLib::free()
    {
        if (handle != nullptr)
        {
            FreeLibrary(reinterpret_cast<HINSTANCE>(handle));
            handle = nullptr;
        }
    }

#endif // _WIN32

#ifdef linux
#include <dlfcn.h>

    void* SharedLib::getAddress(const std::string& name) const
    {
        if (handle != nullptr)
            return dlsym(handle, name.c_str());
        return nullptr;
    }
    bool SharedLib::load(const std::string& path)
    {
        if (handle != nullptr)
            free();
        handle = dlopen(path.c_str());
        if (!handle)
        {
            handle = nullptr;
            return false;
        }
        return true;
    }
    void SharedLib::free()
    {
        if (handle != nullptr)
        {
            dlclose(handle);
            handle = nullptr;
        }
    }

#endif // linux

    std::string SharedLib::getFileSuffix() const
    {
        return Nlib::details::dll_suffix;
    }

    SharedLib::SharedLib()
    {
        handle = nullptr;
    }

    SharedLib::SharedLib(SharedLib&& t) noexcept
    {
        handle = t.handle;
        t.handle = nullptr;

    }
    SharedLib::~SharedLib()
    {
        if (handle != nullptr)
            free();
    }
}

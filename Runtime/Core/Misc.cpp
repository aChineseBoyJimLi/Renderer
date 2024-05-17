#include "Misc.h"

std::string StringFormat(const char* format, ...)
{
    std::string str;
    va_list args;
    va_start(args, format);
    size_t size = _vscprintf(format, args);

    if(size > str.capacity())
    {
        str.resize(size);
    }

    vsprintf(str.data(), format, args);

    va_end(args);

    return str;
}

std::wstring StringFormat(const wchar_t* format, ...)
{
    std::wstring str;
    va_list args;
    va_start(args, format);
    size_t size = _vscwprintf(format, args);

    if(size > str.capacity())
    {
        str.resize(size);
    }

    vswprintf(str.data(), format, args);

    va_end(args);

    return str;
}
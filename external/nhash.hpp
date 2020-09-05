/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/

#ifndef NHASH_HPP
#define NHASH_HPP

#include <string>

namespace nhash
{
	//TODO: add more hashes
	namespace constant
	{
		constexpr size_t strlen_const(char const* s)
		{
			return (s[0] == 0) ? 0 : (1 + strlen_const(s + 1));
		}

		constexpr uint32_t fnv1a_32(char const* s, size_t count)
		{
			return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
		}
		constexpr uint32_t fnv1a_32(char const* s)
		{
			return fnv1a_32(s, strlen_const(s));
		}
	}

	uint32_t fnv1a_32(char const* s, size_t count)
	{
		uint32_t ret = 2166136261u;
		const char* const end = s + count;
		while (s != end)
		{
			ret ^= *s;
			ret *= 16777619u;
			++s;
		}
		ret *= 16777619u;
		return ret;
	}
	uint32_t fnv1a_32(char const* s)
	{
		return fnv1a_32(s, std::strlen(s));
	}
	uint32_t fnv1a_32(const std::string& s)
	{
		return fnv1a_32(s.c_str(), s.length());
	}
}

#endif
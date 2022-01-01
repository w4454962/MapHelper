#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace base {
	class conv_method
	{
	public:
		enum
		{
			stop            = 0 << 16,
			skip            = 1 << 16,
			replace         = 2 << 16,
		};

		conv_method(uint32_t value)
			: value_(value)
		{ }

		uint32_t type() const 
		{
			return value_ & 0xFFFF0000;
		}

		uint16_t default_char() const 
		{
			return value_ & 0x0000FFFF;
		}

	private:
		uint32_t value_;
	};

	std::wstring u2w(const std::string_view& str);
	std::string  w2u(const std::wstring_view& wstr);
	std::wstring a2w(const std::string_view& str);
	std::string  w2a(const std::wstring_view& wstr);
	std::string  a2u(const std::string_view& str);
	std::string  u2a(const std::string_view& str);
	
}

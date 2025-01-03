#pragma once


#include <exception>

namespace base { namespace registry {

	class registry_exception
		: public std::exception
	{
	public:
		registry_exception(const char* reason, int error_code)
			: std::exception(reason, error_code)
		{ }
	};

	class access_denied_exception
		: public registry_exception
	{
	public:
		access_denied_exception(const char* reason, int error_code)
			: registry_exception(reason, error_code)
		{ }
	};

	inline void check_and_throw_exception(const char* reason, int error_code)
	{
		if (ERROR_SUCCESS != error_code)
		{
			if (ERROR_ACCESS_DENIED == error_code)
			{
				throw access_denied_exception(reason, error_code);
			}
			else
			{
				throw registry_exception(reason, error_code);
			}
		}
	}
}}

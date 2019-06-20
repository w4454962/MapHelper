#pragma once

namespace base {
	class noncopyable
	{
	protected:
		noncopyable() {}
		~noncopyable() {}
	private:
		noncopyable(const noncopyable&);
		const noncopyable& operator=(const noncopyable&);
	};
}

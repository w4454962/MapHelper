#pragma once
#include <string_view>
#include <deque>
class BinaryWriter {

public:
	BinaryWriter()
		: BinaryWriter(0x1000)//初始化设置二级缓冲区大小
	{ }

	BinaryWriter(size_t second_size) {
		second_pos = 0;
		second.resize(second_size);
	}

	std::deque<uint8_t>& data() {
		if (second_pos) {
			first.resize(first.size() + second_pos);
			std::copy(second.begin(), second.begin() + second_pos, first.end() - second_pos);
			second_pos = 0;
		}
		return first;
	}

	size_t size() {
		return first.size() + second_pos;
	}

	void clear() {
		second_pos = 0;
		first.clear();
		second.clear();
	}
	//双缓冲
	template<typename T>
	void write(const T& begin, const T& end, size_t data_size) {
		size_t size = data_size + second_pos;

		if (size < second.size()) {
			std::copy(begin, end, second.begin() + second_pos);
			second_pos += data_size;
		}
		else {
			first.resize(first.size() + size);
			std::copy(second.begin(), second.begin() + second_pos, first.end() - size);
			std::copy(begin, end, first.end() - data_size);
			second_pos = 0;
		}
	}

	template<typename T>
	void write(const T value) {
		// These wouldn't make sense
		static_assert(std::is_same<T, std::string>() == false);
		static_assert(std::is_same<T, fs::path>() == false);

		T temp = value;
		uint8_t* ptr = (uint8_t*)&temp;
		write(ptr, ptr + sizeof(T), sizeof(T));
	}


	/// Writes the string to the buffer (null terminated if the input string is null terminated)
	void write_string(const std::string& string) {
		write(string.begin(), string.end(), string.size());
	}

	/// Writes a null terminated string to the buffer
	void write_c_string(const std::string& string) {
		if (!string.empty() && string.back() == '\0') {
			write(string.begin(), string.end(), string.size());
		}
		else {
			write(string.begin(), string.end(), string.size());
			write<uint8_t>('\0');
		}
	}

	void write_string_view(const std::string_view& string) {
		write(string.begin(), string.end(), string.size());
	}

	void write_c_string_view(const std::string_view& string) {
		if (!string.empty() && string.back() == '\0') {
			write(string.begin(), string.end(), string.size());
		}
		else {
			write(string.begin(), string.end(), string.size());
			write<uint8_t>('\0');
		}
	}

	/// Copies the contents of the array to the buffer, has special code for std::string
	template<typename T>
	void write_vector(const std::vector<T>& vector) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (const auto& i : vector) {
				write_string(i);
			}
		}
		else {
			write(vector.begin(), vector.end(), vector.size());
		}
	}

	template<typename T>
	void write_deque(const std::deque<T>& deque) {
		if constexpr (std::is_same_v<T, std::string>) {
			for (const auto& i : deque) {
				write_string(i);
			}
		}
		else {
			write(deque.begin(), deque.end(), deque.size());
		}
	}


private:
	//一级缓冲区
	std::deque<uint8_t> first;
	//二级缓冲区
	std::vector<uint8_t> second;
	size_t second_pos;
};
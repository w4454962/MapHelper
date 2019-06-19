#pragma once

template <size_t N>
class BinaryWriterT {
private:
	struct Chunk {
		uint8_t data[N];
		size_t  size = 0;
		Chunk*  next = nullptr;
	};
	Chunk* head = nullptr;
	Chunk* tail = nullptr;

	void destory() {
		for (Chunk* p = head; p;) {
			Chunk* next = p->next;
			delete p;
			p = next;
		}
	}

public:
	BinaryWriterT() {
		head = tail = new Chunk;
	}

	~BinaryWriterT() {
		destory();
	}

	void clear() {
		destory();
		head = tail = new Chunk;
	}

	void write(const uint8_t* buf, size_t len) {
		if (tail->size + len <= N) {
			memcpy(tail->data + tail->size, buf, len);
			tail->size += len;
			return;
		}
		if (tail->size < N) {
			size_t nlen = N - tail->size;
			write(buf, nlen);
			buf += nlen;
			len -= nlen;
		}
		tail->next = new Chunk;
		tail = tail->next;
		write(buf, len);
	}

	template <typename T>
	void write(const T value) {
		static_assert(std::is_pod<T>::value);
		write((const uint8_t*)&value, sizeof(T));
	}

	/// Writes the string to the buffer (null terminated if the input string is null terminated)
	template <typename T>
	void write_string(const T& string) {
		write((const uint8_t*)string.data(), string.size());
	}

	/// Writes a null terminated string to the buffer
	template <typename T>
	void write_c_string(const T& string) {
		write_string(string);
		if (string.empty() || string.back() != '\0') {
			write<uint8_t>('\0');
		}
	}

	// Do not use char array.
	template <class T, size_t n>
	void write_string(const T (&string)[n]) {
		write((const uint8_t*)string, n-1);
	}

	// Do not use string literals.
	void write_c_string(const char* string) {
		write((const uint8_t*)string, strlen(string) + 1);
	}

	void write_bw(BinaryWriterT<N>& value) {
		tail->next = value.head;
		tail = value.tail;
		value.head = value.tail = nullptr;
	}

	template <typename T>
	void finish(T& out) {
		for (Chunk* p = head; p; p = p->next) {
			out.write((const char*)p->data, p->size);
		}
	}
};

typedef BinaryWriterT<4000> BinaryWriter;

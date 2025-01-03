#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

#define STORMLIB_NO_AUTO_LINK
#include <StormLib.h>

// A thin wrapper around StormLib https://github.com/ladislav-zezula/StormLib
namespace mpq {
	class File {
	public:
		HANDLE handle = nullptr;
	
		File() = default;
		~File();
		File(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		File(const File&) = default;
		File& operator=(const File&) = default;
		File& operator=(File&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}


		std::vector<uint8_t> read() const;
		std::optional<std::vector<uint8_t>> read2() const;
		size_t size() const;
		void close() const;
	};

	class MPQ {
	public:
		HANDLE handle = nullptr;

		MPQ() = default;

		explicit MPQ(const fs::path& path, unsigned long flags = 0);
		~MPQ();
		MPQ(MPQ&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
		}
		MPQ(const MPQ&) = default;
		MPQ& operator=(const MPQ&) = delete;
		MPQ& operator=(MPQ&& move) noexcept {
			handle = move.handle;
			move.handle = nullptr;
			return *this;
		}

		bool create(const fs::path& path, size_t file_count, bool encrypt);

		bool open(const fs::path& path, unsigned long flags = 0);
		void close();
		bool compact();
		bool unpack(const fs::path& path);

		size_t number() const;
		size_t size() const; 
		void reset(size_t& size) const;


		File file_open(const fs::path& path) const;
		void file_write(const fs::path& path, const std::vector<uint8_t>& data) const;
		void file_remove(const fs::path& path) const;
		void file_rename(const fs::path& source, const fs::path& target) const;
		bool file_exists(const fs::path& path) const;
		void file_add(const fs::path& path, const fs::path& new_path) const;

		

		//return true = delete
		typedef std::function<bool(const std::string& filename)> earch_file_callback;

		void earch_delete_files(const std::string& mark, earch_file_callback callback) const;
	};
}
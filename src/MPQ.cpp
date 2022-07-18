#include "MPQ.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include <StormLib.h>

namespace mpq {
	File::~File() {
		close();
	}

	std::vector<uint8_t> File::read() const {
		const uint32_t size = SFileGetFileSize(handle, nullptr);
		if (size == 0) {
			return {};
		}

		std::vector<uint8_t> buffer(size);

		#ifdef _MSC_VER
		unsigned long bytes_read;
		#else
		unsigned int bytes_read;
		#endif
		const bool success = SFileReadFile(handle, buffer.data(), size, &bytes_read, nullptr);
		if (!success) {
			throw std::runtime_error("Failed to read file: " + std::to_string(GetLastError()));
		}
		return buffer;
	}

	/// An implementation using optional. Use this for all reads?
	std::optional<std::vector<uint8_t>> File::read2() const {
		const uint32_t size = SFileGetFileSize(handle, nullptr);
		if (size == 0) {
			return {};
		}

		std::vector<uint8_t> buffer(size);

		#ifdef _MSC_VER
		unsigned long bytes_read;
		#else
		unsigned int bytes_read;
		#endif
		const bool success = SFileReadFile(handle, buffer.data(), size, &bytes_read, nullptr);
		if (!success) {
			std::cout << "Failed to read file: " << GetLastError() << std::endl;
		}
		return buffer;
	}
	
	size_t File::size() const {
		return SFileGetFileSize(handle, nullptr);
	}

	void File::close() const {
		SFileCloseFile(handle);
	}

	// MPQ

	MPQ::MPQ(const fs::path& path, const unsigned long flags) {
		open(path, flags);
	}

	MPQ::~MPQ() {
		close();
	}


	bool MPQ::create(const fs::path& path, size_t file_count, bool encrypt) {
		SFILE_CREATE_MPQ info;
		ZeroMemory(&info, sizeof(info));

		info.cbSize = sizeof(SFILE_CREATE_MPQ);
		
		if (!encrypt) {
			info.dwFileFlags1 = MPQ_FILE_EXISTS;
			info.dwFileFlags2 = MPQ_FILE_EXISTS;
			info.dwFileFlags3 = MPQ_FILE_EXISTS;
		}
		info.dwAttrFlags = MPQ_ATTRIBUTE_CRC32 | MPQ_ATTRIBUTE_FILETIME | MPQ_ATTRIBUTE_MD5;
		info.dwSectorSize = 0x10000;
		info.dwRawChunkSize = 0;
		info.dwMaxFileCount = file_count;
		if (!SFileCreateArchive2(path.string().c_str(), &info, &handle)) {
			std::cout << "Failed create mpq " << path << std::endl;
			return false;
		}
		return true;
	}
	bool MPQ::open(const fs::path& path, const unsigned long flags) {
		return SFileOpenArchive(path.string().c_str(), 0, flags, &handle);
	}

	File MPQ::file_open(const fs::path& path) const {
		File file;
		#ifdef WIN32
		const bool opened = SFileOpenFileEx(handle, fs::weakly_canonical(path).string().c_str(), 0, &file.handle);
		#else
		const bool opened = SFileOpenFileEx(handle, path.string().c_str(), 0, &file.handle);
		#endif
		if (!opened) {
			throw std::runtime_error("Failed to read file " + path.string() + " with error: " + std::to_string(GetLastError()));
		}
		return file;
	}
	
	void MPQ::file_write(const fs::path& path, const std::vector<uint8_t>& data) const {
		HANDLE out_handle;
		bool success = SFileCreateFile(handle , path.string().c_str(), 0, static_cast<DWORD>(data.size()), 0, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, &out_handle);
		if (!success) {
			std::cout << GetLastError() << " " << path << "\n";
		}

		success = SFileWriteFile(out_handle, data.data(), static_cast<DWORD>(data.size()), MPQ_COMPRESSION_ZLIB);
		if (!success) {
			std::cout << "Writing to file failed: " << GetLastError() << " " << path << "\n";
		}

		success = SFileFinishFile(out_handle);
		if (!success) {
			std::cout << "Finishing write failed: " << GetLastError() << " " << path << "\n";
		}
	}

	void MPQ::file_remove(const fs::path& path) const {
		if (!file_exists(path)) {
			return;
		}
		SFileRemoveFile(handle, path.string().c_str(), 0);
	}

	void MPQ::file_rename(const fs::path& source, const fs::path& target) const {
		if (!file_exists(source)) {
			return;
		}
		SFileRenameFile(handle, source.string().c_str(), target.string().c_str());
	}

	bool MPQ::file_exists(const fs::path& path) const {
	    #ifdef WIN32
		return SFileHasFile(handle, fs::weakly_canonical(path).string().c_str());
		#else
		return SFileHasFile(handle, path.string().c_str());
		#endif
	}

	void MPQ::file_add(const fs::path& path, const fs::path& new_path) const {
		
		auto max = max_size();
		if (size() > max - 10) {
			max *= 2;
			reset_max(max);
		}
		bool success = SFileAddFileEx(handle, path.string().c_str(), new_path.string().c_str(), MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_ZLIB);
	
		if (!success) {
			std::cout << "Error adding file: " << GetLastError() << "\n";
		}
	}


	void MPQ::close() {
		SFileCloseArchive(handle);
		handle = nullptr;
	}

	bool MPQ::compact() {
		return SFileCompactArchive(handle, nullptr, false);
	}

	bool MPQ::unpack(const fs::path& path) {
		SFILE_FIND_DATA file_data;
		HANDLE find_handle = SFileFindFirstFile(handle, "*", &file_data, nullptr);
		fs::create_directories((path / file_data.cFileName).parent_path());
		SFileExtractFile(handle, file_data.cFileName, (path / file_data.cFileName).string().c_str(), SFILE_OPEN_FROM_MPQ);

		while (SFileFindNextFile(find_handle, &file_data)) {
			fs::create_directories((path / file_data.cFileName).parent_path());
			SFileExtractFile(handle, file_data.cFileName, (path / file_data.cFileName).string().c_str(), SFILE_OPEN_FROM_MPQ);
		}
		SFileFindClose(find_handle);

		// Delete unneeded files
		fs::remove(path / "(listfile)");
		fs::remove(path / "(attributes)");
		fs::remove(path / "(war3map.imp)");
		return true;
	}

	void MPQ::earch_delete_files(const std::string& mark, earch_file_callback callback) const {
		SFILE_FIND_DATA file_data;

		HANDLE find_handle = SFileFindFirstFile(handle, mark.c_str(), &file_data, nullptr);

		std::vector<fs::path> delete_list;

		while (SFileFindNextFile(find_handle, &file_data)) {
			if (strlen(file_data.cFileName) > 0 && callback(file_data.cFileName)) {
				delete_list.push_back(file_data.cFileName);
			}
		}
		SFileFindClose(find_handle);

		for (auto& name : delete_list) {
			file_remove(name);
		}
	}


	size_t MPQ::size() const {
		size_t num = 0;

		SFileGetFileInfo(handle, SFileMpqNumberOfFiles, &num, 4, nullptr);
		return num;
	}

	size_t MPQ::max_size() const {
		return SFileGetMaxFileCount(handle);
	}

	
	void MPQ::reset_max(size_t& size) const {

		SFileSetMaxFileCount(handle, size);
	}
}

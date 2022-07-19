
/*

为了解决早期1.31版本里面的 原版sfmpq 的bug  

原版sfmpq 一旦修改过 文件最大数之后 就会打开文件失败， 即时文件是存在的

*/


#include <stormlib.h>

#include <string>
#include <vector>


BOOL WINAPI MpqCompactArchive(HANDLE hMpq) {
	return SFileCompactArchive(hMpq, 0, 0);
}

BOOL WINAPI MpqDeleteFile(HANDLE hMpq, const char* szFileName) {
	if (strcmp(szFileName, "(attributes)") == 0) {
		return TRUE;
	}
	return SFileRemoveFile(hMpq, szFileName, 0);
}

BOOL WINAPI MpqCloseUpdatedArchive(HANDLE hMpq, DWORD dwUnknown) {
	return SFileCloseArchive(hMpq);
}

BOOL WINAPI MpqAddFileToArchiveEx(HANDLE hMpq, LPCSTR szFileName, LPCSTR szArchivedName, DWORD dwFlags, DWORD dwCompressionType, DWORD dwCompressLevel) {
	if (dwFlags & 0x00000001) {
		dwFlags &= ~0x00000001;
		dwFlags |= MPQ_FILE_REPLACEEXISTING;
	}
	return SFileAddFileEx(hMpq, szFileName, szArchivedName, dwFlags, dwCompressionType, dwCompressionType);
}

HANDLE WINAPI MpqOpenArchiveForUpdate(LPCSTR szFileName, DWORD dwFlags, DWORD dwMaximumFilesInArchive) {
	HANDLE hMpq = 0;
	if (SFileOpenArchive(szFileName, 0, 0, &hMpq)) {
		if (dwMaximumFilesInArchive > SFileGetMaxFileCount(hMpq)) {
			SFileSetMaxFileCount(hMpq, dwMaximumFilesInArchive);
		}
		return hMpq;
	}
	return 0;
}

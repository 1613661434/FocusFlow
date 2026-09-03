#include "uninstaller/UninstallerSupport.h"

#include <windows.h>

#include <cwchar>
#include <string>
#include <vector>

namespace {

std::wstring joinPath(const std::wstring &directory,
                      const std::wstring &name)
{
    if (directory.empty()) {
        return name;
    }
    const wchar_t last = directory.back();
    if (last == L'\\' || last == L'/') {
        return directory + name;
    }
    return directory + L'\\' + name;
}

bool pathsEqual(const std::wstring &left, const std::wstring &right)
{
    const std::wstring normalizedLeft =
        UninstallerSupport::normalizePath(left);
    const std::wstring normalizedRight =
        UninstallerSupport::normalizePath(right);
    return !normalizedLeft.empty() && !normalizedRight.empty()
           && _wcsicmp(normalizedLeft.c_str(), normalizedRight.c_str()) == 0;
}

bool isVolumeRoot(const std::wstring &path)
{
    wchar_t volumeRoot[MAX_PATH]{};
    if (!GetVolumePathNameW(path.c_str(), volumeRoot, MAX_PATH)) {
        return true;
    }
    return pathsEqual(path, volumeRoot);
}

bool isRegularFile(const std::wstring &path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES
           && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool removeFileOrLink(const std::wstring &path, DWORD attributes)
{
    SetFileAttributesW(path.c_str(),
                       attributes & ~FILE_ATTRIBUTE_READONLY);
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return RemoveDirectoryW(path.c_str()) != FALSE;
    }
    return DeleteFileW(path.c_str()) != FALSE;
}

bool removeTree(const std::wstring &path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0
        || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
        return removeFileOrLink(path, attributes);
    }

    bool success = true;
    const std::wstring pattern = joinPath(path, L"*");
    WIN32_FIND_DATAW entry{};
    HANDLE search = FindFirstFileW(pattern.c_str(), &entry);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if (std::wcscmp(entry.cFileName, L".") == 0
                || std::wcscmp(entry.cFileName, L"..") == 0) {
                continue;
            }
            if (!removeTree(joinPath(path, entry.cFileName))) {
                success = false;
            }
        } while (FindNextFileW(search, &entry));
        FindClose(search);
    } else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        success = false;
    }

    SetFileAttributesW(path.c_str(),
                       attributes & ~FILE_ATTRIBUTE_READONLY);
    if (!RemoveDirectoryW(path.c_str())
        && GetLastError() != ERROR_PATH_NOT_FOUND) {
        success = false;
    }
    return success;
}

bool isSafeDataDirectory(const std::wstring &path,
                         const std::wstring &expectedPath)
{
    const std::wstring normalized =
        UninstallerSupport::normalizePath(path);
    if (normalized.empty() || isVolumeRoot(normalized)
        || !pathsEqual(normalized, expectedPath)) {
        return false;
    }

    const std::size_t separator = normalized.find_last_of(L"\\/");
    const std::wstring name = separator == std::wstring::npos
                                  ? normalized
                                  : normalized.substr(separator + 1);
    return _wcsicmp(name.c_str(), L"FocusFlow") == 0;
}

} // namespace

namespace UninstallerSupport {

std::wstring normalizePath(const std::wstring &path)
{
    if (path.empty()) {
        return {};
    }
    const DWORD length = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (length == 0) {
        return {};
    }
    std::vector<wchar_t> buffer(length + 1, L'\0');
    if (GetFullPathNameW(path.c_str(),
                         static_cast<DWORD>(buffer.size()),
                         buffer.data(), nullptr) == 0) {
        return {};
    }
    std::wstring normalized(buffer.data());
    while (normalized.size() > 3
           && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }
    return normalized;
}

bool isSafeProgramDirectory(const std::wstring &path)
{
    const std::wstring normalized = normalizePath(path);
    if (normalized.empty() || isVolumeRoot(normalized)) {
        return false;
    }
    return isRegularFile(joinPath(normalized, L"FocusFlow.exe"));
}

bool removeProgramDirectory(const std::wstring &path)
{
    const std::wstring normalized = normalizePath(path);
    return isSafeProgramDirectory(normalized) && removeTree(normalized);
}

bool removeDataDirectory(const std::wstring &path,
                         const std::wstring &expectedPath)
{
    const std::wstring normalized = normalizePath(path);
    return isSafeDataDirectory(normalized, expectedPath)
           && removeTree(normalized);
}

} // namespace UninstallerSupport

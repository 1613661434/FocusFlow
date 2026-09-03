#pragma once

#include <string>

namespace UninstallerSupport {

std::wstring normalizePath(const std::wstring &path);
bool isSafeProgramDirectory(const std::wstring &path);
bool removeProgramDirectory(const std::wstring &path);
bool removeDataDirectory(const std::wstring &path,
                         const std::wstring &expectedPath);

} // namespace UninstallerSupport

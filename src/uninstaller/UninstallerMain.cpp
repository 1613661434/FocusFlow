#include "uninstaller/UninstallerSupport.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <cwchar>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kTitle[] = L"卸载 FocusFlow";

std::wstring modulePath()
{
    std::vector<wchar_t> buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    return length > 0 && length < buffer.size()
               ? std::wstring(buffer.data(), length)
               : std::wstring();
}

std::wstring parentDirectory(const std::wstring &path)
{
    const std::size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos
               ? std::wstring()
               : path.substr(0, separator);
}

std::wstring dataDirectory()
{
    PWSTR roamingData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData,
                                    KF_FLAG_DEFAULT, nullptr,
                                    &roamingData))) {
        return {};
    }
    std::wstring path(roamingData);
    CoTaskMemFree(roamingData);
    return path + L"\\FocusFlow";
}

bool focusFlowIsRunning()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return true;
    }

    PROCESSENTRY32W process{};
    process.dwSize = sizeof(process);
    bool running = false;
    if (Process32FirstW(snapshot, &process)) {
        do {
            if (_wcsicmp(process.szExeFile, L"FocusFlow.exe") == 0) {
                running = true;
                break;
            }
        } while (Process32NextW(snapshot, &process));
    }
    CloseHandle(snapshot);
    return running;
}

std::wstring quote(const std::wstring &value)
{
    return L"\"" + value + L"\"";
}

void waitForProcess(DWORD processId)
{
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
    if (process != nullptr) {
        WaitForSingleObject(process, 15000);
        CloseHandle(process);
    } else {
        Sleep(1200);
    }
}

int performUninstall(DWORD parentProcessId,
                     const std::wstring &programDirectory,
                     const std::wstring &requestedDataDirectory,
                     const std::wstring &temporaryExecutable)
{
    waitForProcess(parentProcessId);

    const std::wstring expectedDataDirectory = dataDirectory();
    const bool dataRemoved =
        UninstallerSupport::removeDataDirectory(
            requestedDataDirectory, expectedDataDirectory);
    const bool programRemoved =
        UninstallerSupport::removeProgramDirectory(programDirectory);

    if (dataRemoved && programRemoved) {
        MessageBoxW(nullptr,
                    L"FocusFlow 已卸载，本地数据库、设置、托管声音和备份也已删除。",
                    kTitle, MB_OK | MB_ICONINFORMATION);
    } else {
        const std::wstring message =
            L"卸载没有完全完成。请确认 FocusFlow 已退出，然后手动检查：\n\n程序目录：\n"
            + programDirectory + L"\n\n本地数据目录：\n"
            + expectedDataDirectory;
        MessageBoxW(nullptr, message.c_str(), kTitle,
                    MB_OK | MB_ICONERROR);
    }

    const std::wstring currentExecutable = modulePath();
    const std::wstring normalizedCurrent =
        UninstallerSupport::normalizePath(currentExecutable);
    const std::wstring normalizedTemporary =
        UninstallerSupport::normalizePath(temporaryExecutable);
    if (!normalizedCurrent.empty() && !normalizedTemporary.empty()
        && _wcsicmp(normalizedCurrent.c_str(),
                    normalizedTemporary.c_str()) == 0) {
        MoveFileExW(currentExecutable.c_str(), nullptr,
                    MOVEFILE_DELAY_UNTIL_REBOOT);
    }
    return dataRemoved && programRemoved ? 0 : 1;
}

bool launchTemporaryUninstaller(const std::wstring &programDirectory,
                                const std::wstring &requestedDataDirectory)
{
    wchar_t temporaryDirectory[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, temporaryDirectory) == 0) {
        return false;
    }

    const std::wstring temporaryExecutable =
        std::wstring(temporaryDirectory)
        + L"FocusFlow-Uninstall-"
        + std::to_wstring(GetCurrentProcessId()) + L"-"
        + std::to_wstring(GetTickCount64()) + L".exe";
    const std::wstring currentExecutable = modulePath();
    if (currentExecutable.empty()
        || !CopyFileW(currentExecutable.c_str(),
                      temporaryExecutable.c_str(), FALSE)) {
        return false;
    }

    std::wstring commandLine =
        quote(temporaryExecutable) + L" --perform "
        + std::to_wstring(GetCurrentProcessId()) + L" "
        + quote(programDirectory) + L" "
        + quote(requestedDataDirectory) + L" "
        + quote(temporaryExecutable);
    std::vector<wchar_t> mutableCommand(commandLine.begin(),
                                        commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL started = CreateProcessW(
        temporaryExecutable.c_str(), mutableCommand.data(), nullptr, nullptr,
        FALSE, 0, nullptr, temporaryDirectory, &startup, &process);
    if (!started) {
        DeleteFileW(temporaryExecutable.c_str());
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int argumentCount = 0;
    LPWSTR *arguments = CommandLineToArgvW(GetCommandLineW(),
                                           &argumentCount);
    if (arguments != nullptr && argumentCount == 6
        && std::wcscmp(arguments[1], L"--perform") == 0) {
        const DWORD parentProcessId = static_cast<DWORD>(
            std::wcstoul(arguments[2], nullptr, 10));
        const std::wstring programDirectory(arguments[3]);
        const std::wstring requestedDataDirectory(arguments[4]);
        const std::wstring temporaryExecutable(arguments[5]);
        LocalFree(arguments);
        return performUninstall(parentProcessId, programDirectory,
                                requestedDataDirectory,
                                temporaryExecutable);
    }
    if (arguments != nullptr) {
        LocalFree(arguments);
    }

    const std::wstring executable = modulePath();
    const std::wstring programDirectory = parentDirectory(executable);
    const std::wstring requestedDataDirectory = dataDirectory();
    if (!UninstallerSupport::isSafeProgramDirectory(programDirectory)
        || requestedDataDirectory.empty()) {
        MessageBoxW(nullptr,
                    L"无法确认 FocusFlow 的程序目录或本地数据目录，"
                    L"为防止误删，本次卸载已经取消。",
                    kTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    if (focusFlowIsRunning()) {
        MessageBoxW(nullptr,
                    L"FocusFlow 正在运行。请先在系统托盘图标的右键菜单中选择“退出”，"
                    L"再重新运行卸载程序。\n\n为保护数据库，本次没有删除任何内容。",
                    kTitle, MB_OK | MB_ICONWARNING);
        return 2;
    }

    const int confirmation = MessageBoxW(
        nullptr,
        L"此操作将永久删除：\n\n"
        L"• 当前程序目录中的 FocusFlow 及运行组件\n"
        L"• 本地数据库中的任务、项目和专注记录\n"
        L"• 应用设置、托管提示音和自动备份\n\n"
        L"删除后无法恢复，建议先在设置页面备份数据库。是否继续卸载？",
        kTitle, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (confirmation != IDYES) {
        return 0;
    }

    if (!launchTemporaryUninstaller(programDirectory,
                                    requestedDataDirectory)) {
        MessageBoxW(nullptr,
                    L"无法启动卸载清理程序，没有删除任何内容。",
                    kTitle, MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}

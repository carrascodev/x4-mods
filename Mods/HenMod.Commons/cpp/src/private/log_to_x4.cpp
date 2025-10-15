#include "../public/log_to_x4.h"
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <shlobj.h>
#include <sstream>
#include <windows.h>

using namespace std;

namespace LogToX4 {

static string g_module_dir;
static string g_log_timestamp;

string GetLogDir() {
  if (!g_module_dir.empty())
    return g_module_dir;

  // Use Documents folder for logs - more reliable than mod directory
  char documentsPath[MAX_PATH];
  if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath) == S_OK) {
    g_module_dir = string(documentsPath) + "\\Egosoft\\X4\\logs\\HenMod";
    // Create directory if it doesn't exist
    CreateDirectoryA(g_module_dir.c_str(), NULL);
  } else {
    g_module_dir = ".";
  }
  return g_module_dir;
}

string GetLogTimestamp() {
  if (!g_log_timestamp.empty())
    return g_log_timestamp;

  time_t now = time(nullptr);
  tm *t = localtime(&now);
  char buf[20];
  strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", t);
  g_log_timestamp = string(buf);
  return g_log_timestamp;
}

void AppendToModLog(const string &s) {
  // For debugging in X4, use OutputDebugString instead of file
  // Original file logging commented out
  string logfile = GetLogDir() + "\\henmod" + GetLogTimestamp() + ".log";

  // Use Windows API for file I/O
  HANDLE hFile = CreateFileA(logfile.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    string logEntry = s + "\n";
    DWORD bytesWritten;
    WriteFile(hFile, logEntry.c_str(), logEntry.length(), &bytesWritten, NULL);
    CloseHandle(hFile);
  }
}

static string vformat(const char *fmt, va_list ap) {
  char buf[4096];
  vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
  return string(buf);
}

void Log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  string msg = vformat(fmt, ap);
  va_end(ap);
  OutputDebugStringA(msg.c_str());
  OutputDebugStringA("\n");
  AppendToModLog(msg);
}

} // namespace LogToX4
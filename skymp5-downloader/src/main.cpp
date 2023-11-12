#include <fstream>
#include <httplib.h>
#include <sstream>
#include <thread>
#include <windows.h>

#include <commctrl.h>

constexpr auto HOSTNAME = "sweettaffy.b-cdn.net";

// Constants for file URLs and paths
const char* URLS[] = {
  "https://sweettaffy.b-cdn.net/InstallSkyMP-0d0b740.exe",
  "https://sweettaffy.b-cdn.net/InstallSkyMP-0d0b740-1.bin",
  "https://sweettaffy.b-cdn.net/InstallSkyMP-0d0b740-2.bin"
};

const char* FILES[] = { "InstallSkyMP-0d0b740.exe",
                        "InstallSkyMP-0d0b740-1.bin",
                        "InstallSkyMP-0d0b740-2.bin" };

// Function Declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DownloadFile(const std::string& url, const std::string& path,
                  HWND hwndProgressBar, int fileIndex, int numFiles);

// Global variables for window handles, etc.
HWND hwndProgressBar;
HWND hwndMainWindow;

enum class DiskSpaceCheckResult
{
  EnoghSpace,
  NotEnoughSpace,
  FailedToCheck
};

// Forward declarations of functions
void CreateMainWindow(HINSTANCE hInstance);
void CreateProgressBar(HWND hwndParent);
DiskSpaceCheckResult CheckDiskSpace(const std::string& directoryPath);

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
  // Initialize common controls
  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icc.dwICC = ICC_PROGRESS_CLASS; // Initialize progress bar control
  InitCommonControlsEx(&icc);

  // Initialize and show window with progress bar
  CreateMainWindow(hInstance);
  CreateProgressBar(hwndMainWindow);
  ShowWindow(hwndMainWindow, iCmdShow);
  UpdateWindow(hwndMainWindow);

  // Start download in a separate thread
  std::thread downloadThread([&] {
    // Check if there is enough disk space (at least 10 GB)
    DiskSpaceCheckResult checkResult = CheckDiskSpace(".");

    switch (checkResult) {
      case DiskSpaceCheckResult::EnoghSpace:
        break;
      case DiskSpaceCheckResult::NotEnoughSpace:
        MessageBoxA(
          0,
          "Not enough disk space. At least 10 GB of free space is required.",
          "Error", MB_OK | MB_ICONERROR);
        std::exit(0);
        break;
      case DiskSpaceCheckResult::FailedToCheck:
        MessageBoxA(0, "Failed to check disk space.", "Error",
                    MB_OK | MB_ICONERROR);
        std::exit(0);
        break;
      default:
        break;
    }

    for (int i = 0; i < std::size(URLS); ++i) {
      DownloadFile(URLS[i], FILES[i], hwndProgressBar, i, std::size(URLS));
    }

    // Execute the downloaded file
    HINSTANCE hInst =
      ShellExecuteA(NULL, "open", FILES[0], NULL, NULL, SW_SHOWNORMAL);

    // Cast to an error code
    int errorCode = (int)(uintptr_t)hInst;

    // Check if execution failed
    if (errorCode <= 32) {
      const char* errorMessage = "An unknown error occurred.";
      switch (errorCode) {
        case ERROR_FILE_NOT_FOUND:
          errorMessage = "The specified file was not found.";
          break;
        case ERROR_PATH_NOT_FOUND:
          errorMessage = "The specified path was not found.";
          break;
        case ERROR_BAD_FORMAT:
          errorMessage = "The .exe file is corrupt or invalid.";
          break;
          // You can add more cases for specific errors if needed
      }

      MessageBoxA(NULL, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }

    Sleep(2000);
    std::exit(0);
  });
  downloadThread.detach();

  // Message loop
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return static_cast<int>(msg.wParam);
}

// Function to create main window
void CreateMainWindow(HINSTANCE hInstance)
{
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  int windowWidth = 280;
  int windowHeight = 85;
  int windowPosX = (screenWidth - windowWidth) / 2;
  int windowPosY = (screenHeight - windowHeight) / 2;

  // Define a window class
  WNDCLASS wc = {};
  wc.lpfnWndProc = WndProc; // Pointer to your window procedure
  wc.hInstance = hInstance;
  wc.lpszClassName = "SkympDownloaderClass";
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.hIcon = nullptr;

  // Register the window class
  RegisterClass(&wc);

  LPCWSTR lpClassName = L"SkympDownloaderClass";
  LPCWSTR lpWindowName = L"Skymp Downloader";

  // Create the window
  hwndMainWindow = CreateWindowW(lpClassName,         // Window class name
                                 lpWindowName,        // Window title
                                 WS_OVERLAPPEDWINDOW, // Window style
                                 windowPosX, windowPosY, windowWidth,
                                 windowHeight, // Position and size
                                 NULL,         // Parent window
                                 NULL,         // Menu
                                 hInstance,    // Instance handle
                                 NULL          // Additional application data
  );
}

void CreateProgressBar(HWND hwndParent)
{
  hwndProgressBar = CreateWindowEx(
    0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 10, 10, 200,
    30, hwndParent, NULL, GetModuleHandle(NULL), NULL);

  // Set the range and step of the progress bar
  SendMessage(hwndProgressBar, PBM_SETRANGE, 0,
              MAKELPARAM(0, 10000));                       // Range 0-100
  SendMessage(hwndProgressBar, PBM_SETSTEP, (WPARAM)1, 0); // Step increment
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_CREATE:
      // Handle window creation
      break;

    case WM_COMMAND:
      // Handle commands, like button presses
      break;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      // Add any drawing code here...
      EndPaint(hwnd, &ps);
    } break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_GETMINMAXINFO: {
      LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
      lpMMI->ptMinTrackSize.x = 280;
      lpMMI->ptMinTrackSize.y = 85;
      lpMMI->ptMaxTrackSize.x = 280;
      lpMMI->ptMaxTrackSize.y = 85;
      break;
    }

    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

void DownloadFile(const std::string& url, const std::string& path,
                  HWND hwndProgressBar, int fileIndex, int numFiles)
{
  httplib::Client cli(HOSTNAME);
  std::ofstream outFile(path, std::ios::binary);

  if (!outFile) {
    MessageBoxA(0, "Failed to open file for writing.", "Error",
                MB_OK | MB_ICONERROR);
    return;
  }

  auto res = cli.Get(
    url.c_str(),
    [&](const char* data, size_t len) {
      outFile.write(data, len);

      if (!outFile.good()) {
        MessageBoxA(0, "Failed to write data to file.", "Error",
                    MB_OK | MB_ICONERROR);
        outFile.close();
        return false; // Stop the download
      }

      return true; // Return false to cancel
    },
    [&](uint64_t current, uint64_t total) { // Calculate the progress
      int progress =
        static_cast<int>((static_cast<double>(current) / total) * 10000);

      float percentage = (static_cast<double>(current) / total * 100);

      std::ostringstream stream;
      stream << std::fixed << std::setprecision(2) << percentage << "% "
             << (fileIndex + 1) << "/" << numFiles;
      std::string newTitle = stream.str();

      //   Update the window's title
      SetWindowTextA(hwndMainWindow, newTitle.c_str());

      // Update progress bar
      SendMessage(hwndProgressBar, PBM_SETPOS, progress, 0);
      return true;
    });

  outFile.close();

  if (res) {
    if (res->status != 200) {
      std::string errorMessage =
        "Failed to download file. Server responded with status code: " +
        std::to_string(res->status);
      MessageBoxA(0, errorMessage.c_str(), "Error", MB_OK | MB_ICONERROR);
    }
    // Additional checks for other HTTP statuses can be handled here
  } else {
    MessageBoxA(0, "Failed to download file. No response from server.",
                "Error", MB_OK | MB_ICONERROR);
  }
}

DiskSpaceCheckResult CheckDiskSpace(const std::string& directoryPath)
{
  ULARGE_INTEGER freeBytesAvailableToUser;
  ULARGE_INTEGER totalNumberOfBytes;
  ULARGE_INTEGER totalNumberOfFreeBytes;

  // If the directory is empty, GetDiskFreeSpaceEx will check the current drive
  BOOL result = GetDiskFreeSpaceExA(
    directoryPath.empty() ? NULL : directoryPath.c_str(),
    &freeBytesAvailableToUser, &totalNumberOfBytes, &totalNumberOfFreeBytes);

  if (result) {
    const ULONGLONG tenGigabytes = 10ULL * 1024 * 1024 * 1024;
    if (freeBytesAvailableToUser.QuadPart < tenGigabytes) {
      return DiskSpaceCheckResult::NotEnoughSpace;
    }
    return DiskSpaceCheckResult::EnoghSpace;
  }

  return DiskSpaceCheckResult::NotEnoughSpace;
}

#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <fstream> 
#include <Mmdeviceapi.h>
#include <endpointvolume.h>
// 添加头文件
#include <WinUser.h>
#pragma comment(lib, "Winmm.lib")
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// 声明监测的进程名称和声音文件名称
const wchar_t* PROCESS_NAME = L"rtcRemoteDesktop.exe";
const wchar_t* SOUND_FILE_START = L"start.wav";
const wchar_t* SOUND_FILE_END = L"end.wav";

// 声明监测时间段
const int START_HOUR = 17;
const int START_MINUTE = 0;
const int END_HOUR = 22;
const int END_MINUTE = 0;

void setVolume(float volume) {
    HRESULT hr = S_OK;
    CoInitialize(NULL);

    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

    IAudioEndpointVolume* pAudioEndpointVolume = NULL;
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);

    hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(volume, NULL);

    pAudioEndpointVolume->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
}//改变系统音量

bool isMonitoringTime() {
    SYSTEMTIME currentTime;
    GetLocalTime(&currentTime);
    int currentHour = currentTime.wHour;
    int currentMinute = currentTime.wMinute;
    if ((currentHour > START_HOUR || (currentHour == START_HOUR && currentMinute >= START_MINUTE))
        && (currentHour < END_HOUR || (currentHour == END_HOUR && currentMinute < END_MINUTE))) {
        return true;
    }
    return false;
}

void playSound(const wchar_t* soundFile) {
    setVolume(0.3f);
    //播放声音文件
    PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC);
}

//将日志信息写入文件
void logToFile(const std::string& message) {
    std::ofstream logFile("install.txt", std::ios_base::app);
    if (!logFile.is_open()) {
        // 文件不存在，则创建一个新文件
        logFile.open("install.txt", std::ios_base::out);
    }
    logFile << message << std::endl;
    logFile.close();
}

//判断进程是否正在运行
bool isProcessRunning(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &entry)) {
            do {
                if (wcscmp(entry.szExeFile, processName) == 0) {
                    CloseHandle(hSnapshot);
                    return true;
                }
            } while (Process32Next(hSnapshot, &entry));
        }
        CloseHandle(hSnapshot);
    }
    return false;
}

//监测进程



// 判断是否有鼠标点击或移动事件
bool isMouseActive() {
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lastInputInfo)) {
        DWORD lastInputTime = lastInputInfo.dwTime;
        DWORD currentTime = GetTickCount();
        if (currentTime - lastInputTime < 300000) {
            return true;
        }
    }
    return false;
}

// 在监测进程时加入判断是否有鼠标点击或移动事件
void monitorProcess(const wchar_t* processName, const wchar_t* startSoundFile, const wchar_t* endSoundFile) {
    bool wasRunning = false;
    while (true) {
        bool isMonitoring = isMonitoringTime();
        bool isRunning = isProcessRunning(processName);
        bool isMouseactive = isMouseActive(); // 添加判断是否有鼠标点击或移动事件

        if (isMonitoring && !isMouseactive) { 
            SYSTEMTIME currentTime;
            GetLocalTime(&currentTime);
            int currentHour = currentTime.wHour;
            int currentMinute = currentTime.wMinute;


            if (isRunning && !wasRunning) {
                playSound(startSoundFile);
                std::cout << "start:\n";
                std::string logMessage = std::to_string(currentHour) + ":" + std::to_string(currentMinute) + ":  Process started on " + std::to_string(currentTime.wDay) + "/" + std::to_string(currentTime.wMonth) + "/" + std::to_string(currentTime.wYear);
                logToFile(logMessage);
            }
            else if (!isRunning && wasRunning) {
                playSound(endSoundFile);
                std::cout << "end:\n";
                std::string logMessage = std::to_string(currentHour) + ":" + std::to_string(currentMinute) + ":  Process ended on " + std::to_string(currentTime.wDay) + "/" + std::to_string(currentTime.wMonth) + "/" + std::to_string(currentTime.wYear);
            }
        }

        wasRunning = isRunning;
        Sleep(1000);
    }
}


int main() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"Global\\MyApplicationMutex");
    if (hMutex == NULL) {
        // 处理错误
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // 程序已经在运行
        MessageBox(NULL, L"The program is already running.", L"Error", MB_OK | MB_ICONERROR);
        CloseHandle(hMutex);
        return 0;
    }
    // 加锁互斥量
    WaitForSingleObject(hMutex, INFINITE);
    // 监测进程
    monitorProcess(PROCESS_NAME, SOUND_FILE_START, SOUND_FILE_END);
    // 释放互斥量并关闭句柄
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}


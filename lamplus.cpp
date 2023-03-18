#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <fstream> // 添加头文件
#include <Mmdeviceapi.h>
#include <endpointvolume.h>

#pragma comment(lib, "Winmm.lib")
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// 声明监测的进程名称和声音文件名称
const wchar_t* PROCESS_NAME = L"rtcRemoteDesktop.exe";
const wchar_t* SOUND_FILE_START = L"start.wav";
const wchar_t* SOUND_FILE_END = L"end.wav";

// 第一个时间段的开始和结束时间
const int START_HOUR_1 = 6;
const int START_MINUTE_1 = 30;
const int END_HOUR_1 = 7;
const int END_MINUTE_1 = 25;

// 第二个时间段的开始和结束时间
const int START_HOUR_2 = 17;
const int START_MINUTE_2 = 0;
const int END_HOUR_2 = 22;
const int END_MINUTE_2 = 0;

bool isMonitoringTime() {
    SYSTEMTIME currentTime;
    GetLocalTime(&currentTime);
    int currentHour = currentTime.wHour;
    int currentMinute = currentTime.wMinute;
    if ((currentHour > START_HOUR_1 || (currentHour == START_HOUR_1 && currentMinute >= START_MINUTE_1))
        && (currentHour < END_HOUR_1 || (currentHour == END_HOUR_1 && currentMinute < END_MINUTE_1))) {
        return true;
    }
    if ((currentHour > START_HOUR_2 || (currentHour == START_HOUR_2 && currentMinute >= START_MINUTE_2))
        && (currentHour < END_HOUR_2 || (currentHour == END_HOUR_2 && currentMinute < END_MINUTE_2))) {
        return true;
    }
    return false;
}

int main() {



    // 进入监测循环
    bool wasRunning = false;
    while (true) {
        // 检查是否在监测时间段内
        bool isMonitoring = isMonitoringTime();

        // 检查进程是否在运行，并根据进程是否在运行播放声音和隐藏窗口
        bool isRunning = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 entry;
            entry.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &entry)) {
                do {
                    if (wcscmp(entry.szExeFile, PROCESS_NAME) == 0) {
                        isRunning = true;
                        break;
                    }
                } while (Process32Next(hSnapshot, &entry));
            }
            CloseHandle(hSnapshot);
        }

        if (isMonitoring) {

            HRESULT hr = S_OK;
            CoInitialize(NULL);

            IMMDeviceEnumerator* pEnumerator = NULL;
            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

            IMMDevice* pDevice = NULL;
            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

            IAudioEndpointVolume* pAudioEndpointVolume = NULL;
            hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);

            float volume = 0.3f; // 设置音量为 30%
            hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(volume, NULL);

            pAudioEndpointVolume->Release();
            pDevice->Release();
            pEnumerator->Release();
            CoUninitialize();

            //调节系统音量







            SYSTEMTIME currentTime;
            GetLocalTime(&currentTime);
            int currentday = currentTime.wDay;
            int currentHour = currentTime.wHour;
            int currentMinute = currentTime.wMinute;

            //定义时间

            if (isRunning && !wasRunning) {
                PlaySound(SOUND_FILE_START, NULL, SND_FILENAME | SND_ASYNC);
                std::cout << "start:\n";
                // 添加输出日志到文件的代码
                std::ofstream logFile("install.txt", std::ios_base::app);
                if (logFile.is_open()) {
                    logFile << "start: " << currentday << "日： " << currentHour << "： " << currentMinute << '\n';
                    logFile.close();
                }
                else {
                    std::cerr << "Unable to open log file\n";  //好像屁用么得 但我也不敢删 不知道会不会报错
                }
            }
            else if (!isRunning && wasRunning) {
                PlaySound(SOUND_FILE_END, NULL, SND_FILENAME | SND_ASYNC);
                std::cout << "end:\n";
                // 添加输出日志到文件的代码
                std::ofstream logFile("install.txt", std::ios_base::app);
                if (logFile.is_open()) {
                    logFile << "end: " << currentday << "日： " << currentHour << "： " << currentMinute << '\n';
                    logFile.close();
                }
                else {
                    std::cerr << "Unable to open log file\n";   //好像屁用么得 但我也不敢删 不知道会不会报错
                }
            }
        }


        // 更新 wasRunning 变量并等待一段时间再次检查
        wasRunning = isRunning;
        Sleep(500);
    }

    return 0;
}

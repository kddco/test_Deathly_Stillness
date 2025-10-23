#include "windows.h" //Windows API Library
#include <stdio.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>


//#include "SDK.hpp"
// dll注入的loader，是一個exe檔

// 使用 CreateRemoteThread 方法 注入Dll
DWORD InjectDll(HANDLE process, LPCTSTR dllPath)
{
	try {
		OutputDebugStringA("InjectDll START");
		DWORD_PTR dllPathSize = ((DWORD)wcslen(dllPath) + 1) * sizeof(TCHAR);
		printf("dllPathSize:%d %ws\n", dllPathSize, dllPath);
		//對目標Process 申請用來存放DLL 路徑的記憶體
		//LPVOID WINAPI VirtualAllocEx(
		//	_In_     HANDLE hProcess,  //目標process handledllPathSize
		//	_In_opt_ LPVOID lpAddress, //指定的記憶體位置 , 可填NULL 讓系統分配
		//	_In_     SIZE_T dwSize,    //欲申請的記憶體大小
		//	_In_     DWORD  flAllocationType, //分配的型態
		//	_In_     DWORD  flProtect         //欲申請的記憶體權限(REW)
		//);

		void* remote_memory_dll_path = VirtualAllocEx(process, NULL, dllPathSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (remote_memory_dll_path == NULL)
		{
			printf("申請記憶體失敗  ErrorCode：%d\n", GetLastError());
			OutputDebugStringA("申請記憶體失敗  ErrorCode： % d\n");
			return 0;
		}

		// 寫入DLL路徑
		if (WriteProcessMemory(process, remote_memory_dll_path, dllPath, dllPathSize, NULL) == FALSE)
		{
			printf("寫入記憶體失敗 ErrorCode：%d\n", GetLastError());
			OutputDebugStringA("申請記憶體失敗  ErrorCode： % d\n");
			return 0;
		}

		//HANDLE CreateRemoteThread(
		//	HANDLE                 hProcess,  //目標process handle
		//	LPSECURITY_ATTRIBUTES  lpThreadAttributes, //特殊的權限 ,這裡不使用 , 填NULL即可
		//	SIZE_T                 dwStackSize, //需要的Stack大小  , 填NULL使用系統預設即可
		//	LPTHREAD_START_ROUTINE lpStartAddress,//建立Thread後要執行的函數位置 , 這裡我們使用 LoadLibrary
		//	LPVOID                 lpParameter,   //建立Thread後的附加參數 , 這裡填上我們申請好 , 並寫入Dll 路徑的記憶體位置(remote_memory_dll_path)
		//	DWORD                  dwCreationFlags, //建立時的Flag , 填NULL會立即執行
		//	LPDWORD                lpThreadId       //建立後的ThreadID , 這裡不使用 , 填NULL即可
		//);
		//迫使目標 Process 建立 RemoteThread , 執行 LoadLibrary 函數 , 載入我們指定的Dll
		HANDLE remoteThread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibrary, remote_memory_dll_path, NULL, NULL);
		if (remoteThread == NULL)
		{
			printf("建立 Remote Thread 失敗 ErrorCode：%d\n", GetLastError());
			return NULL;
		}
		else
		{
			printf("Inject Dll %ws Success!!\n", dllPath);
		}

		DWORD remoteModule = NULL;

		//等待remoteThread 執行結束 , 才繼續程式執行
		WaitForSingleObject(remoteThread, INFINITE);

		//取得DLL Image Base.
		GetExitCodeThread(remoteThread, &remoteModule);

		//Free Handle & DLl path Memory
		CloseHandle(remoteThread);
		VirtualFreeEx(process, remote_memory_dll_path, dllPathSize, MEM_DECOMMIT);

		return remoteModule;
	}
	catch (const std::exception& e) {
		// 捕捉並處理其他標準異常
		std::string debugMessage = "InjectDll 捕捉到异常: ";
		debugMessage += e.what();
		printf("%s", debugMessage);
		OutputDebugStringA(debugMessage.c_str());
	}
}
uintptr_t GetProcessBaseAddress(DWORD processID) {
	uintptr_t baseAddress = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 me;
		me.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(hSnapshot, &me)) {
			baseAddress = (uintptr_t)me.modBaseAddr;
		}
		CloseHandle(hSnapshot);
	}
	return baseAddress;
}
int main()
{

	//HWND FindWindowA(
	//LPCSTR lpClassName, 
	//LPCSTR lpWindowName //Game Windows Title);
	//HWND hWnd = FindWindow(NULL, L"VirtuaCop 2");//Game Window Title to Hwnd handle
	HWND hWnd = FindWindow(NULL, L"DeathlyStillnessGame  ");//Game Window Title to Hwnd handle


	if (hWnd)
	{
		printf("Find Game Windows Hwnd\n");
		DWORD game_pid = NULL;
		GetWindowThreadProcessId(hWnd, &game_pid);//hwnd to PID
		printf("game_pid:%d\n", game_pid);
		DWORD game_image_base = GetProcessBaseAddress(game_pid);// Hwnd to 遊戲 image address
		printf("game_image_base:0x%x\n", game_image_base);



		//HANDLE OpenProcess(
		//DWORD dwDesiredAccess, //要求的 Process Handle 權限  
		//BOOL  bInheritHandle,  //此Handle 是否需要被 child process 繼承
		//DWORD dwProcessId      //欲請求 process Handle 的目標PID);

		//(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION)
		HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, game_pid);
		if (pHandle)
		{


			//DWORD_PTR dll_base = InjectDll(pHandle, L"C:\\Users\\kddc\\source\\repos\\test_Deathly_Stillness\\x64\\Release\\Dll1.dll");
			DWORD_PTR dll_base = InjectDll(pHandle, L"E:\\舊電腦_C槽\\repos\\test_Deathly_Stillness\\x64\\Release\\Dll1.dll");
			//DWORD_PTR dll_base = InjectDll(pHandle, L"E:\\舊電腦_C槽\\repos\\draw_dll-claude - 成功可以繪製紅心\\x64\\Debug\\draw_dll.dll");

		
			printf("dll_base:0x%x\n", dll_base);

			printf("injecter loaded\n");
			OutputDebugStringA("injecter loaded\n");

			CloseHandle(pHandle);//關閉 process Handle
		}
		else
		{
			printf("OpenProcess Handle Fail\n");
		}
	}
	else
	{
		printf("Please Open Game.\n");
	}

	printf("waiting for end...");
	getchar();//PAUSE for input
	return 0;
}
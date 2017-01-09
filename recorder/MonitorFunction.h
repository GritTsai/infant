// MonitorFunction.h: interface for the CMonitorFunction class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MONITORFUNCTION_H__4F2DEA57_AA5B_4BF5_A989_EED9B12C93BC__INCLUDED_)
#define AFX_MONITORFUNCTION_H__4F2DEA57_AA5B_4BF5_A989_EED9B12C93BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "winsock.h"
#include "afxmt.h"

#define DEV_DST_IP "192.168.3.30"
#define WAV_FILE_FOLDER "D:\\soundfielddir\\"
//#define WAV_FILE_FOLDER "C:\\Users\\ECAIJUN\\Downloads\\"
const int Fpoint = 2048;

#define NUM_CHANNEL 16 //采样通道数

#define SOCK_MESSAGE WM_USER+101
#define WM_UPDATE_LOG WM_USER+102

class CMonitorFunction  
{
public:
	CMonitorFunction();
	virtual ~CMonitorFunction();

	BOOL InitAndConnet(HWND hwnd);
	BOOL SendDataToServer(char *msg, int msgLen, int code);

	//static variables
	static float s_duration;

	static CSemaphore s_buffer_available;
	static CSemaphore s_data_ready;
	static CMutex s_mutex;
	//static CMutex s_card_mutex;

	static float *s_pSoundBuffer;
	static int s_freq;    //采样频率
	static int s_ch_num_samp;
	static int s_num_samp; // 采样频率 * 16通道 * 采样时间长度 = 采样个数
	static float *s_pData; //malloc buffer for samp
	static float *s_value_ch[NUM_CHANNEL]; // 16 通道采样值

	//static functions 
	static DWORD WINAPI SoundCollectThread(LPVOID lpParameter);
	static DWORD WINAPI SoundHandleThread(LPVOID lpParameter); 

private:
	void AsyncSocketInit();

public:
	SOCKET m_hSocket;
	sockaddr_in m_addr;
    HWND m_hWnd;
};

#endif // !defined(AFX_MONITORFUNCTION_H__4F2DEA57_AA5B_4BF5_A989_EED9B12C93BC__INCLUDED_)

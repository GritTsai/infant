// MonitorFunction.cpp: implementation of the CMonitorFunction class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MonitorFunction.h"
#include "recorder.h"
#include "recorderDlg.h"

#include <stdlib.h>
#include<stdio.h>
#include <math.h>
#include <string.h>
#include<string>
using std::string;
#include <Mmreg.h>
#include <deque>
#include <queue>
#include<vector>
using std::vector;
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define PORT 5050
unsigned int ReSetCardV6360(char *destIP);//�����忨
int ADStopReceive(void);
int ReadAdBufData(float* databuf, int num);
int GetAdBufSize(void);
int AD_Config(char *destIP,int ad_os,int gain,int ad_freq);

void  AD_DataReceive(void * pParam);

CRITICAL_SECTION critical_sec;
HANDLE hAdThread = NULL;
int AD_scan_stop = 0;
std::queue<float> AdDataBuf;

int ad_range=0;//��������
int ad_osample=0;
int ad_frequency=20480;
int continuea=0;
int errTimeout=1000;//һ�����ݲ���ʱ��
int recont_times = 0; //�����Ĵ���
int Id_Num;           //�忨ID
int stop_flag = 0;
int connect_flag =0;   //��������
int change_flag =0;   //�޸�ip
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////
//init static variables

float CMonitorFunction::s_duration = 0.1f;

CSemaphore CMonitorFunction::s_buffer_available(1, 1);
CSemaphore CMonitorFunction::s_data_ready(0, 1);
CMutex CMonitorFunction::s_mutex;

//CMutex CMonitorFunction::s_card_mutex;

float *CMonitorFunction::s_pSoundBuffer = NULL;
int CMonitorFunction::s_freq = 20480;    //����Ƶ��
int Pow_N = (int)floor(log(0.0+CMonitorFunction::s_freq * CMonitorFunction::s_duration)/log(2.0));
int CMonitorFunction::s_ch_num_samp = (int)pow(2, Pow_N);
int CMonitorFunction::s_num_samp = NUM_CHANNEL * s_ch_num_samp; // ����Ƶ�� * 16ͨ�� * ����ʱ�䳤�� = ��������
float *CMonitorFunction::s_pData = NULL; //malloc buffer for samp
float *CMonitorFunction::s_value_ch[NUM_CHANNEL];

BOOL boDbgLog = FALSE;
int iLogCount = 0;

CMonitorFunction::CMonitorFunction()
{
	m_hSocket = NULL;
	//׼������������Ϣ��������Ҫָ���������ĵ�ַ
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = inet_addr("192.168.3.100");
	m_addr.sin_port = htons(54321); 
}

CMonitorFunction::~CMonitorFunction()
{

}

void CMonitorFunction::AsyncSocketInit()
{
	if(WSAAsyncSelect(m_hSocket,m_hWnd, SOCK_MESSAGE,FD_READ|FD_CLOSE)>0)
	{
		AfxMessageBox("Error in select");
	}
}

BOOL CMonitorFunction::InitAndConnet(HWND hwnd)
{
	int ret = 0;
	DWORD sock_error = 0;

	m_hWnd=hwnd;

	if(m_hSocket != NULL)
	{
		return TRUE;
	}

	m_hSocket = socket(AF_INET, SOCK_STREAM,0);
	ASSERT(m_hSocket != NULL);
	AsyncSocketInit();

	while(SOCKET_ERROR == connect(m_hSocket, (LPSOCKADDR)&m_addr, sizeof(m_addr)))
	{
		sock_error = GetLastError();
		if(10056==sock_error) //10056: socket already connected!
		{
			break;
		}
		TRACE("Fail to connect to server, error: %d\n", sock_error);
		Sleep(2000);

		//maybe closed by server in other thread
		if(m_hSocket == NULL)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CMonitorFunction::SendDataToServer(char *msg, int msgLen, int code)
{
	char buf[512];
	int idx = 0;

	memset(buf, 0, sizeof(buf));
	//SOF
	buf[idx++] = 85;
	//CODE

	buf[idx++] = code;
	//STA
	//buf[idx++] = 0;
	//CRC
	//buf[idx++] = 0;
	//DLEN
	buf[idx++] = (char)msgLen;

	memcpy(buf+idx, msg, msgLen);
	if(send(m_hSocket, buf, idx+msgLen, 0) < 0)
	{
		TRACE("Fail to send message to server! error:%d\n", GetLastError());
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI CMonitorFunction::SoundHandleThread(LPVOID lpParameter)
{
	CRecorderDlg *pDlg = (CRecorderDlg *)lpParameter;
	int i = 0;
	char data[16];

	while(pDlg->boMonThreadLive)
	{
		TRACE(_T("Handle thread should check data ready to handle at first!\n"));
		CMonitorFunction::s_data_ready.Lock();
		TRACE("Handle thread lock data mutex!\n");
		CMonitorFunction::s_mutex.Lock(); //������, ��ָֹ���ڴ汻�ɼ��߳��ͷ�
		TRACE(_T("handle thread got data mutex!\n"));

		if(NULL == CMonitorFunction::s_pData ||
			NULL == CMonitorFunction::s_pSoundBuffer ||
			NULL == CMonitorFunction::s_value_ch)
		{
			TRACE(_T("NULL data buffer! %p-%p-%p\n"), s_pData, s_pSoundBuffer, s_value_ch);
			pDlg->boMonThreadLive = FALSE;
			CMonitorFunction::s_mutex.Unlock();
			CMonitorFunction::s_buffer_available.Unlock();
			break;
		}

		//�����ݴӲɼ�buffer����������buffer
		//�ɼ��̰߳����ݲɼ���s_pSoundBuffer, �����߳̿�����s_pData֮��,�Ϳ���֪ͨ�ɼ��̼߳���������
		memcpy(s_pData, s_pSoundBuffer, sizeof(float)*CMonitorFunction::s_num_samp);
		//֪ͨ�ɼ��̼߳�������
		TRACE("Notify collect thread that it can go on!\n");
		CMonitorFunction::s_buffer_available.Unlock();

		//д�ļ�
		if(pDlg->boRecording)
		{
			if(pDlg->m_txtdata)//�ı�
			{
				for(i = 0; i<CMonitorFunction::s_num_samp; i++)
				{
					_snprintf(data, sizeof(data), "%f, ", s_pData[i]);
					pDlg->m_FileTXT.Write(data, strlen(data));
				}
			}

			if(pDlg->m_bitdata)//����
			{
				pDlg->m_FileDATA.Write(s_pData, sizeof(float)*CMonitorFunction::s_num_samp);
			}
			TRACE("Write audio data into file finish!\r\n");
		}

		//�ͷ�������, ��������ɼ����鳤������仯, �ɼ��߳̿��Դ��·������鳤����
		CMonitorFunction::s_mutex.Unlock();
	}

	return 0;
}

DWORD WINAPI CMonitorFunction::SoundCollectThread(LPVOID lpParameter)
{
	int err = 0;
	int len = 0;
	int i = 0;
	CRecorderDlg *pDlg = (CRecorderDlg *)lpParameter;

	//���Ҫ�޸����û�������,�����󻥳���
	TRACE("Collect thread lock data mutex!\n");
	CMonitorFunction::s_mutex.Lock();

	//Malloc buffer
	CMonitorFunction::s_pData = new float[CMonitorFunction::s_num_samp]; //malloc buffer for samp
	CMonitorFunction::s_pSoundBuffer = new float[CMonitorFunction::s_num_samp];
	for(i=0; i<NUM_CHANNEL; i++)
	{
		CMonitorFunction::s_value_ch[i] = new float[CMonitorFunction::s_ch_num_samp];
	}
	CMonitorFunction::s_mutex.Unlock(); //�ͷ�������
	TRACE("Collect thread release data mutex!\n");
     if(change_flag ==1)   //���������������10000����λ�忨
	 {
        Id_Num = ReSetCardV6360(DEV_DST_IP);
			 
	    Sleep(3000); //��λ�ȴ�
			 
	 }
	
	ADStopReceive();
	TRACE("Thread sound collect begin...\n");
	AD_Config(DEV_DST_IP, 0, 0, CMonitorFunction::s_freq);

	while(pDlg->boMonThreadLive)
	{   
		TRACE("Collect thread should check buffer available before sound collection!\n");
		CMonitorFunction::s_buffer_available.Lock();

		//���� 
		TRACE("collect data\n");
		while(GetAdBufSize() < CMonitorFunction::s_num_samp)
		{
			if((recont_times>100) && (hAdThread == NULL))   //�������������100����λ�忨
			{
				Id_Num = ReSetCardV6360(DEV_DST_IP);
				Sleep(5000); //��λ�ȴ�
			 
				if(Id_Num!=0)
				{
					recont_times = 0; //��λ�ɹ���0
					TRACE("reset succeed!\n");
				}
				else
				{
					TRACE("reset failed!\n");
				}
			}		
			
			if(hAdThread == NULL&& stop_flag == 0)//���˹��رտ�ʼ����
			{
            	if(connect_flag == 0)
				{
					TRACE(_T("Adreceive break,begin connecting time tick:%d\r\n"), GetTickCount());
					recont_times++;
				}
				ADStopReceive();
				hAdThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AD_DataReceive, NULL, 0, NULL);
				continuea=1;
			}
     
			Sleep(10);//while�ȴ� 
		}

		len = ReadAdBufData(CMonitorFunction::s_pSoundBuffer, CMonitorFunction::s_num_samp);
		if(CMonitorFunction::s_num_samp != len)
		{
			TRACE(_T("Receive %d bytes, not expected %d\n"), len, CMonitorFunction::s_num_samp);
		}
	
		CMonitorFunction::s_data_ready.Unlock(); //֪ͨ���ݴ����߳�, �Ѿ������ɹ�

	}

	CMonitorFunction::s_mutex.Lock();
	delete CMonitorFunction::s_pData; //free buffer
	CMonitorFunction::s_pData = NULL;
	delete CMonitorFunction::s_pSoundBuffer;
	CMonitorFunction::s_pSoundBuffer = NULL;
	for(i=0; i<NUM_CHANNEL; i++)
	{
		delete CMonitorFunction::s_value_ch[i];
		CMonitorFunction::s_value_ch[i] = NULL;
	}
	CMonitorFunction::s_mutex.Unlock();

	return 0;
}


void  AD_DataReceive(void * pParam)
{
	int gain=ad_range;
	int ad_os=ad_osample;
	int ad_freq=ad_frequency;

	float adResult=0.0f;

	EnterCriticalSection(&critical_sec);
	while(AdDataBuf.empty() == false)
	{
		AdDataBuf.pop();
	}
	LeaveCriticalSection(&critical_sec);

	char *destIP=DEV_DST_IP;//"192.168.3.30";

	unsigned int cnt=0;
	SOCKET SockClient;
	WSADATA wsaData;
	WORD wVersionRequested;
	int err;
	int state=-1;					//��������ֵ���ɹ�����0
	timeval tv={1,50000};			//��ʱʱ������
	unsigned char outbuf[100];		//���ͻ���
	unsigned char recvBuf[1000];		//���ջ���
	//select ģ�ͣ������ó�ʱ
	fd_set fdRead;
	fd_set fdWrite;

	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		return ;
	}
	if ( LOBYTE( wsaData.wVersion ) != 1 ||
		HIBYTE( wsaData.wVersion ) != 1 ) 
	{
		WSACleanup( );
		return ;
	}
	SockClient=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr=inet_addr(destIP);
	addrSrv.sin_family=AF_INET;
	addrSrv.sin_port=htons(PORT);

	//���ó�ʱ50ms�ͻظ�ack����ʱ
	int iTimeOut = 3000;bool nodelay=true;
	setsockopt(SockClient,SOL_SOCKET,SO_RCVTIMEO,(char*)&iTimeOut,sizeof(iTimeOut));
	setsockopt(SockClient,SOL_SOCKET,SO_SNDTIMEO,(char*)&iTimeOut,sizeof(iTimeOut));
	setsockopt(SockClient, SOL_SOCKET, TCP_NODELAY  , (char*)&nodelay, sizeof(nodelay));

	//���÷�������ʽ���ӣ����conect �˳��ٶ�
	unsigned long ul = 1;//0 ����ģʽ��1������ģʽ
	int ret = ioctlsocket(SockClient, FIONBIO, (unsigned long*)&ul);
	if(ret==SOCKET_ERROR)return ;
	connect(SockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));	

	int nError;
	FD_ZERO(&fdWrite);
	FD_SET(SockClient,&fdWrite);
	ret = select(0,NULL,&fdWrite,NULL,&tv);
	if ( ret <= 0 )
	{
		::closesocket(SockClient);
		WSACleanup();
		hAdThread = NULL;
		TRACE("����ʧ��");
		return ;
	}
	TRACE("���ӳɹ�");


	FD_ZERO(&fdWrite);	
	FD_SET(SockClient,&fdWrite);
	nError = select(0,&fdRead,&fdWrite,NULL,&tv);

	memset(outbuf,0,20);
	outbuf[0]=0x55;outbuf[1]=20;outbuf[2]=0;outbuf[3]=0;outbuf[4]=0;
	outbuf[5]=3;outbuf[6]=ad_os;outbuf[7]=gain;outbuf[8]=ad_freq;outbuf[9]=ad_freq>>8;
	outbuf[10]=ad_freq>>16;outbuf[11]=ad_freq>>24;
	outbuf[12]=0;outbuf[13]=0;outbuf[14]=0;outbuf[15]=0;
	outbuf[16]=0;outbuf[17]=0;outbuf[18]=0;outbuf[19]=0;

	//SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);

	if(FD_ISSET(SockClient,&fdWrite))
	{
		ret=send(SockClient,(char *)outbuf,20,0);//���Ϳ�ʼ����
		if(ret<20)
		{
			state=-1;
			goto END;
		}
	}
	tv.tv_sec=0;tv.tv_usec=50000;
	FD_ZERO(&fdRead);
	FD_SET(SockClient,&fdRead);
	nError = select(0,&fdRead,NULL,NULL,&tv);

	signed short int ResultBuf[3000];
	if(FD_ISSET(SockClient,&fdRead))
	{
		errTimeout=0;
		err=recv(SockClient,(char *)recvBuf,6,0);
		if((err==6)&&(recvBuf[0]==0xaa)&&(recvBuf[1]==0xff)&&(recvBuf[5]==3))
		{

		}else
		{
			AD_scan_stop=1;
			//֡ͷ����ʧ��
		}
	}
	memset(outbuf,0,16);
	outbuf[0]=0x55;outbuf[1]=10;outbuf[2]=0;outbuf[3]=0;outbuf[4]=0;
	outbuf[5]=9;outbuf[6]=0x01;outbuf[7]=0;outbuf[8]=0;outbuf[9]=0;
	while (1)
	{
		if (AD_scan_stop)
		{
			break;
		}
		
		FD_ZERO(&fdRead);
		FD_SET(SockClient,&fdRead);
		nError = select(0,&fdRead,NULL,NULL,&tv);

		if(FD_ISSET(SockClient,&fdRead))
		{
			errTimeout=0;
			err=recv(SockClient,(char *)&ResultBuf[0],2920,0);

			EnterCriticalSection(&critical_sec);
			for(int ii=0;ii<(err/2);ii++)
			{
				adResult=ResultBuf[ii];		 
				adResult=adResult*5.0f/32768.0f;	

				if(gain==0)//����������������
				{
					AdDataBuf.push(adResult*2.0f);
				}
				else
				{
					AdDataBuf.push(adResult);
				}
			}
			LeaveCriticalSection(&critical_sec);
		}
		else
		{
			errTimeout++;
			if(nError != 0)
			{
				cnt++;
			}
			else
			{
				cnt = 0;
			}
			
			if(errTimeout>40 && 0==cnt)
			{
				TRACE(_T("Receive data from AD-Card timeout, 2s!\n"));
				//hAdThread = NULL;
				break;
			}
			
			if(cnt > 20)
			{
				TRACE(_T("Thread AD_DataReceive fail and exit! %d-%d\n"), nError,  WSAGetLastError());
				break;
			}
		}
	}
	//SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);

	//���ۺ���ԭ��Ҫ����ֹͣ����
	tv.tv_sec=0;tv.tv_usec=50000;
	FD_ZERO(&fdWrite);	
	FD_SET(SockClient,&fdWrite);
	nError = select(0,&fdRead,&fdWrite,NULL,&tv);
	memset(outbuf,0,20);
	outbuf[0]=0x55;outbuf[1]=10;outbuf[2]=0;outbuf[3]=0;outbuf[4]=0;
	outbuf[5]=4;outbuf[6]=0;outbuf[7]=0;outbuf[8]=0;outbuf[9]=0;
	outbuf[10]=0;
	if(FD_ISSET(SockClient,&fdWrite))
	{
		ret=send(SockClient,(char *)outbuf,10,0);//���Ϳ�ʼ����
		if(ret<20)
		{
			state=-1;
			goto END;
		}
	}

END:
	closesocket(SockClient);
	WSACleanup();
	TRACE("Disconnect success \n");
	hAdThread = NULL;
}


int AD_Config(char *destIP,int ad_os,int gain,int ad_freq)
{
	if(ad_freq>40000)
   {
     return -2;
   }

	if(gain>6)
		return -1;
	InitializeCriticalSection(&critical_sec); 
	
	//  SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
	AD_scan_stop=0;
	ad_range=gain;//��������
	ad_osample=ad_os;
	ad_frequency=ad_freq;
	AD_scan_stop=0;
//	ADStopReceive;
	hAdThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AD_DataReceive, NULL, 0, NULL);//ThreadStart(&hAdThread, AD_DataReceive, NULL);
	continuea=1;
	Sleep(5);
	return 0;
}

int GetAdBufSize(void)
{
	if(continuea==0)
		return 0;
	if(hAdThread ==NULL)
		return 0;
	std::queue<float>::size_type bufSize;
	EnterCriticalSection(&critical_sec);
	bufSize = AdDataBuf.size();
	LeaveCriticalSection(&critical_sec);
	// if(ADerro=1)return -1;
	return bufSize;
}

int ReadAdBufData(float* databuf, int num)
{
	//SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
	//if (opened == 0)
	//	return -1;
	int jj = 0;

	EnterCriticalSection(&critical_sec);
	for (jj = 0; jj<num; jj++)
	{
		if (AdDataBuf.empty() == false)
		{	
			*databuf++ = AdDataBuf.front();
			AdDataBuf.pop();
		}
		else
		{
			break;
		}
	}
	LeaveCriticalSection(&critical_sec);

	return jj;
}

int ADStopReceive(void)
{
	continuea=0;
	if(hAdThread == NULL)
	{
	  AD_scan_stop = 0;		
      return -1;
	}
		
	AD_scan_stop = 1;	
	Sleep(100);
	WaitForMultipleObjects(1, &hAdThread, TRUE, INFINITE);
	Sleep(300);
	hAdThread = NULL;
	AD_scan_stop = 0;
	return 0;
}

unsigned int ReSetCardV6360(char *destIP)
{
	unsigned int idnum=0;
	SOCKET SockClient;
	WSADATA wsaData;
	WORD wVersionRequested;
	int err;
	int state=-1;					//��������ֵ���ɹ�����0
	timeval tv={0,50000};			//��ʱʱ������
	unsigned char outbuf[100];		//���ͻ���
	//unsigned char recvBuf[100];		//���ջ���
	//select ģ�ͣ������ó�ʱ
	fd_set fdRead;
	fd_set fdWrite;

	wVersionRequested = MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		return -1;
	}
	if ( LOBYTE( wsaData.wVersion ) != 1 ||
		HIBYTE( wsaData.wVersion ) != 1 ) 
	{
		WSACleanup( );
		return -1;
	}
	SockClient=socket(AF_INET,SOCK_STREAM,0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr=inet_addr(destIP);
	addrSrv.sin_family=AF_INET;
	addrSrv.sin_port=htons(PORT);

	//���ó�ʱ50ms�ͻظ�ack����ʱ
	int iTimeOut = 3000;bool nodelay=true;
	setsockopt(SockClient,SOL_SOCKET,SO_RCVTIMEO,(char*)&iTimeOut,sizeof(iTimeOut));
	setsockopt(SockClient,SOL_SOCKET,SO_SNDTIMEO,(char*)&iTimeOut,sizeof(iTimeOut));
	setsockopt(SockClient, SOL_SOCKET, TCP_NODELAY  , (char*)&nodelay, sizeof(nodelay));

	//���÷�������ʽ���ӣ����conect �˳��ٶ�
	unsigned long ul = 1;//0 ����ģʽ��1������ģʽ
	int ret = ioctlsocket(SockClient, FIONBIO, (unsigned long*)&ul);
	if(ret==SOCKET_ERROR)return 0;
	connect(SockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));	

	int nError;
	FD_ZERO(&fdWrite);
	FD_SET(SockClient,&fdWrite);
	ret = select(0,NULL,&fdWrite,NULL,&tv);
	if ( ret <= 0 )
	{
		::closesocket(SockClient);
		WSACleanup();
		TRACE("����ʧ��");
		return -1;
	}
	TRACE("���ӳɹ�");

	FD_ZERO(&fdWrite);	
	FD_SET(SockClient,&fdWrite);
	nError = select(0,&fdRead,&fdWrite,NULL,&tv);
	memset(outbuf,0,16);
	outbuf[0]=0x55;outbuf[1]=20;outbuf[2]=0;outbuf[3]=0;outbuf[4]=0;
	outbuf[5]=12;outbuf[6]=0x00;outbuf[7]=0;outbuf[8]=0;outbuf[9]=0;
	 
	if(FD_ISSET(SockClient,&fdWrite))
	{
		ret=send(SockClient,(char *)outbuf,20,0);//���Ͳɼ�����ָ��
		if(ret<20)
		{
			state=-1;
			goto END;
		}
	}

	 
END:
	closesocket(SockClient);
	WSACleanup();
	TRACE("Disconnect success \n");
	
	return idnum;
}


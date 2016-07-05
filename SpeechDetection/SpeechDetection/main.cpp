#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <conio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#pragma comment (lib, "winmm.lib")

#define SRATE 44100
#define THRESHOLD 6000
#define RECVSIZE 256
#define BITS 16

unsigned int __stdcall detect(void *a);
unsigned int __stdcall recv_thread(void *a);
int compare_str(char str1[],char str2[]);

void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void CALLBACK waveInProc2(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void wav_write(const char *filename, short *buf, int size);

 static WAVEFORMATEX wfe;
 static short *bufferIN;
 static short *bufferIN2;
 static HWAVEIN hWaveIn;
 static WAVEHDR whdrIn;
 static WAVEHDR whdrIn2;
 static HWAVEIN hWaveIn2;
 SOCKET sock0; SOCKET sock;
 struct sockaddr_in addr;
 struct sockaddr_in client;

 char detection_flag=1;
 char detect_thread_flag=0;
 char recv_thread_flag=1;


//main
void main(void){   
    HANDLE hTh;
    hTh = (HANDLE)_beginthreadex(NULL, 0, recv_thread, NULL, 0, NULL);
    WaitForSingleObject(hTh, INFINITE);
 return ;
}


/*////////////////////////*/
/*recv_thread(thread)     */
/*////////////////////////*/
unsigned int __stdcall recv_thread(void *a)
{	
 WSADATA wsaData;
 int len;
 char recv_buf[RECVSIZE];
 int nBytesRecv;
 HANDLE Th;

 //message
 char msg[] ="connected\n";
 char msg_notstarted[] ="Error;detection is not started";
 char msg_start[]="Ack;Start";
 char msg_stop[]="Ack;Stop";
 char msg_running[]="Ack;Status detection running";
 char msg_notrunning[]="Ack;Status detection is not running";
 char msg_invalid[]="Invalid Command";


// winsock2�̏�����
 WSAStartup(MAKEWORD(2,0), &wsaData);

 // �\�P�b�g�̍쐬
 sock0 = socket(AF_INET, SOCK_STREAM, 0);

 // �\�P�b�g�̐ݒ�
 addr.sin_family = AF_INET;
 addr.sin_port = htons(12345);
 addr.sin_addr.S_un.S_addr = INADDR_ANY;
 bind(sock0, (struct sockaddr *)&addr, sizeof(addr));

 // TCP�N���C�A���g����̐ڑ��v����҂Ă��Ԃɂ���
 listen(sock0, 5);

 // TCP�N���C�A���g����̐ڑ��v�����󂯕t����
 len = sizeof(client);
 printf("�ڑ��҂�");
 sock = accept(sock0, (struct sockaddr *)&client, &len);
 system("cls");
 send(sock,msg,strlen(msg), 0);
 printf("�ڑ�����\n");
 
 memset(recv_buf,0,sizeof(recv_buf));

  while(recv_thread_flag){

	nBytesRecv = recv(sock,recv_buf,sizeof(recv_buf),0);
	recv_buf[nBytesRecv] = '\0'; 
	printf("��M:%s",recv_buf);

		if(compare_str(recv_buf, "end") == 1){
			printf("�ؒf\n");
			break;
		}else if(compare_str(recv_buf,"start")==1){
			if(detect_thread_flag==0){
				detect_thread_flag=1;
				Th = (HANDLE)_beginthreadex(NULL, 0, detect, NULL, 0, NULL);
				send(sock,msg_start,strlen(msg_start), 0);
				printf("���o�J�n\n");
			}else{
				send(sock,msg_notstarted,strlen(msg_running), 0);
			}
		}else if(compare_str(recv_buf,"stop")==1){
			if(detect_thread_flag==1){
				detect_thread_flag=0;
				send(sock,msg_stop,strlen(msg_stop), 0);
				printf("���o��~\n");
			}else{
				send(sock,msg_notstarted,strlen(msg_notstarted), 0);
			}
		}else if(compare_str(recv_buf,"status")==1){
			if(detect_thread_flag==0){
				send(sock,msg_notrunning,strlen(msg_notrunning), 0);
				printf("%s\n",msg_notrunning);
			}else{
				send(sock,msg_running,strlen(msg_running), 0);
		printf("%s\n",msg_running);
			}
		}else if(recv_buf[0]==0){
				//printf("�����ȃR�}���h��M\n");
				///	break;
		}

	memset(recv_buf,0,sizeof(recv_buf));
	Sleep(100);
 }
 // TCP�Z�b�V�����̏I��
 closesocket(sock);

 // winsock2�̏I������
 WSACleanup();
 recv_thread_flag=0;
 return 0;
}
/*////////////////////////*/
/*detect(thread)          */
/*////////////////////////*/
unsigned int __stdcall detect(void *a){	
	char flag1 =0;
	char flag2 =0;
	long counter =0;
	int size;
	int size2;
	short *buf;
	char through_flag=1;
	DWORD msg;
	char szFullPath[MAX_PATH] = {'\0'};
	char *szFilePart;

//�e��ݒ� in out�ŋ���

	wfe.wFormatTag = WAVE_FORMAT_PCM;
	wfe.nChannels = 1;//���m���� 1 or 2
	wfe.nSamplesPerSec = SRATE;//�T���v�����O���g��
	wfe.wBitsPerSample = BITS;// 8 or 16
	wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;//byte/sample
	wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;//SRATE;//1�b�Ԃ̃o�C�g�� 

	bufferIN = (short*)calloc(1,wfe.nAvgBytesPerSec);//1�b���̃o�b�t�@��p��
	bufferIN2 = (short*)calloc(5,wfe.nAvgBytesPerSec);//1�b���̃o�b�t�@��p��
//////////////////////////
//���͏���

	whdrIn.lpData = (LPSTR)bufferIN;
	whdrIn.dwBufferLength = wfe.nAvgBytesPerSec;

	waveInOpen(&hWaveIn , WAVE_MAPPER , &wfe , (DWORD)waveInProc , 0 , CALLBACK_FUNCTION);
	waveInPrepareHeader(hWaveIn , &whdrIn , sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn , &whdrIn , sizeof(WAVEHDR));
	
	waveInStart(hWaveIn);//�^���J�n
	
	whdrIn2.lpData = (LPSTR)bufferIN2;
	whdrIn2.dwBufferLength = wfe.nAvgBytesPerSec*5;
	
	waveInOpen(&hWaveIn2 , WAVE_MAPPER , &wfe , (DWORD)waveInProc2 , 0 , CALLBACK_FUNCTION);
	waveInPrepareHeader(hWaveIn2 , &whdrIn2 , sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn2 , &whdrIn2 , sizeof(WAVEHDR));

/////////	
	through_flag=1;
	while(detection_flag){
		while(detect_thread_flag){
		
			for(int i=0;i<SRATE;i++)
			{
			//	printf("%d\n",(int)bufferIN[i]);
				if(abs((int)bufferIN[i])>THRESHOLD){
					flag1=1;
					break;
				}
			}
			if(flag1){
				if(through_flag){
					printf("speak:\n");
					waveInStart(hWaveIn2);//�^���J�n
					through_flag=0;
					flag2=1;
				}
			}else{
				if(flag2){
					printf("end\n");
					flag2=0;
					counter=0;
					size2 = (SRATE*10); // 5�b��
					wav_write("rec.wav", bufferIN2,size2);
					printf("created wave file\n");
					_fullpath(szFullPath, "..\\rec.wav", sizeof(szFullPath)/sizeof(szFullPath[0]));
					send(sock,szFullPath,sizeof(szFullPath)/sizeof(szFullPath[0]), 0);
					waveInAddBuffer(hWaveIn , &whdrIn , sizeof(WAVEHDR));
					waveInAddBuffer(hWaveIn2 , &whdrIn2 , sizeof(WAVEHDR));
					through_flag=1;
					detect_thread_flag=0;
				}
			}
			flag1=0;
			Sleep(100);
		}
			Sleep(100);
	}
//���͉��
	waveInUnprepareHeader(hWaveIn2 , &whdrIn2 , sizeof(WAVEHDR));
	waveInStop(hWaveIn);
	waveInClose(hWaveIn2);
	waveInUnprepareHeader(hWaveIn , &whdrIn , sizeof(WAVEHDR));
	waveInStop(hWaveIn);
	waveInClose(hWaveIn);
//	free(bufferIN);
//	free(bufferIN2);
	detection_flag=0;
	detect_thread_flag=0;
	_endthreadex(0);
	return 0;
}

/*////////////////////////////////////*/
/* waveInProc
/* waveInOpen�̃R�[���o�b�N
/* �o�b�t�@����t�ɂȂ�����Ă΂��
/*///////////////////////////////////*/
void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	/* �C�x���g���� */
	switch(uMsg){
    	case WIM_DATA:
			waveInAddBuffer(hWaveIn , &whdrIn , sizeof(WAVEHDR));
			break;
	}
}
/*////////////////////////////////////*/
/* waveInProc2
/* waveInOpen�̃R�[���o�b�N
/* �o�b�t�@����t�ɂȂ�����Ă΂��
/*///////////////////////////////////*/
void CALLBACK waveInProc2(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	/* �C�x���g���� */
	switch(uMsg){
    	case WIM_DATA:
			waveInAddBuffer(hWaveIn2 , &whdrIn2 , sizeof(WAVEHDR));
			break;
	}
}


/*////////////////////////*/
/* wav_write
/* buf��wav�Ƃ��ď����o��
/*////////////////////////*/
void wav_write(const char *filename, short *buf, int size)
{
 int filesize = 44 + size;//+size2;
 char *work = (char *) malloc(filesize);
 FILE *fp = fopen(filename, "wb");

 if (fp == NULL) return;

 // RIFF�w�b�_ 
 memcpy(work, "RIFF", 4);
 work[4] = (filesize - 8) >> 0  & 0xff;
 work[5] = (filesize - 8) >> 8  & 0xff;
 work[6] = (filesize - 8) >> 16 & 0xff;
 work[7] = (filesize - 8) >> 24 & 0xff;
 // WAVE�w�b�_ 
 memcpy(work+8, "WAVE", 4);
 // fmt�`�����N 
 memcpy(work+12, "fmt ", 4);
 work[16] = 16;
 work[17] = work[18] = work[19] = 0;
 work[20] = 1;
 work[21] = 0;
 work[22] = 1;
 work[23] = 0;
 work[24] = SRATE >> 0  & 0xff;
 work[25] = SRATE >> 8  & 0xff;
 work[26] = SRATE >> 16 & 0xff;
 work[27] = SRATE >> 24 & 0xff;
 work[28] = (SRATE * (BITS / 8)) >> 0  & 0xff;
 work[29] = (SRATE * (BITS / 8)) >> 8  & 0xff;
 work[30] = (SRATE * (BITS / 8)) >> 16 & 0xff;
 work[31] = (SRATE * (BITS / 8)) >> 24 & 0xff;
 work[32] = ((BITS / 8)) >> 0 & 0xff;
 work[33] = ((BITS / 8)) >> 8 & 0xff;
 work[34] = BITS >> 0 & 0xff;
 work[35] = BITS >> 8 & 0xff;
 // data�`�����N 
 memcpy(work+36, "data", 4);
 work[40] = size >> 0  & 0xff;
 work[41] = size >> 8  & 0xff;
 work[42] = size >> 16 & 0xff;
 work[43] = size >> 24 & 0xff;
 memcpy(work + 44, buf, size);
 // �����o�� 
 fwrite(work, filesize, 1, fp);
 fclose(fp);
 free(work);
}


/*////////////////////////*/
/*compare_str
/*��������r
/*////////////////////////*/
int compare_str(char str1[],char str2[]){
	char counter=0;
	for(char i=0;i<strlen(str2);i++){
	//	printf("%c,%c\n",str1[i],str2[i]);
		if(str1[i] == str2[i]){
		counter++;
		}
	}
	if(counter==strlen(str2)){
		counter =1;
	}else{
		counter=0;
	}
	return counter;
}
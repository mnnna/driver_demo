 #include<Windows.h>
#include <iostream>
#include <winioctl.h>

#define DEVICE_LINK_NAME "\\\\.\\DemoInject" // R3 format  

void main(){
	BOOL bRet; 
	HANDLE hDevice = NULL; 

	hDevice = CreateFileA(DEVICE_LINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, NULL, NULL);

	if (hDevice == (HANDLE)-1) {  //�� Windows ����У�ͨ��ʹ�� -1 ����ʾһ���������Ч�����INVALID_HANDLE_VALUE��
		printf("Create file faild ! \n ");
	}

	char outBuff[MAXBYTE] = { 0 };
	char inBuff[MAXBYTE] = "Hello world from R3";


	bRet = WriteFile(hDevice, inBuff, sizeof(inBuff), NULL, NULL);
	if (!bRet) {
		printf("write file failed\n");
	}
	else {
		printf("write file outBuff: %s \n", inBuff);
	}


	system("pause");

	CloseHandle(hDevice);

	system("pause");

}
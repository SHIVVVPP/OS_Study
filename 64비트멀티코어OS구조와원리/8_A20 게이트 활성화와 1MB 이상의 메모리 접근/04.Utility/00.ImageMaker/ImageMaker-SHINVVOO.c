#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTESOFSECTOR 512

// 함수 선언
int AdjustInSectorSize( int iFd, int iSourceSize );
void WriteKernelInformation( int iTargetFd, int iKernelSectorCount );
int CopyFile( int iSourceFd, int iTargetFd );

//Main
int main(int argc, char* argv[])
{
	int iSourceFd;
	int iTargetFd;
	int iBootLoaderSize;
	int iKernel32SectorCount;
	int iSourceSize;
	
	// 커맨드 라인 옵션 검사
	if(argc <3)
	{
		fprintf( stderr, "[ERROR] ImageMaker.exe BootLoader.bin Kernel32.bin \n");
		exit(-1);
	}
	
	// Disk.img 파일을 생성
	if( (iTargetFd = open( "Disk.img", O_RDWR | 0_CREAT | O_TRUNC |
		O_BINARY, S_IREAD | S_IWRITE ) ) == -1 )
		{
			fprintf( stderr, "[ERROR] Disk.img open fail\n");
			exit(-1);
		}
		
	//===================================================================
	//	부트 로더 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
	//===================================================================
	printf("[INFO] Copy boot loader to image file\n");
	if((iSourceFd = open( argv[1], O_RDONLY | O_BINARY)) == -1 )
	{
		fprintf(stderr, "[ERROR] %s open fail\n", argv[1]);
		exit(-1);
	}
	
	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);
	
	// 파일 크기를 섹터 크기인 512바이트로 맞추기 위해 나머지 부분을 0x00으로 채운다.
	iBootLoaderSize = AdjustInSectorSize( iTargetFd, iSourceSize);
	printf("[INFO] %s size = [%d] and sector count = [%d]\n",
			argv[1], iSourceSize, iBootLoaderSize);
			
			
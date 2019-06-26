#pragma once
#include "stdint.h"
#include "windef.h"

/*   _emit is DB(Define Byte - 8bit) equivalent but not DD(Define double word - 4Bytes) equivalent exist
so we define it ourself */
// _emit 은 DB(Define Byte - 8bit)는 가능하지만 DD(Define double word - 4Byte(32bit)는 가능하지 않으므로 새로 정의 
// _emit [Byte] 인자로 넘겨진 바이트들을 직접 기계어 코드로 삽입
#define dd(x)                            \
        __asm _emit     (x)       & 0xff \
        __asm _emit     (x) >> 8  & 0xff \
        __asm _emit     (x) >> 16 & 0xff \
        __asm _emit     (x) >> 24 & 0xff

#define KERNEL_STACK			0x0040000

// ALIGN 은 GRUB으로 PE커널을 로드할 수 있는 가장 중요한 것 중 하나이다.
// 왜냐하면 PE header가 파일 시작 부분에 있으므로 코드섹션이 이동되기 때문이다.
// 코드섹션을 이동시키는데 사용되는 값 (0x400)은 linker 정렬 옵션인 /ALIGN:value이다.
// 헤더 사이즈의 합계가 512보다 크므로, ALIGN 값은 그보다 더 커야 한다.
/*  This is the one of most important thing to be able to load a PE kernel
with GRUB. Because PE header are in the begining of the file, code section
will be shifted. The value used to shift the code section is the linker
align option /ALIGN:value. Note the header size sum is larger than 512,
so ALIGN value must be greater */
#define   ALIGN               0x400

/*
16진수 용량 빠르게 계산하기
1KB : 2의 10제곱
1MB : 2의 20제곱
1GB : 2의 30제곱
1TB : 2의 40제곱...

16진수 digit 하나는 2의 4제곱

2의 20제곱의 배수는 다음과 같아진다.
1MB : 0x100000 ( 0x0이 5개 즉, 2의 4제곱 X 5)
1TB : 0x100000 00000 (0x0이 10개)

6진수는 자리가 증가할 때마다 16이 곱해진다
그렇다면 자리가 증가할 때마다 일정 패턴이 있을까?
0x1          => 1
0x10        => 16
0x100       => 256
0x1000      => 4K
0x10000     => 64KB
0x100000  => 1MB(First Pattern으로 간단하게 알 수 있음)
자리수가 늘어날 때마다
1=>16=>256=>4(Kilo만큼 단위 증가)=>64=>1(Kilo만큼 단위 증가)
1MB 단위에서 다시 1이 나왔으므로 MB만 빼면 1=>16=>256...패턴이 되겠군.

- 유용성 테스트
0x10000000000(0x0이 10개) : First Patten으로 바로 1GB

0x4000
Second Pattern 사용하여 먼저 사용하여 먼저 0x1000을 구해보면 1=>16=>256=>4K이고
곱하기 4를 해주면 16K
*/

// GRUB이 0~1Mb(0x100000)를 사용
/*   Must be >= 1Mb(0x100000) for GRUB
Base adress from advanced linker option
*/
#define KERNEL_LOAD_ADDRESS            0x100000


#define   HEADER_ADRESS         KERNEL_LOAD_ADDRESS+ALIGN

#define MULTIBOOT_HEADER_MAGIC         0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002
#define MULTIBOOT_HEADER_FLAGS         0x00010003 
#define STACK_SIZE              0x4000    
#define CHECKSUM            -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

#pragma pack(push,1)// 구조체 정렬 크기 조절 (1바이트로)
struct MULTIBOOT_HEADER {
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
	uint32_t header_addr;
	uint32_t load_addr;
	uint32_t load_end_addr;
	uint32_t bss_end_addr;
	uint32_t entry_addr;
};

/*
ELF = Excutable and Linkable Format
실행파일, 목적파일, 공유 라이브러리 그리고 코어 덤프(작업중인 메모리상태를 기록)를 위한 표준 파일 형식
*/
struct ELFHeaderTable
{
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
};
typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;

struct AOUTSymbolTable
{
	unsigned int tabsize;
	unsigned int strsize;
	unsigned int addr;
	unsigned int reserved;
};

//Bochs 2.4 doesn't provide a ROM configuration table. Virtual PC does.
struct ROMConfigurationTable
{
	unsigned short Length;
	unsigned char Model;
	unsigned char Submodel;
	unsigned char BiosRevision;
	bool DualBus : 1;
	bool MicroChannelBus : 1;
	bool EBDAExists : 1;
	bool WaitForExternalEventSupported : 1;
	bool Reserved0 : 1;
	bool HasRTC : 1;
	bool MultipleInterruptControllers : 1;
	bool BiosUsesDMA3 : 1;
	bool Reserved1 : 1;
	bool DataStreamingSupported : 1;
	bool NonStandardKeyboard : 1;
	bool BiosControlCpu : 1;
	bool BiosGetMemoryMap : 1;
	bool BiosGetPosData : 1;
	bool BiosKeyboard : 1;
	bool DMA32Supported : 1;
	bool ImlSCSISupported : 1;
	bool ImlLoad : 1;
	bool InformationPanelInstalled : 1;
	bool SCSISupported : 1;
	bool RomToRamSupported : 1;
	bool Reserved2 : 3;
	bool ExtendedPOST : 1;
	bool MemorySplit16MB : 1;
	unsigned char Reserved3 : 1;
	unsigned char AdvancedBIOSPresence : 3;
	bool EEPROMPresent : 1;
	bool Reserved4 : 1;
	bool FlashEPROMPresent : 1;
	bool EnhancedMouseMode : 1;
	unsigned char Reserved5 : 6;
};

struct Module
{
	void *ModuleStart;
	void *ModuleEnd;
	char *Name;
	unsigned int Reserved;
};

struct multiboot_mmap_entry
{
	uint32_t size;
	uint64_t addr;
	uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
	uint32_t type;
};
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

/* Drive Info structure.  */
struct drive_info
{
	/* The size of this structure.  */
	unsigned long size;

	/* The BIOS drive number.  */
	unsigned char drive_number;

	/* The access mode (see below).  */
	unsigned char drive_mode;

	/* The BIOS geometry.  */
	unsigned short drive_cylinders;
	unsigned char drive_heads;
	unsigned char drive_sectors;

	/* The array of I/O ports used for the drive.  */
	unsigned short drive_ports;
};

struct APMTable
{
	unsigned short Version;
	unsigned short CS;
	unsigned int Offset;
	unsigned short CS16Bit;	//This is the 16-bit protected mode code segment
	unsigned short DS;
	unsigned short Flags;
	unsigned short CSLength;
	unsigned short CS16BitLength;
	unsigned short DSLength;
};

struct VbeInfoBlock
{
	char Signature[4];
	unsigned short Version;
	short OEMString[2];
	unsigned char Capabilities[4];
	short VideoModes[2];
	short TotalMemory;
};

/*struct VbeModeInfo
{
	unsigned short Attributes;
	unsigned char WinA;
	unsigned char WinB;
	unsigned short Granularity;
	unsigned short WinSize;
	unsigned short SegmentA;
	unsigned short SegmentB;
	unsigned short WindowFunctionPointer[2];
	unsigned short Pitch;
	unsigned short XResolution;
	unsigned short YResolution;
	unsigned char CharacterWidth;
	unsigned char CharacterHeight;
	unsigned char Planes;
	unsigned char BitsPerPixel;
	unsigned char Banks;
	unsigned char MemoryModel;
	unsigned char BankSize;
	unsigned char ImagePages;
	unsigned char Reserved;

	unsigned char RedMask;
	unsigned char RedPosition;

	unsigned char GreenMask;
	unsigned char GreenPosition;

	unsigned char BlueMask;
	unsigned char BluePosition;

	unsigned char ReservedMask;
	unsigned char ReservedPosition;

	unsigned char DirectColorAttributes;

	unsigned int FrameBuffer;
};*/

struct VbeModeInfo
{
	UINT16 ModeAttributes;
	char WinAAttributes;
	char WinBAttributes;
	UINT16 WinGranularity;
	UINT16 WinSize;
	UINT16 WinASegment;
	UINT16 WinBSegment;
	UINT32 WinFuncPtr;
	short BytesPerScanLine;
	short XRes;
	short YRes;
	char XCharSize;
	char YCharSize;
	char NumberOfPlanes;
	char BitsPerPixel;
	char NumberOfBanks;
	char MemoryModel;
	char BankSize;
	char NumberOfImagePages;
	char res1;
	char RedMaskSize;
	char RedFieldPosition;
	char GreenMaskSize;
	char GreenFieldPosition;
	char BlueMaskSize;
	char BlueFieldPosition;
	char RsvedMaskSize;
	char RsvedFieldPosition;
	char DirectColorModeInfo; //MISSED IN THIS TUTORIAL!! SEE ABOVE
							  //VBE 2.0
	UINT32 PhysBasePtr;
	UINT32 OffScreenMemOffset;
	short OffScreenMemSize;
	//VBE 2.1
	short LinbytesPerScanLine;
	char BankNumberOfImagePages;
	char LinNumberOfImagePages;
	char LinRedMaskSize;
	char LinRedFieldPosition;
	char LingreenMaskSize;
	char LinGreenFieldPosition;
	char LinBlueMaskSize;
	char LinBlueFieldPosition;
	char LinRsvdMaskSize;
	char LinRsvdFieldPosition;
	char res2[194];
};


/*
GRUB은 보호 모드로 전환한 후 커널을 호출 하는데,
커널 입장에서는 하드웨어 관련 정보를 얻기가 어려우므로
GRUB이 커널에 넘겨주는 multiboot_info 구조체를 활용해서
메모리 사이즈라던지 어떤 디바이스에서 부팅이 되었는지에 대한 정보를 알아낼 수 있다.
OS는 GRUB가 넘겨준 이 구조체를 통해 시스템 환경을 초기화 한다.
*/
struct multiboot_info
{
	uint32_t flags;
	// 플래그 : 플래그 값을 확인해서 VESA 모드가 가능한지의 여부를 파악할 수 있다.
	// 비디오 전자공학 표준위원회(Video Electronics Standards Association, VESA)
	// 바이오스로부터 얻은 이용 가능한 메모리 영역 정보
	uint32_t mem_lower;
	uint32_t mem_upper;

	uint32_t boot_device; // 부팅 디바이스의 번호
	char* cmdline; // 커널에 넘기는 command line

	//부팅 모듈 리스트
	uint32_t mods_count;
	Module* Modules;
	// 리눅스 파일과 관계된 정보
	union
	{
		AOUTSymbolTable AOUTTable;
		ELFHeaderTable ELFTable;
	} SymbolTables;

	// 메모리 매핑 정보를 알려준다.
	// 이 정보를 통해 메모리 특정 블록을 사용할 수 있는지 파악 가능하다.
	uint32_t mmap_length;
	uint32_t mmap_addr;

	//해당 PC에 존재하는 드라이브에 대한 정보
	uint32_t drives_length;
	drive_info* drives_addr;

	// ROM configuration table
	ROMConfigurationTable* ConfigTable;

	// 부트로더 이름
	char* boot_loader_name;

	// APM table
	APMTable* APMTable;

	// 비디오
	VbeInfoBlock* vbe_control_info;
	VbeModeInfo* vbe_mode_info;
	uint16_t vbe_mode;
	uint16_t vbe_interface_seg;
	uint16_t vbe_interface_off;
	uint16_t vbe_interface_len;
};
typedef struct multiboot_info multiboot_info_t;



#pragma pack(pop) // 정렬을 원래 상태로 되돌림

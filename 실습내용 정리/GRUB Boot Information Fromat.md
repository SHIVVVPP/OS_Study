# GRUB Boot Information 정리

## MultiBoot.h

```c++
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
```

### 3.3 Boot information format

Upon entry to the operating system, the `EBX` register contains the physical address of a Multiboot information data structure, through which the boot loader communicates vital information to the operating system. The operating system can use or ignore any parts of the structure as it chooses; all information passed by the boot loader is advisory only.    

> 운영 체제에 진입하면 'EBX' 레지스터에는 부트 로더가 운영 체제에 중요한 정보를 전달하는 멀티부팅 정보 데이터 구조의 물리적 주소가 포함됩니다. 운영 체제는 선택한 대로 구조의 모든 부분을 사용하거나 무시할 수 있습니다. 부트 로더에서 전달되는 모든 정보는 권장 사항일 뿐입니다. 

The Multiboot information structure and its related substructures may be placed anywhere in memory by the boot loader (with the exception of the memory reserved for the kernel and boot modules, of course). It is the operating system's responsibility to avoid overwriting this memory until it is done using it.    

> 멀티부팅 정보 구조 및 관련 하부 구조는 부트 로더에 의해 메모리의 어느 곳에나 배치될 수 있습니다(물론 커널 및 부트 모듈용으로 예약된 메모리를 제외함). 이 메모리를 사용할 때까지 덮어쓰지 않는 것은 운영 체제의 책임입니다.

The format of the Multiboot information structure (as defined so far) follows: 

> 멀티부팅 정보 구조의 형식(지금까지 정의된 형식)은 다음과 같다.

```
             +-------------------+
     0       | flags             |    (required)
             +-------------------+
     4       | mem_lower         |    (present if flags[0] is set)
     8       | mem_upper         |    (present if flags[0] is set)
             +-------------------+
     12      | boot_device       |    (present if flags[1] is set)
             +-------------------+
     16      | cmdline           |    (present if flags[2] is set)
             +-------------------+
     20      | mods_count        |    (present if flags[3] is set)
     24      | mods_addr         |    (present if flags[3] is set)
             +-------------------+
     28 - 40 | syms              |    (present if flags[4] or
             |                   |                flags[5] is set)
             +-------------------+
     44      | mmap_length       |    (present if flags[6] is set)
     48      | mmap_addr         |    (present if flags[6] is set)
             +-------------------+
     52      | drives_length     |    (present if flags[7] is set)
     56      | drives_addr       |    (present if flags[7] is set)
             +-------------------+
     60      | config_table      |    (present if flags[8] is set)
             +-------------------+
     64      | boot_loader_name  |    (present if flags[9] is set)
             +-------------------+
     68      | apm_table         |    (present if flags[10] is set)
             +-------------------+
     72      | vbe_control_info  |    (present if flags[11] is set)
     76      | vbe_mode_info     |
     80      | vbe_mode          |
     82      | vbe_interface_seg |
     84      | vbe_interface_off |
     86      | vbe_interface_len |
             +-------------------+
     88      | framebuffer_addr  |    (present if flags[12] is set)
     96      | framebuffer_pitch |
     100     | framebuffer_width |
     104     | framebuffer_height|
     108     | framebuffer_bpp   |
     109     | framebuffer_type  |
     110-115 | color_info        |
             +-------------------+
```

The first longword indicates the presence and validity of other fields in the Multiboot information structure. All as-yet-undefined bits must be set to zero by the boot loader. Any set bits that the operating system does not understand should be ignored. Thus, the ‘flags’ field also functions as a version indicator, allowing the Multiboot information structure to be expanded in the future without breaking anything.   

> 첫 번째 longword는 멀티부팅 정보 구조에 있는 다른 필드의 존재와 유효성을 나타냅니다. <br>모든 as-yet-Undefined 비트는 부트 로더에 의해 0으로 설정되어야 합니다. <br>운영 체제에서 이해할 수 없는 설정된 비트는 무시해야 합니다. <br>따라서 'flags' 필드는 버전 지표로도 기능하여 향후에 어떤 것도 깨뜨리지 않고 멀티부팅 정보 구조를 확장할 수 있습니다. 

If bit 0 in the ‘flags’ word is set, then the ‘mem_*’ fields are valid. ‘mem_lower’ and ‘mem_upper’ indicate the amount of lower and upper memory, respectively, in kilobytes. Lower memory starts at address 0, and upper memory starts at address 1 megabyte. The maximum possible value for lower memory is 640 kilobytes. The value returned for upper memory is maximally the address of the first upper memory hole minus 1 megabyte. It is not guaranteed to be this value.    

> 'flags' 워드의 비트 0이 설정된 경우 'mem_*' 필드가 유효합니다.<br> 'mem_lower' 및 'mem_upper'는 각각 하위 및 상위 메모리의 양을 킬로바이트로 나타냅니다. <br> 메모리는 주소 0에서 시작하고, 상위 메모리는 주소 1MB에서 시작합니다.<br> 낮은 메모리에 대해 가능한 최대값은 640kbyte입니다. 상위 메모리에 대해 반환되는 값은 최대 1MB를 뺀 첫 번째 상위 메모리 hole의 주소입니다. 이 값은 보장되지 않습니다. 

If bit 1 in the ‘flags’ word is set, then the ‘boot_device’ field is valid, and indicates which bios disk device the boot loader loaded the OS image from. If the OS image was not loaded from a bios disk, then this field must not be present (bit 3 must be clear). The operating system may use this field as a hint for determining its own root device, but is not required to. 

> 'flags' 워드의 비트 1을 설정하면 'boot_device' 필드가 유효하며 부팅 로더가 OS 이미지를 로드한 바이오스 디스크 디바이스를 나타냅니다. OS 이미지가 바이오스 디스크에서 로드되지 않은 경우 이 필드가 없어야 합니다(비트 3은 삭제되어야 함). 운영 체제는 이 필드를 자신의 루트 장치를 결정하기 위한 힌트로 사용할 수 있지만 반드시 사용할 필요는 없습니다.

The ‘boot_device’ field is laid out in four one-byte subfields as follows: 

>  boot_device 필드는 다음과 같이 4개의 1바이트 하위 필드에 배치됩니다.

```
     +-------+-------+-------+-------+
     | part3 | part2 | part1 | drive |
     +-------+-------+-------+-------+
     Least significant             Most significant
```

The most significant byte contains the bios drive number as understood by the bios INT 0x13 low-level disk interface: e.g. 0x00 for the first floppy disk or 0x80 for the first hard disk.

> 가장 중요한 바이트에는 바이오스 INT 0x13 로우 레벨 디스크 인터페이스(예: 첫 번째 플로피 디스크의 경우 0x00 또는 첫 번째 하드 디스크의 경우 0x80)에서 파악한 바이오스 드라이브 번호가 포함되어 있습니다.   

The three remaining bytes specify the boot partition. ‘part1’ specifies the top-level partition number, ‘part2’ specifies a sub-partition in the top-level partition, etc. Partition numbers always start from zero. Unused partition bytes must be set to 0xFF. For example, if the disk is partitioned using a simple one-level DOS partitioning scheme, then ‘part1’ contains the DOS partition number, and ‘part2’ and ‘part3’ are both 0xFF. As another example, if a disk is partitioned first into DOS partitions, and then one of those DOS partitions is subdivided into several BSD partitions using BSD's disklabel strategy, then ‘part1’ contains the DOS partition number, ‘part2’ contains the BSD sub-partition within that DOS partition, and ‘part3’ is 0xFF.    

> 나머지 세 바이트는 부팅 파티션을 지정합니다.<br> 'part1'은 최상위 파티션 번호를 지정하고, 'part2'는 최상위 파티션 등의 하위 파티션을 지정합니다. <br>파티션 번호는 항상 0에서 시작합니다. 사용되지 않는 파티션 바이트는 0xFF로 설정해야 합니다. <br>예를 들어 디스크가 간단한 1-수준 DOS 파티션 구성표를 사용하여 파티셔닝된 경우 'part1'에는 DOS 파티션 번호가 포함되어 있으며 'part2'와 'part3'은 모두 0xFF입니다.<br> 다른 예로, 디스크를 먼저 DOS 파티션으로 분할한 다음 이러한 DOS 파티션 중 하나가 BSD 디스크 레이블 전략을 사용하여 여러 BSD 파티션으로 분할된 경우 'part1'에는 DOS 파티션 번호가 포함되고 'part2'에는 해당 DOS 파티션 내에 BSD 하위 파티션이 포함되며 'part3'은 0xFF입니다

DOS extended partitions are indicated as partition numbers starting from 4 and increasing, rather than as nested sub-partitions, even though the underlying disk layout of extended partitions is hierarchical in nature. For example, if the boot loader boots from the second extended partition on a disk partitioned in conventional DOS style, then ‘part1’ will be 5, and ‘part2’ and ‘part3’ will both be 0xFF.    

> DOS 확장 파티션은 기본적으로 확장 파티션의 기본 디스크 레이아웃이 계층적이더라도 중첩된 하위 파티션이 아니라 4부터 증가된 파티션 번호로 표시됩니다. 예를 들어 부팅 로더가 기존 DOS 스타일로 분할된 디스크의 두 번째 확장 파티션에서 부팅되는 경우 'part1'은 5가 되고 'part2'와 'part3'은 모두 0xFF가 됩니다. 

If bit 2 of the ‘flags’ longword is set, the ‘cmdline’ field is valid, and contains the physical address of the command line to be passed to the kernel. The command line is a normal C-style zero-terminated string. The exact format of command line is left to OS developpers. General-purpose boot loaders should allow user a complete control on command line independently of other factors like image name.  Boot loaders with specific payload in mind may completely or partially generate it algorithmically.    

> 'flags' longword의 비트 2를 설정하면 'cmdline' 필드가 유효하며 커널에 전달할 명령줄의 실제 주소가 포함됩니다. 명령줄은 일반 C-스타일 제로 종단 문자열입니다. 정확한 명령줄 형식은 OS 디벨로퍼에 그대로 유지됩니다. 범용 부트 로더는 이미지 이름과 같은 다른 요인과는 별개로 사용자가 명령줄을 완전히 제어할 수 있도록 허용해야 합니다.  특정 페이로드(payload)를 염두에 둔 부팅 로더는 알고리즘적으로 완전히 또는 부분적으로 생성될 수 있습니다. 

If bit 3 of the ‘flags’ is set, then the ‘mods’ fields indicate to the kernel what boot modules were loaded along with the kernel image, and where they can be found. ‘mods_count’ contains the number of modules loaded; ‘mods_addr’ contains the physical address of the first module structure. 

> 'flags' 중 비트 3을 설정하면 'mods' 필드가 커널에 커널 이미지와 함께 로드된 부팅 모듈과 해당 모듈을 찾을 수 있는 위치를 나타냅니다. 'mods_count'에는 로드된 모듈 수가 포함되며, 'mods_addr'에는 첫 번째 모듈 구조의 물리적 주소가 포함됩니다. 

‘mods_count’ may be zero, indicating no boot modules were loaded, even if bit 3 of ‘flags’ is set. Each module structure is formatted as follows: 

> 'mods_count'는 0일 수 있으며, 이는 'flags'의 비트 3을 설정한 경우에도 부팅 모듈이 로드되지 않았음을 나타냅니다. 각 모듈 구조는 다음과 같이 포맷됩니다.

```
             +-------------------+
     0       | mod_start         |
     4       | mod_end           |
             +-------------------+
     8       | string            |
             +-------------------+
     12      | reserved (0)      |
             +-------------------+
```

The first two fields contain the start and end addresses of the boot module itself. The ‘string’ field provides an arbitrary string to be associated with that particular boot module; it is a zero-terminated ASCII string, just like the kernel command line. The ‘string’ field may be 0 if there is no string associated with the module. Typically the string might be a command line (e.g. if the operating system treats boot modules as executable programs), or a pathname (e.g. if the operating system treats boot modules as files in a file system), but its exact use is specific to the operating system. The ‘reserved’ field must be set to 0 by the boot loader and ignored by the operating system.    

> 처음 두 필드에는 부팅 모듈 자체의 시작 및 종료 주소가 포함됩니다. 문자열' 필드는 커널 명령줄과 마찬가지로 0으로 종단 처리된 ASCII 문자열입니다. 모듈과 연결된 문자열이 없는 경우 '스트링' 필드는 0일 수 있습니다. 일반적으로 문자열은 명령줄(예: 운영 체제가 부팅 모듈을 실행 가능한 프로그램으로 취급하는 경우) 또는 경로 이름(예: 운영 체제가 부팅 모듈을 파일 시스템의 파일로 취급하는 경우)일 수 있지만 정확한 사용은 운영 체제에 따라 다릅니다. '예약됨' 필드는 부팅 로더에서 0으로 설정하고 운영 체제에서 무시해야 합니다. 

**Caution:** Bits 4 & 5 are mutually exclusive.    

> **주의사항:** 비트 4, 5는 상호 배타적입니다.

If bit 4 in the ‘flags’ word is set, then the following fields in the Multiboot information structure starting at byte 28 are valid: 

> 'flags' 워드의 비트 4가 설정된 경우 바이트 28부터 시작하는 멀티부팅 정보 구조의 다음 필드가 유효합니다.

```
             +-------------------+
     28      | tabsize           |
     32      | strsize           |
     36      | addr              |
     40      | reserved (0)      |
             +-------------------+
```

These indicate where the symbol table from an a.out kernel image can be found. ‘addr’ is the physical address of the size (4-byte unsigned long) of an array of a.out format nlist structures, followed immediately by the array itself, then the size (4-byte unsigned long) of a set of zero-terminated ascii strings (plus sizeof(unsigned long) in this case), and finally the set of strings itself. ‘tabsize’ is equal to its size parameter (found at the beginning of the symbol section), and ‘strsize’ is equal to its size parameter (found at the beginning of the string section) of the following string table to which the symbol table refers. Note that ‘tabsize’ may be 0, indicating no symbols, even if bit 4 in the ‘flags’ word is set.    

> 이는 a.out 커널 이미지의 기호 테이블을 찾을 수 있는 위치를 나타냅니다. 'addr'은 a.out 형식 nlist 구조 배열의 크기(4바이트 비서명 길이)에 대한 실제 주소이며, 그 다음 배열 바로 뒤에 배열 자체의 크기(4바이트 비서명 길이)를 의미하며, 마지막으로 문자열 자체의 크기(4바이트 미서명 길이)를 나타냅니다. 'tabsize'는 기호 섹션의 시작 부분에서 찾을 수 있는 크기 매개 변수와 동일하며, 'strize'는 기호 테이블이 참조하는 다음 문자열 테이블의 크기 매개 변수와 동일합니다. '태그 크기'는 0일 수 있으며, '플래그' 단어의 비트 4가 설정된 경우에도 기호가 없음을 나타냅니다. 

If bit 5 in the ‘flags’ word is set, then the following fields in the Multiboot information structure starting at byte 28 are valid: 

> 'flags' 워드의 비트 5를 설정하면 바이트 28에서 시작하는 멀티부팅 정보 구조의 다음 필드가 유효합니다.

```
             +-------------------+
     28      | num               |
     32      | size              |
     36      | addr              |
     40      | shndx             |
             +-------------------+
```

These indicate where the section header table from an ELF kernel is, the size of each entry, number of entries, and the string table used as the index of names. They correspond to the ‘shdr_*’ entries (‘shdr_num’, etc.) in the Executable and Linkable Format (elf) specification in the program header. All sections are loaded, and the physical address fields of the elf section header then refer to where the sections are in memory (refer to the i386 elf documentation for details as to how to read the section header(s)). Note that ‘shdr_num’ may be 0, indicating no symbols, even if bit 5 in the ‘flags’ word is set.    

> 이러한 표는 ELF 커널의 섹션 헤더 테이블 위치, 각 항목의 크기, 항목 수 및 이름 색인으로 사용되는 문자열 테이블을 나타냅니다. 프로그램 헤더의 실행 가능 및 연결 가능 형식(elf) 규격의 'shdr_*' 항목('shdr_num')에 해당합니다. 모든 섹션이 로드되고 엘프 섹션 헤더의 물리적 주소 필드는 섹션이 메모리에 있는 위치를 참조합니다(섹션 헤더를 읽는 방법에 대한 자세한 내용은 i386 엘프 설명서를 참조하십시오). 'shdr_num'은 0일 수 있으며, 'flags' 단어의 비트 5가 설정된 경우에도 기호가 없음을 나타냅니다.

If bit 6 in the ‘flags’ word is set, then the ‘mmap_*’ fields are valid, and indicate the address and length of a buffer containing a memory map of the machine provided by the bios. ‘mmap_addr’ is the address, and ‘mmap_length’ is the total size of the buffer. The buffer consists of one or more of the following size/structure pairs (‘size’ is really used for skipping to the next pair): 

> 'flags' 워드의 비트 6을 설정하면 'mmap_*' 필드가 유효하며, 바이오스에서 제공하는 시스템의 메모리 맵이 포함된 버퍼의 주소와 길이를 나타냅니다. 'mmap_addr'은 주소이고, 'mmap_length'는 버퍼의 총 크기입니다. 버퍼는 다음 크기/구조 쌍 중 하나 이상으로 구성됩니다('size'는 실제로 다음 쌍으로 건너뛰는 데 사용됩니다).

```
             +-------------------+
     -4      | size              |
             +-------------------+
     0       | base_addr         |
     8       | length            |
     16      | type              |
             +-------------------+
```

where ‘size’ is the size of the associated structure in bytes, which can be greater than the minimum of 20 bytes. ‘base_addr’ is the starting address. ‘length’ is the size of the memory region in bytes.  ‘type’ is the variety of address range represented, where a value of 1 indicates available ram, value of 3 indicates usable memory holding ACPI information, value of 4 indicates reserved memory which needs to be preserved on hibernation, value of 5 indicates a memory which is occupied by defective RAM modules and all other values currently indicated a reserved area.    

> 여기서 'size'는 최소 20바이트보다 클 수 있는 관련 구조의 크기입니다. base_addr'이(가) 시작 주소입니다. 'length'는 메모리 영역의 크기(바이트)입니다.  '유형'은 표시된 주소 범위의 다양성입니다. 여기서 1은 사용 가능한 램, 3은 ACPI 정보를 포함하는 사용 가능한 메모리를 나타내며, 4는 최대 절전 모드에서 보존해야 하는 메모리를 나타내며, 5는 결함 있는 RAM 모듈 및 현재 표시된 모든 다른 값을 나타냅니다.예약 구역에 대해 알려드립니다. 

The map provided is guaranteed to list all standard ram that should be available for normal use.   

> 제공된 map에는 정상적으로 사용할 수 있어야 하는 모든 표준 램이 나열되어 있습니다.    

If bit 7 in the ‘flags’ is set, then the ‘drives_*’ fields are valid, and indicate the address of the physical address of the first drive structure and the size of drive structures. ‘drives_addr’ is the address, and ‘drives_length’ is the total size of drive structures. Note that ‘drives_length’ may be zero.

> 'flags'의 비트 7을 설정하면 'drives_*' 필드가 유효하며 첫 번째 드라이브 구조의 물리적 주소와 드라이브 구조의 크기를 나타냅니다. 'drives_addr'은 주소이며, 'drives_length'는 드라이브 구조의 총 크기입니다. 'drives_length'는 0일 수 있습니다. 

 Each drive structure is formatted as follows: 

> 각 드라이브 구조는 다음과 같이 포맷됩니다.

```
             +-------------------+
     0       | size              |
             +-------------------+
     4       | drive_number      |
             +-------------------+
     5       | drive_mode        |
             +-------------------+
     6       | drive_cylinders   |
     8       | drive_heads       |
     9       | drive_sectors     |
             +-------------------+
     10 - xx | drive_ports       |
             +-------------------+
```

The ‘size’ field specifies the size of this structure. The size varies, depending on the number of ports. Note that the size may not be equal to (10 + 2 * the number of ports), because of an alignment.  

> 'size' 필드는 이 구조의 크기를 지정합니다. 포트 수에 따라 크기가 달라집니다. 선형으로 인해 크기가 (10 + 2 * 포트 수)와 같지 않을 수 있습니다.  

The ‘drive_number’ field contains the BIOS drive number. The ‘drive_mode’ field represents the access mode used by the boot loader. Currently, the following modes are defined:      

> 'drive_number' 필드에는 BIOS 드라이브 번호가 포함되어 있습니다. 'drive_mode' 필드는 부트 로더가 사용하는 액세스 모드를 나타냅니다. 현재 다음과 같은 모드가 정의되어 있습니다. 

- ‘0’

  CHS mode (traditional cylinder/head/sector addressing mode).       

  > CHS 모드(기존 실린더/헤드/섹터 주소 지정 모드)입니다. 

- ‘1’

  LBA mode (Logical Block Addressing mode).  

  > LBA 모드(논리 블록 주소 지정 모드)입니다.

The three fields, ‘drive_cylinders’, ‘drive_heads’ and ‘drive_sectors’, indicate the geometry of the drive detected by the bios. ‘drive_cylinders’ contains the number of the cylinders. ‘drive_heads’ contains the number of the heads. ‘drive_sectors’ contains the number of the sectors per track.

> 'drive_cylinders', 'drive_heads' 및 'drive_seectors' 세 필드는 바이오스가 탐지한 드라이브의 형상을 나타냅니다. 'drive_cylinders'에는 실린더 수가 포함되어 있습니다. 'drive_heads'에는 헤드 수가 포함되어 있습니다. 'drive_seectors'에는 트랙당 섹터 수가 포함됩니다.    

The ‘drive_ports’ field contains the array of the I/O ports used for the drive in the bios code. The array consists of zero or more unsigned two-bytes integers, and is terminated with zero. Note that the array may contain any number of I/O ports that are not related to the drive actually (such as dma controller's ports).    

> 'drive_ports' 필드에는 바이오스 코드의 드라이브에 사용되는 I/O 포트의 배열이 포함되어 있습니다. 어레이는 0개 이상의 서명되지 않은 2바이트 정수로 구성되며 0으로 종료됩니다. 어레이에는 실제로 드라이브와 관련이 없는 I/O 포트가 포함될 수 있습니다(예: dma 컨트롤러의 포트). 

If bit 8 in the ‘flags’ is set, then the ‘config_table’ field is valid, and indicates the address of the rom configuration table returned by the GET CONFIGURATION bios call. If the bios call fails, then the size of the table must be *zero*.    

> 'flags'의 비트 8을 설정하면 'config_table' 필드가 유효하며, GET CONFIGURATION 바이오스 호출에 의해 반환된 Rom 구성 테이블의 주소를 나타냅니다. 바이오스 호출에 실패하는 경우 테이블의 크기는 *0*이어야 합니다. 

If bit 9 in the ‘flags’ is set, the ‘boot_loader_name’ field is valid, and contains the physical address of the name of a boot loader booting the kernel. The name is a normal C-style zero-terminated string.    

> 'flags'의 비트 9를 설정하면 'boot_loader_name' 필드가 유효하며 커널을 부팅하는 부트 로더 이름의 실제 주소가 포함됩니다. 이름은 일반 C-스타일 제로 종단 문자열입니다. 

If bit 10 in the ‘flags’ is set, the ‘apm_table’ field is valid, and contains the physical address of an apm table defined as below: 

> 'flags'의 비트 10을 설정하면 'apm_table' 필드가 유효하며, 아래와 같이 정의된 apm 테이블의 실제 주소가 포함됩니다.

```
             +----------------------+
     0       | version              |
     2       | cseg                 |
     4       | offset               |
     8       | cseg_16              |
     10      | dseg                 |
     12      | flags                |
     14      | cseg_len             |
     16      | cseg_16_len          |
     18      | dseg_len             |
             +----------------------+
```

The fields ‘version’, ‘cseg’, ‘offset’, ‘cseg_16’, ‘dseg’, ‘flags’, ‘cseg_len’, ‘cseg_16_len’, ‘dseg_len’ indicate the version number, the protected mode 32-bit code segment, the offset of the entry point, the protected mode 16-bit code segment, the protected mode 16-bit data segment, the flags, the length of the protected mode 32-bit code segment, the length of the protected mode 16-bit code segment, and the length of the protected mode 16-bit data segment, respectively. Only the field ‘offset’ is 4 bytes, and the others are 2 bytes. See [Advanced Power Management (APM) BIOS Interface Specification](http://www.microsoft.com/hwdev/busbios/amp_12.htm), for more information.    

> 'version', 'cseg', 'offset', 'cseg_16', 'flags', 'cseg_len', 'cseg_16_len', 'dseg_len' 필드는 버전 번호, 보호 모드 32비트 코드 세그먼트, 보호 모드 16비트 코드 세그먼트, 보호 모드 16비트 데이터 플래그, 32 모드 길이를 나타냅니다. 각각 보호 모드 16비트 코드 세그먼트의 길이와 보호 모드 16비트 데이터 세그먼트의 길이입니다. 필드 'offset'만 4바이트이고 다른 필드도 2바이트입니다. 자세한 내용은 [Advanced Power Management (APM) BIOS Interface Specification](http://www.microsoft.com/hwdev/busbios/amp_12.htm)을 참조하시기 바랍니다. 

If bit 11 in the ‘flags’ is set, the vbe table is available.    

> 'flags'의 비트 11이 설정된 경우 vbe 테이블을 사용할 수 있습니다. 

The fields ‘vbe_control_info’ and ‘vbe_mode_info’ contain the physical addresses of vbe control information returned by the vbe Function 00h and vbe mode information returned by the vbe Function 01h, respectively.    

> 'vbe_control_info' 및 'vbe_mode_info' 필드에는 각각 vbe 기능 00h 및 vbe 기능 01h에서 반환된 vbe 모드 정보가 반환되는 vbe 제어 정보의 물리적 주소가 포함됩니다. 

The field ‘vbe_mode’ indicates current video mode in the format specified in vbe 3.0.    

> 'vbe_mode' 필드는 vbe 3.0에 지정된 형식으로 현재 비디오 모드를 나타냅니다. 

The rest fields ‘vbe_interface_seg’, ‘vbe_interface_off’, and ‘vbe_interface_len’ contain the table of a protected mode interface defined in vbe 2.0+. If this information is not available, those fields contain zero. Note that vbe 3.0 defines another protected mode interface which is incompatible with the old one. If you want to use the new protected mode interface, you will have to find the table yourself.  

> 나머지 필드 'vbe_interface_seg', 'vbe_interface_off' 및 'vbe_interface_len'에는 vbe 2.0+에 정의된 보호 모드 인터페이스의 테이블이 포함되어 있습니다. 이 정보를 사용할 수 없는 경우 해당 필드에 0이 포함됩니다. vbe 3.0은 이전 모드와 호환되지 않는 다른 보호 모드 인터페이스를 정의합니다. 새 보호 모드 인터페이스를 사용하려면 직접 테이블을 찾아야 합니다.

The fields for the graphics table are designed for vbe, but Multiboot boot loaders may simulate vbe on non-vbe modes, as if they were vbe modes.    

> 그래픽 테이블의 필드는 vbe용으로 설계되었지만 멀티부팅 부트 로더는 vbe가 아닌 모드에서 마치 vbe 모드인 것처럼 vbe를 시뮬레이션할 수 있습니다. 

If bit 12 in the ‘flags’ is set, the Framebuffer table is available.    

> 'flags'의 비트 12가 설정된 경우 Framebuffer 테이블을 사용할 수 있습니다. 

The field ‘framebuffer_addr’ contains framebuffer physical address. This field is 64-bit wide but bootloader should set it under 4 GiB if possible for compatibility with kernels which aren't aware of PAE or AMD64. The field ‘framebuffer_pitch’ contains the framebuffer pitch in bytes. The fields ‘framebuffer_width’, ‘framebuffer_height’ contain the framebuffer dimensions in pixels. The field ‘framebuffer_bpp’ contains the number of bits per pixel. If ‘framebuffer_type’ is set to ‘0’ it means indexed color will be used.

> framebuffer_addr' 필드에는 framebuffer 물리적 주소가 포함되어 있습니다. 이 필드는 64비트이지만 PAE 또는 AMD64를 모르는 커널과의 호환성을 위해 가능하면 부트로더가 4 GiB 이하로 설정해야 합니다. framebuffer_pitch' 필드에는 프레임 버퍼 피치가 바이트 단위로 포함됩니다. framebuffer_width', 'framebuffer_height' 필드에는 framebuffer 치수가 픽셀로 포함됩니다. framebuffer_bpp 필드에는 픽셀당 비트 수가 포함됩니다. 'framebuffer_type'이 '0'으로 설정되어 있으면 색인화된 색상이 사용된다는 뜻입니다. 

 In this case color_info is defined as follows: 

> 이 경우 color_info는 다음과 같이 정의됩니다.

```
             +----------------------------------+
     110     | framebuffer_palette_addr         |
     114     | framebuffer_palette_num_colors   |
             +----------------------------------+
```

‘framebuffer_palette_addr’ contains the address of the color palette, which is an array of color descriptors. Each color descriptor has the following structure: 

> 'framebuffer_palette_addr'은 색상 설명자의 배열인 색상 팔레트의 주소를 포함합니다. 각 색상 설명자의 구조는 다음과 같습니다.

```
             +-------------+
     0       | red_value   |
     1       | green_value |
     2       | blue_value  |
             +-------------+
```

If ‘framebuffer_type’ is set to ‘1’ it means direct RGB color will be used. Then color_type is defined as follows: 

> 'framebuffer_type'이 '1'로 설정되어 있으면 RGB 색상이 직접 사용된다는 뜻입니다. 그런 다음 color_type은 다음과 같이 정의됩니다.

```
             +----------------------------------+
     110     | framebuffer_red_field_position   |
     111     | framebuffer_red_mask_size        |
     112     | framebuffer_green_field_position |
     113     | framebuffer_green_mask_size      |
     114     | framebuffer_blue_field_position  |
     115     | framebuffer_blue_mask_size       |
             +----------------------------------+
```

If ‘framebuffer_type’ is set to ‘2’ it means EGA-standard text mode will be used. In this case ‘framebuffer_width’ and ‘framebuffer_height’ are expressed in characters instead of pixels.  

> 'framebuffer_type'을 '2'로 설정하면 EGA 표준 텍스트 모드가 사용됩니다. 이 경우 'framebuffer_width' 및 'framebuffer_height'는 픽셀 대신 문자로 표시됩니다.

‘framebuffer_bpp’ is equal to 16 (bits per character) and ‘framebuffer_pitch’ is expressed in bytes per text line.  All further values of ‘framebuffer_type’ are reserved for future expansion. 

>  'framebuffer_bpp'는 16(문자당 비트 수)이며 'framebuffer_pitch'는 텍스트 라인당 바이트로 표시됩니다.  이후의 모든 'framebuffer_type' 값은 향후 확장을 위해 예약됩니다.
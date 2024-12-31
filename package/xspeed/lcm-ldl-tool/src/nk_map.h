#ifndef __NK_MAP_H__
#define __NK_MAP_H__

// typedef unsigned char uint8_t;
// typedef unsigned int  uint32_t;

// �ڲ�flash
#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_SECTOR_MASK (FLASH_SECTOR_SIZE - 1)
#define DRV_FLASH_BASE (((uint32_t)0x08000000)) // inner flash start addr
#ifndef FLASH_PAGE_SIZE
#define FLASH_PAGE_SIZE ((uint16_t)0x200) // inner flash page size
#endif

// #define DBG_ON_BOARD    // ͨ�����Կ����� - 32K

// Flash�ռ����
typedef struct
{
  unsigned char boot[(FLASH_SECTOR_SIZE >> 2) * 20];   // boot
  unsigned char app[(FLASH_SECTOR_SIZE >> 2) * 40];    // app
  unsigned char rfu[(FLASH_SECTOR_SIZE >> 2) * 3];     // rfu
  unsigned char syszone[(FLASH_SECTOR_SIZE >> 2) * 1]; // syszone ���� -- ����
} FLASH_ROM;                                           // inner flash total size 64KB

#define SA_FLASH_BASE DRV_FLASH_BASE
#define GET_MEMBER_ADDR_(_member) \
  (SA_FLASH_BASE + (unsigned int)&((FLASH_ROM *)0)->_member)
#define GET_MEMBER_SIZE_(_member) sizeof(((FLASH_ROM *)0)->_member)

typedef enum
{ /* �ռ��ַ���� */
  SA_BOOT = GET_MEMBER_ADDR_(boot),
  SA_APP = GET_MEMBER_ADDR_(app),
#ifndef DBG_ON_BOARD
  SA_SYSZONE = GET_MEMBER_ADDR_(syszone),
#endif
  SA_RFU = GET_MEMBER_ADDR_(rfu),
  // SA_APPREG         = GET_MEMBER_ADDR_(appreg     ),
} flash_parting_addr_t;

typedef enum
{ /* �ռ䳤�ȶ��� */
  LEN_BOOT = GET_MEMBER_SIZE_(boot),
  LEN_APP = GET_MEMBER_SIZE_(app),
#ifndef DBG_ON_BOARD
  LEN_SYSZONE = GET_MEMBER_SIZE_(syszone),
#endif
  LEN_RFU = GET_MEMBER_SIZE_(rfu),
  // LEN_APPREG        = GET_MEMBER_SIZE_(appreg       ),
} flash_parting_size_t;

#ifdef DBG_ON_BOARD
/* DEV-BOARD FLASH size equal 32K,syszone place APP - PAGESIZE */
#define SA_SYSZONE (SA_APP - FLASH_PAGE_SIZE)
#define LEN_SYSZONE (FLASH_PAGE_SIZE)
#endif

/* MCU bin codeinfo parse */
#define BIN_MAGICNUM_START 0xAA5555AA
#define BIN_MAGICNUM_END 0x55AAAA55

#define BOOT_FLASH_BASE (((uint32_t)0x08000000)) //
#define APP_FLASH_BASE (((uint32_t)0x08005000))  //
#define BIN_INFO_OFFSET (((uint32_t)0x00C0))

/* Ӧ����Ϣ�ṹ�嶨�� */
typedef struct
{
  unsigned int MagicStart;
  unsigned char Name[32]; // x-speed nk05 boot
  // unsigned char Ver[16]; // V1.0.0
  unsigned int Ver;            // ver[3]=descript,default:0x00 =debug prototype; ver[2-1-0]=ver info
  unsigned char BuildTime[24]; // Oct 25 2021 10:53:52
  unsigned char Descript[16];  // �豸��;��Ϣ
  unsigned int filesize;       // 0
  // unsigned int  filecrc; // 0 - noused
  unsigned int MagicEnd;
} bin_info_t;

#define BMP_COMPANY_LOGO_INDEX 0xFE

#define BMP_SIZE_DEF_ (1536)
#define BMP_NUM_MAX_ 20
// #pragma pack(1)
typedef struct
{
  unsigned char bmp_index; // 0-BMP_NUM_MAX_
  unsigned char type;      // 0-lattice other-bmp
  unsigned char rfu[2];
  unsigned char bmp_desc[32]; // data descriptions --string
  unsigned int datalen;       // file size
  unsigned int width;
  unsigned int heigth;
  unsigned char rfubuf[64 - 4 - 32 - 12 - 4];
  unsigned int crc32; // bmp_info crc32
                      // unsigned char bmp_dat[BMP_SIZE_DEF_];
} BMP_FILE_DESC;
// #pragma pack()

#endif /*__NK_MAP_H__ */

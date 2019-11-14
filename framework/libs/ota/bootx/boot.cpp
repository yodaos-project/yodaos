#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include "winadapt.h"
#else
#include <unistd.h>
#include <stdint.h>
#endif
#include <string.h>
#include <iostream>
#include <fstream>

#include "boot.hpp"
#include "str2argv.h"
#include "resolve_conf.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define SINGLE_CMD_MAXLEN 500

// leo.boot
extern unsigned char leo_boot[];
extern unsigned int  leo_boot_len;
// leo_mpw.boot
extern unsigned char leo_mpw_boot[];
extern unsigned int  leo_mpw_boot_len;

struct boot {
	const char *name;
	unsigned char *boot_bin;
	size_t boot_size;
} default_boot[] = {
	{"leo",     leo_boot,     leo_boot_len},
	{"leo_mpw", leo_mpw_boot, leo_mpw_boot_len},
	{NULL,      NULL,         0},
};

enum command_mode{
	COMMON_MODE = 0,
	REBOOT_MODE,

	SERIALDOWN_MODE,
	SERIALDUMP_MODE,

	USBSLAVEDOWN_MODE,
	USBSLAVEDUMP_MODE,
};

typedef struct boot_info_s{
	char file[SINGLE_CMD_MAXLEN];
	unsigned int addr;
	int len;
}boot_info_t;

static boot_info_t g_boot_info;

static unsigned int rom_baudrate = BOOT_DEFAULT_BAUDRATE;
static bool  baudrate_from_args = false;
void save_rom_baudrate(unsigned int baudrate)
{
	rom_baudrate     = baudrate;
	baudrate_from_args = true;
}

unsigned int get_rom_baudrate(void)
{
	return rom_baudrate;
}

bool get_baudrate_from_args(void)
{
	return baudrate_from_args;
}

static unsigned long get_file_size(const char *path)
{
	struct stat statbuff;
	if(stat(path, &statbuff) < 0)
		return -1;
	else
		return statbuff.st_size;
}

int Boot::read_file(const char *name, unsigned char **boot, size_t *size)
{
	FILE *fp;
	size_t read_ret;
	int ret;

	if (name == NULL || boot == NULL || size == NULL)
		return -1;

	*boot = NULL;
	*size = 0;

	fp = fopen(name, "rb");
	if (fp == NULL) {
		printf ("Failed to open file %s\n", name);
		return -1;
	}

	ret = fseek(fp, 0, SEEK_END);
	if (ret != 0) {
		printf ("Failed to seek end\n");
		goto _fail;
	}

	*size = ftell(fp);
	if (*size < 0) {
		printf ("Failed to get file point offset\n");
		goto _fail;
	}

	ret = fseek(fp, 0, SEEK_SET);
	if (ret != 0) {
		printf ("Failed to seek set\n");
		goto _fail;
	}

	*boot = (unsigned char*)malloc(*size);
	if (!*boot) {
		printf ("Failed to malloc, size : %d\n", *size);
		goto _fail;
	}

	read_ret = fread(*boot, 1, *size, fp);
	if (read_ret != *size) {
		printf ("failed to read bootfile, want : %u, actual : %u\n", *size, read_ret);
		free (*boot);
		goto _fail;
	}

	fclose(fp);
	return 0;

_fail:
	fclose(fp);
	return -1;
}

int Boot::get_boot(const char *name, unsigned char **boot, size_t *size, int *need_free)
{
	int ret;
	int i = 0;

	if (boot == NULL || size == NULL || need_free == NULL)
		return -1;

	*boot = NULL;
	*size = 0;
	*need_free = 0;

	for (map<string,string>::iterator it = bootlist.begin(); it != bootlist.end(); it++) {
		if (it->first == name) {
			printf ("want to open file name : %s\n", it->second.c_str());
			ret = read_file(it->second.c_str(), boot, size);
			if (ret != 0)
				return -1;

			*need_free = 1;
			return 0;
		}
	}

	while (default_boot[i].name != NULL) {
		if (strcmp(name, default_boot[i].name) == 0) {
			*boot = default_boot[i].boot_bin;
			*size = default_boot[i].boot_size;
			return 0;
		}
		i++;
	}

	return -1;
}

#if 0
static void usec_sleep(int usec_delay)
{
#ifdef HAVE_NANOSLEEP
	struct timespec ts;
	ts.tv_sec = usec_delay / 1000000;
	ts.tv_nsec = (usec_delay % 1000000) * 1000;
	nanosleep(&ts, NULL);
#else
	usleep(usec_delay);
#endif
}
#endif

bool Boot::InitResource(void)
{
#if defined(linux) || defined(__APPLE__)
	serial = new PosixSerial();
#elif defined(WIN32)
	serial = new WindowSerial();
#endif
	if (!serial) {
		printf ("failed to new a serial object\n");
		return false;
	}

#if defined(linux) || defined(__APPLE__) || defined(WIN32)
	usbslave = new PosixUsbSlave();
#endif
	if (!usbslave) {
		printf ("failed to new a usbslave object\n");
		return false;
	}

	return true;
}

void Boot::ReleaseResource(void)
{
	// if 1 中的代码不应该放到这里面，需重新考虑设备打开和关闭流程
#if 1
	switch (transferMode) {
	case SERIAL:
		if (serial)
			serial->Close();
		break;

	case S1_NONE_S2_NONE_USBSLAVE:
	case S1_SERIAL_S2_USBSLAVE:
	case USBSLAVE:
		if (usbslave)
			usbslave->Close();
		break;
	default:
		break;
	}
#endif

	if (serial)
		delete serial;
	if (usbslave)
		delete usbslave;

	serial = NULL;
	usbslave = NULL;
} 

void Boot::NetConfig(void)
{
	char ip[32] = "";
	char cmd[128];
	struct in_addr addr;

	get_stb_ip(ip);
	if(strcmp(ip, "") == 0){
		get_local_ip(ip);
		if(strcmp(ip, "") == 0)
			printf("Warning: NetConfig get local ip failed. please check.\n");
		addr.s_addr = inet_addr(ip);
		addr.s_addr+= 0x01000000;
		strcpy(ip, inet_ntoa(addr));
	}

	if (strcmp(ip, "") != 0) {
		sprintf(cmd, "config ip %s", ip);
		RunOneCommand(cmd, Boot::WAIT);
		RunOneCommand("config tftpport 2000", Boot::WAIT);
	}
}

int Boot::Wait(const char *s, int echo, int timeout)
{
	int pos = 0;
	uint8_t buf[1024];
	int len = strlen(s);
	int flag;
	int timeout_flag;
	if(timeout)
	{
		serial->setTimeouts(timeout);
	}
	while ( pos < len && pos < 1024) {
		timeout_flag=1;
		if (serial->Read(buf + pos, 1) == 1) {
			flag = 0;
			timeout_flag=0;
			if (buf[pos] == '\r') {
				buf[pos] = '\n';
				flag = 1;
			}
			if (s[pos] == buf[pos]) {
				pos ++;
			}
			else {
				int i;
				if (buf[pos] == '\n' && flag == 1)
					buf[pos] = '\r';
				if (showMessage) {
					for(i = 0; i <= pos; i++)
						printf("%c", buf[i]);
				}
				fflush(stdout);
				pos = 0;
			}
		}
		if((timeout!=0)&&(timeout_flag==1))
		{
			return -1;
		}
	}
	buf[pos] = 0;
	if (echo == SHOW) {
		printf("%s", buf);
		fflush(stdout);
	}

	return strncmp((const char*)buf, s, len);
}

unsigned int Boot::CRC32(unsigned char* pBuffer, unsigned int nSize)
{
	const unsigned int s_nTable[]  =
	{
		0x00000000, 0x04C11DB7, 0x9823B6E, 0xD4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2,
		0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3,
		0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
		0x5BD4B01B, 0x569796C2, 0x52568B75,  0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
		0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,  0x9823B6E0, 0x9CE2AB57, 0x91A18D8E,
		0x95609039,0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,  0xBE2B5B58, 0xBAEA46EF,
		0xB7A96036,0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,  0xD4326D90,
		0xD0F37027,0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
		0xF23A8028,0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A,
		0xEC7DD02D,0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C,
		0x2E003DC5,0x2AC12072,  0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x18AEB13,
		0x54BF6A4,0x808D07D, 0xCC9CDCA,  0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB,
		0x6F52C06C, 0x6211E6B5, 0x66D0FB02,  0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
		0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,  0xACA5C697, 0xA864DB20, 0xA527FDF9,
		0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,  0x8AAD2B2F, 0x8E6C3698,
		0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,  0xE0B41DE7,
		0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
		0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED,
		0xD8FBA05A,  0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85,
		0x738AAD5C, 0x774BB0EB,  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
		0x58C1663D, 0x558240E4, 0x51435D53,  0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
		0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,  0x315D626, 0x7D4CB91, 0xA97ED48,
		0xE56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,  0xF12F560E, 0xF5EE4BB9,
		0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,  0xD727BBB6,
		0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
		0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC,
		0xA379DD7B,  0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD,
		0x81B02D74, 0x857130C3,  0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645,
		0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,  0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
		0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,  0x119B4BE9, 0x155A565E, 0x18197087,
		0x1CD86D30, 0x29F3D35, 0x65E2082, 0xB1D065B, 0xFDC1BEC,  0x3793A651, 0x3352BBE6,
		0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,  0xC5A92679,
		0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
		0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673,
		0xFDE69BC4,  0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
		0x933EB0BB, 0x97FFAD0C,  0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,0xBCB4666D,
		0xB8757BDA, 0xB5365D03, 0xB1F740B4,
	};

	unsigned int nResult = 0xFFFFFFFF;

	while (nSize--)
	{
		nResult = (nResult << 8) ^ s_nTable[(nResult >> 24) ^ *pBuffer++];
	}

	return nResult;
}

int display_progress(int progress)
{
#define MAX_PROGRESS 100
    int i = 0;
	int equal_sign_num, left_num;

    if (progress > 100) {
        printf ("error progress\n");
        return 0;
    }

    printf("\r");
    printf ("[");

	equal_sign_num = progress / 10;
	left_num       = MAX_PROGRESS / 10 - equal_sign_num;

    for (i = 0; i < equal_sign_num; i++)
        printf("=");

	if (left_num > 0) {
		printf(">");

		left_num--;
        for (i = 0; i < left_num; i++)
            printf(" ");
	}

    printf ("]");

    printf("[%2d%%]", progress);
    fflush(stdout);

	if (progress == MAX_PROGRESS)
		printf("\n");

    return 0;
}

/* 32字节长boot文件头 */
struct boot_header{
	unsigned short chip_id;
	unsigned char  chip_type;
	unsigned char  chip_version;
	unsigned short boot_delay;
	unsigned char  baud_rate;
	unsigned char  reserved[25];
};

bool Boot::SendData(const void *data, unsigned int len)
{
	unsigned int i, size;
	char const *wait_char;
	unsigned char buf[SCPU_SPECIAL_STRING_LEN];
	unsigned char *pdata = (unsigned char *)data;
	uint32_t crc;
	struct boot_header *boot_header = (struct boot_header *)data;
	int baud_rate[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

	int stage1_download_mode, stage2_download_mode;
	int ret;

	switch (transferMode) {
	case SERIAL:
		stage1_download_mode = SERIAL;
		stage2_download_mode = SERIAL;
		break;
	case USBSLAVE:
		stage1_download_mode = USBSLAVE;
		stage2_download_mode = USBSLAVE;
		break;
	case S1_SERIAL_S2_USBSLAVE:
		stage1_download_mode = SERIAL;
		stage2_download_mode = USBSLAVE;
		break;
	case S1_NONE_S2_NONE_USBSLAVE:
		stage1_download_mode = NONE;
		stage2_download_mode = NONE;
		break;
	default :
		printf ("download mode is unknow\n");
		return false;
	}

	// download stage1
	if (stage1_download_mode == SERIAL) {
		printf("wait ROM request... please power on or restart the board...\n");

		switch(boot_header->chip_id) {
		case 0x8010:
			switch(boot_header->chip_version){
			case 0x00:
				size = len > SIZE_8K ? SIZE_8K : len;
				wait_char = "X";
				break;
			case 0x01:
				size = len > SIZE_32K ? SIZE_32K : len;
				wait_char = "M";
				break;
			default:
				printf("Warning chip version unsupported, please info nationalchip\n");
				return false;
			}
			break;
		default:
			printf("\nUnsupported chip id[%x], please info nationalchip\n\n", boot_header->chip_id);
			return false;
		}

		ret = serial->Open(devName);
		if (ret == false)
			return false;
		printf("Found serial: %s\n", serial->devname);

		if (get_baudrate_from_args() == true){
			if (serial->Config(get_rom_baudrate(), 8, 1, 'N') == false) {
				serial->Close();
				return false;
			}
		} else if (serial->Config(baud_rate[boot_header->baud_rate], 8, 1, 'N') == false) {
			serial->Close();
			return false;
		}

		fflush(stdout);
		ret = Wait(wait_char, HIDE);
		if (ret != 0) {
			serial->Close();
			return false;
		}

		printf("downloading [1/2] : \n");

		size = size / 4;

		display_progress(0);
		buf[0] = 'Y';
		buf[1] = (size >>  0) & 0xFF;
		buf[2] = (size >>  8) & 0xFF;
		buf[3] = (size >> 16) & 0xFF;
		buf[4] = (size >> 24) & 0xFF;
		serial->Write(buf, 5);
		display_progress(5);

		/* send bootloader stage1 */
		if (boot_header->chip_type == 0x01) {
			size = size * 4 - 4;
			serial->Write(pdata + sizeof(struct boot_header), size);
			serial->Write("boot", 4);
		} else {
			printf("\nUnsupported chip type[%x], please info nationalchip\n\n", boot_header->chip_type);
			serial->Close();
			return false;
		}
		display_progress(100);

		if (boot_header->chip_id == 0x8010 && boot_header->chip_version == 0x01) {
			ret = Wait("F", HIDE);
			if (ret != 0) {
				serial->Close();
				return false;
			}
		}

		//有的串口芯片自带缓存,切换波特率会导致缓存丢失,延时等待确保缓存的数据发送完毕
		for (i = 0; i < boot_header->boot_delay; i++)
			usleep(1000);

		if (serial->Config(BOOT_DEFAULT_BAUDRATE, 8, 1, 'N') == false) {
			serial->Close();
			return false;
		}

		if (Wait("GET", HIDE, 4) != 0) {
			printf("BootROM error: wait for \"GET\",please check stb UART receive !\n");
			serial->Close();
			return false;
		}

		// Choose how to download stage2
		if (stage2_download_mode == USBSLAVE)
			buf[0] = 'U';
		else
			buf[0] = 'S';

		serial->Write(buf, 1);
		serial->Close();
	} else if (stage1_download_mode == USBSLAVE) {
		printf("Please press BOOT button and power on or restart the board...\n");

		unsigned char *rom_32k;

		ret = usbslave->Open();
		if (ret != 0)
			return false;

		size = SIZE_32K - 8;
		rom_32k = (unsigned char *)malloc (SIZE_32K);
		if (!rom_32k) {
			fprintf (stderr, "malloc %u failed\n", SIZE_32K);
			usbslave->Close();
			return false;
		}

		memset (rom_32k, 0, SIZE_32K);
		memcpy (rom_32k, pdata + sizeof(struct boot_header), SIZE_32K - sizeof(struct boot_header));
		crc = CRC32(rom_32k, size);
		// Need to be set as little endian
		*(unsigned int *)(&rom_32k[SIZE_32K - 8]) = crc;//cpu_to_le32(crc);

		printf("downloading [1/2] : \n");
		display_progress(0);
		ret = usbslave->Write(rom_32k, SIZE_32K);
		if (ret != 0) {
			printf ("failed to send 32k\n");
			free (rom_32k);
			usbslave->Close();
			return false;
		}
		display_progress(95);

		ret = usbslave->GetStatus();
		if (ret != 1) {
			fprintf (stderr, "stage1 verification failed\n");
			free (rom_32k);
			usbslave->Close();
			return false;
		}
		display_progress(100);

		ret = usbslave->SendRun();
		if (ret != 0) {
			fprintf (stderr, "failed to send run cmd\n");
			free (rom_32k);
			usbslave->Close();
			return false;
		}
		free (rom_32k);
		usbslave->Close();

		// stage1 起来后, slave 设备节点会断开一次,这里等一下
		usleep(1000 * 1000);

		ret = usbslave->Open();
		if (ret != 0) {
			fprintf(stderr, "failed to open usb device!\n");
			return false;
		}

		memset (buf, 0, SCPU_SPECIAL_STRING_LEN);
		ret = usbslave->DoRx(buf, 3);
		if (ret != 3 || strcmp((char *)buf, "GET") != 0) {
			fprintf (stderr, "failed to receive \"GET\" from host, ret : %d\n", ret);
			usbslave->Close();
			return false;
		}

		// Choose how to download stage2
		memset (buf, 0, SCPU_SPECIAL_STRING_LEN);
		if (stage2_download_mode == USBSLAVE)
			buf[0] = 'U';
		else
			buf[0] = 'S';

		ret = usbslave->DoTx(buf, 1);
		if (ret != 1) {
			fprintf (stderr, "failed to send \'%c\' to host\n", buf[0]);
			usbslave->Close();
			return false;
		}

		usbslave->Close();
	} else if (stage1_download_mode == NONE) {
		printf("downloading [1/2] : \n");
		display_progress(100);
	} else {
		printf ("Unknow download mode of stage1\n");
		return false;
	}

	// download stage2
	printf("downloading [2/2] : \n");
	if (stage2_download_mode == SERIAL) {

		ret = serial->Open(devName);
		if (ret == false)
			return false;
		ret = serial->Config(BOOT_DEFAULT_BAUDRATE, 8, 1, 'N');
		if (ret == false)
			return false;

		crc = 0;
		for (i=0; i < len; i++)
			crc += pdata[i];

		if (boot_header->chip_type == 0x01) {
			serial->Write(&crc, 4);
			serial->Write(&len, 4);
		} else {
			printf("\r\n Warning chip type unsupported, please info nationalchip\r\n\n");
			serial->Close();
			return false;
		}

		/* send bootloader stage2 */
		while (1) {
			i = 0;
			while (i < len) {
				i += serial->Write(pdata + i, MIN(len - i, 2048) );
				display_progress((i / (len * 1.0)) * 100);
			}

			size = serial->Read(buf, 1);
			if (size < 1)
				break;
			if (buf[0] == 'O'){
				break;
			} else if (buf[0] == 'E'){
				printf("data transmit error, try again ...\n");
				fflush(stdout);
			} else {
				printf("buf[0]=%d\n", buf[0]);
				printf("error, please try agin.\n");
				fflush(stdout);
				serial->Close();
				return false;
			}
		}

		// keep serial opening
		;
	} else if (stage2_download_mode == USBSLAVE) {
		if (enter_debug_mode) {
			printf ("Please open the serial port tool to accept the debugging output of the serial port in 5s\n");
			usleep (5 * 1000 * 1000);
		}

		// stage1 起来后, slave 设备节点会断开一次,这里等一下
		if (stage1_download_mode != USBSLAVE)
			usleep(1000 * 1000);

		ret = usbslave->Open();
		if (ret != 0) {
			fprintf(stderr, "Failed to open usbslave\n");
			return false;
		}

		display_progress(0);
		ret = usbslave->DownloadStage2(pdata, len);
		if (ret != 0) {
			fprintf(stderr, "Failed to send stage2\n");
			usbslave->Close();
			return false;
		}
		display_progress(100);

		memset (buf, 0, SCPU_SPECIAL_STRING_LEN);
		ret = usbslave->DoRx(buf, SCPU_SPECIAL_STRING_LEN);
		if (ret != SCPU_SPECIAL_STRING_LEN || buf[0] != 'O') {
			fprintf(stderr, "Failed to verify stage2, ret : %d\n", ret);
			usbslave->Close();
			return false;
		}

		ret = usbslave->RunStage2();
		if (ret != 0) {
			fprintf(stderr, "Failed to send RunStage2 cmd\n");
			usbslave->Close();
			return false;
		}

		// keep usbslave opening
		;
	} else if (stage2_download_mode == NONE) {
		printf("downloading [2/2] : \n");
		display_progress(100);
	} else {
		printf ("Unknow download mode of stage2\n");
		return false;
	}

	// wait for ready
	if (transferMode == S1_NONE_S2_NONE_USBSLAVE) {
		ret = usbslave->Open();
		if (ret != 0) {
			fprintf(stderr, "Failed to open usbslave\n");
			return false;
		}

		// 因为没有交互的同步机制，所以延时等待 usb 准备好
		usleep(500 * 1000);
	}

	return true;
}

// 返回命令执行状态
int Boot::RunOneCommand(const char *cmd, int wait)
{
	char exec_flag = COMMON_MODE;
	const char *new_cmd = cmd_parser(cmd, &exec_flag);
	int len = strlen(new_cmd);
	int ret;

	if (new_cmd == NULL) {
		printf("parse command error\n");
		return RUN_CMD_STOP;
	}

	if(exec_flag == COMMON_MODE){
		char *common_cmd = strdup(new_cmd);
		int argc;
		char **argv;
		const char *err;

		str2argv(common_cmd, &argc, &argv, &err);
		free(common_cmd);

		switch (transferMode) {
		case SERIAL:
			serial->Write(new_cmd, len);
			if (new_cmd[len - 1] != '\n')
				serial->Write("\n", 1);

			printf ("done\n");
			break;

		case USBSLAVE:
		case S1_SERIAL_S2_USBSLAVE:
		case S1_NONE_S2_NONE_USBSLAVE:
			char status_buf[SCPU_SPECIAL_STRING_LEN];

			ret = usbslave->ExcuteCmd(new_cmd);
			if (ret != 0) {
				printf ("Failed to exec cmd\n");
				return RUN_CMD_STOP;
			}

			// wait for start flag
			memset (status_buf, 0, sizeof(status_buf));
			ret = usbslave->DoRx(status_buf, sizeof(status_buf));
			//printf ("status buf : %s\n", status_buf);
			if (ret != sizeof(status_buf)) {
				printf ("Failed to receive start flag\n");
				return RUN_CMD_STOP;
			}

			// wait for finish
			memset (status_buf, 0, sizeof(status_buf));
			ret = usbslave->DoRx(status_buf, sizeof(status_buf));
			//printf ("status buf : %s\n", status_buf);
			if (ret != sizeof(status_buf)) {
				printf ("Failed to receive end flag\n");
				return RUN_CMD_STOP;
			}

			printf ("done\n");
			break;

		default:
			break;
		}

		argv_free(&argc, &argv);
	} else if (exec_flag == SERIALDOWN_MODE) {
		char *buf;
		FILE *file_fp;
		int i, count;
		int unit;
		int act_len = 0;

		file_fp = fopen(g_boot_info.file, "rb");
		if (file_fp == NULL) {
			printf ("Failed to open file %s\n", g_boot_info.file);
			return RUN_CMD_CONTINUE;
		}

		buf = (char*)malloc(g_boot_info.len);
		if(buf == NULL){
			printf("Failed to malloc, size : %d\n", g_boot_info.len);
			fclose (file_fp);
			return RUN_CMD_CONTINUE;
		}

		if ((act_len = fread(buf, 1, g_boot_info.len, file_fp)) != g_boot_info.len)
			printf("Failed to read file, ret : %u, want : %u\n", act_len, g_boot_info.len);

		fclose(file_fp);

		serial->Write(new_cmd, len);
		if (new_cmd[len - 1] != '\n')
			serial->Write("\n", 1);
		if (Wait("~sta~", HIDE, 3) != 0) {
			free(buf);
			printf("Failed to receive \"~sta~\" from stb in 3s\n");
			return RUN_CMD_STOP;
		}

		display_progress (0);
		for(i = 0, count = 0, unit = (g_boot_info.len / 100 + 1); i <= 100; i++){
			if(unit == 0){
				serial->Write(buf, g_boot_info.len);
				break;
			}

			display_progress (i);

			if((count + unit) <= g_boot_info.len) {
				serial->Write((void *)(buf + count), unit);
				count += unit;
			} else {
				serial->Write((void *)(buf + count), g_boot_info.len - count);
				count = g_boot_info.len;
				break;
			}
		}
		free(buf);
		display_progress(100);

		if (Wait("~fin~", HIDE, 3) != 0) {
			printf("Failed to receive \"~fin~\" from stb in 3s, will EXIT_FAILURE\n");
			return RUN_CMD_STOP;
		}

		printf ("done\n");
	} else if (exec_flag == SERIALDUMP_MODE) {
		char *buf;
		FILE *file_fp;
		int i, count;
		int unit;
		int act_len = 0;

		file_fp = fopen(g_boot_info.file, "wb");
		if (file_fp == NULL) {
			printf ("Failed to open file %s\n", g_boot_info.file);
			return RUN_CMD_CONTINUE;
		}

		buf = (char*)malloc(g_boot_info.len);
		if(buf == NULL){
			printf("Failed to malloc memory, size : %u\n", g_boot_info.len);
			return RUN_CMD_CONTINUE;
		}

		serial->Write(new_cmd, len);
		if (new_cmd[len - 1] != '\n')
			serial->Write("\n", 1);
		if (Wait("~sta~", HIDE) != 0) {
			free(buf);
			printf("Failed to receive \"~sta~\" from stb in 3s, will EXIT_FAILURE\n");
			return RUN_CMD_STOP;
		}

		display_progress(0);
		for(i = 0, count = 0, unit = (g_boot_info.len / 100 + 1); i <= 100; i++){
			if(unit == 0){
				serial->BlockRead(buf, g_boot_info.len);
				break;
			}

			display_progress(i);

			if((count + unit) <= g_boot_info.len){
				serial->BlockRead((void *)(buf + count), unit);
				count += unit;
			}
			else{
				serial->BlockRead((void *)(buf + count), g_boot_info.len - count);
				count = g_boot_info.len;

				break;
			}
		}
		display_progress(100);

		if (Wait("~fin~", HIDE, 3) != 0) {
			free(buf);
			printf("Failed to receive \"~fin~\" from stb in 3s, will EXIT_FAILURE\n");
			return RUN_CMD_STOP;
		}

		if ((act_len = fwrite(buf, 1, g_boot_info.len, file_fp)) != g_boot_info.len)
			printf("Failed to write to file, ret : %u, want : %u\n", act_len, g_boot_info.len);

		fclose(file_fp);
		free(buf);

		printf ("done\n");
	} else if (exec_flag == USBSLAVEDOWN_MODE) {
#define MAX_BUFF_SIZE (1 * 1024 * 1024)
		char status_buf[SCPU_SPECIAL_STRING_LEN];
		char *buf;
		FILE *file_fp;
		int read_len;
		int buf_len, write_len, total_len;
		int progress;

		write_len = 0;
		total_len = g_boot_info.len;
		buf_len   = MIN(MAX_BUFF_SIZE, g_boot_info.len);

		buf = (char*)malloc(buf_len);
		if(buf == NULL) {
			printf("Failed to malloc, size : %d\n", buf_len);
			return RUN_CMD_CONTINUE;
		}

		file_fp = fopen(g_boot_info.file, "rb");
		if (!file_fp) {
			printf("Failed to open file : %s\n", g_boot_info.file);
			free (buf);
			return RUN_CMD_CONTINUE;
		}

		// send cmd
		ret = usbslave->ExcuteCmd(new_cmd);
		if (ret != 0) {
			printf ("Failed to send cmd\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		// wait for start
		memset (status_buf, 0, sizeof(status_buf));
		ret = usbslave->DoRx(status_buf, sizeof(status_buf));
		//printf ("%s, recv str : %s\n", __func__, status_buf);
		if (ret != sizeof(status_buf)) {
			printf ("Failed to receive start flag!\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		// send
		while (total_len > 0) {
			write_len = MIN(total_len, buf_len);
			read_len = fread(buf, 1, write_len, file_fp);
			if (read_len != write_len)
				printf ("read file error, want : %d, act : %d\n", write_len, read_len);

			ret = usbslave->DoTx(buf, write_len);
			if (ret != write_len) {
				printf ("Failed to send data\n");
				fclose (file_fp);
				free (buf);
				return RUN_CMD_STOP;
			}

			progress = (((g_boot_info.len - total_len) / (g_boot_info.len * 1.0)) * 100);
			display_progress (progress);

			total_len -= write_len;
		}
		display_progress (100);

		// wait for finish
		memset (status_buf, 0, sizeof(status_buf));
		ret = usbslave->DoRx(status_buf, sizeof(status_buf));
		//printf ("%s, recv str : %s\n", __func__, status_buf);
		if (ret != sizeof(status_buf)) {
			printf ("Failed to receive end flag!!\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		fclose(file_fp);
		free (buf);

		printf ("done\n");
	} else if (exec_flag == USBSLAVEDUMP_MODE) {
		char status_buf[SCPU_SPECIAL_STRING_LEN];
		char *buf;
		FILE *file_fp;
		int act_len = 0;
		int total_len, rd_size, buf_len;
		int progress;

		total_len = g_boot_info.len;
		buf_len   = MIN(MAX_BUFF_SIZE, total_len);

		buf = (char*)malloc(g_boot_info.len);
		if(buf == NULL){
			printf("Failed to malloc, size : %d\n", g_boot_info.len);
			return RUN_CMD_CONTINUE;
		}

		file_fp = fopen(g_boot_info.file, "wb");
		if (!file_fp) {
			printf ("Failed to open file %s\n", g_boot_info.file);
			free (buf);
			return RUN_CMD_CONTINUE;
		}

		// send cmd
		ret = usbslave->ExcuteCmd(new_cmd);
		if (ret != 0) {
			printf ("Failed to send cmd\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		// wait for start flag
		memset (status_buf, 0, sizeof(status_buf));
		ret = usbslave->DoRx(status_buf, sizeof(status_buf));
		//printf ("status buf : %s\n", status_buf);
		if (ret != sizeof(status_buf)) {
			printf ("Failed to receive start flag!!\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		// read data
		while (total_len > 0) {
			rd_size = MIN(total_len, buf_len);

			memset (buf, 0, rd_size);
			ret = usbslave->DoRx(buf, rd_size);
			if (ret != rd_size) {
				printf ("Failed to receive data!\n");
				fclose (file_fp);
				free (buf);
				return RUN_CMD_STOP;
			}

			act_len = fwrite(buf, 1, rd_size, file_fp);
			if (act_len != rd_size)
				printf("write file error, act write len=%d\n", act_len);

			progress = (((g_boot_info.len - total_len) / (g_boot_info.len * 1.0)) * 100);
			display_progress (progress);

			total_len -= rd_size;
		}
		display_progress (100);

		// wait for finish
		memset (status_buf, 0, sizeof(status_buf));
		ret = usbslave->DoRx(status_buf, sizeof(status_buf));
		//printf ("status buf : %s\n", status_buf);
		if (ret != sizeof(status_buf)) {
			printf ("Failed to receive end flag!!\n");
			fclose (file_fp);
			free (buf);
			return RUN_CMD_STOP;
		}

		fclose(file_fp);
		free (buf);

		printf ("done\n");
	} else if (exec_flag == REBOOT_MODE) {
		switch (transferMode) {
		case SERIAL:
			serial->Write(new_cmd, len);
			if (new_cmd[len - 1] != '\n')
				serial->Write("\n", 1);

			printf ("done\n");
			break;

		case USBSLAVE:
		case S1_SERIAL_S2_USBSLAVE:
		case S1_NONE_S2_NONE_USBSLAVE:
			char status_buf[SCPU_SPECIAL_STRING_LEN];

			ret = usbslave->ExcuteCmd(new_cmd);
			if (ret != 0) {
				printf ("Failed to send cmd\n");
				return RUN_CMD_STOP;
			}

			// wait for start flag
			memset (status_buf, 0, sizeof(status_buf));
			ret = usbslave->DoRx(status_buf, sizeof(status_buf));
			//printf ("status buf : %s\n", status_buf);
			if (ret != sizeof(status_buf)) {
				printf ("Failed to receive start flag\n");
				return RUN_CMD_STOP;
			}

			// wait for finish
			//memset (status_buf, 0, sizeof(status_buf));
			//usbslave->DoRx(status_buf, sizeof(status_buf));
			//printf ("status buf : %s\n", status_buf);

			printf ("done\n");
			break;

		default:
			break;
		}

		return RUN_CMD_STOP;
	}

	if (wait == Boot::WAIT) {
		switch (transferMode) {
		case SERIAL:
			if (Wait("boot> ", HIDE) != 0)
				return RUN_CMD_STOP;
		default :
			break;
		}
	}

	return RUN_CMD_CONTINUE;
}

/*
 * download loader loader.bin
 * download <partition> <filename>
 */
const char *Boot::cmd_parser(const char *cmd, char *exec_flag)
{
	char *cmd_dup = strdup(cmd);
	char *retcmd = NULL;
	static char new_cmd[SINGLE_CMD_MAXLEN] = {0};
	int argc;
	char **argv;
	const char *err;
	char *p;

	strcpy(new_cmd, cmd);

	str2argv(cmd_dup, &argc, &argv, &err);
	free(cmd_dup);

	if (strcmp(argv[0], "download") == 0 || strcmp(argv[0], "serialdown") == 0 || strcmp(argv[0], "usbslavedown") == 0) {
		//boot command: serialdown <flash addr> <file name>
		if (argc >= 3) {
			char str[30];
			strcpy(g_boot_info.file, argv[2]);
			g_boot_info.addr = strtol(argv[1], 0, 0);
			g_boot_info.len = get_file_size(argv[2]);
			if (g_boot_info.len < 0) {
				printf("file %s not find, please check.\n", argv[2]);
				exit(EXIT_FAILURE);
			}
			sprintf(str, "%d", g_boot_info.len);

			switch (transferMode) {
			case SERIAL:
				if (strcmp(argv[0], "usbslavedown") == 0)
					printf ("Info : current transmission mode does not support usbslavedown cmd, use serialdown instead\n");

				*exec_flag = SERIALDOWN_MODE;
				sprintf(new_cmd, "serialdown %s %s", argv[1], str);
				break;

			case S1_NONE_S2_NONE_USBSLAVE:
			case S1_SERIAL_S2_USBSLAVE:
			case USBSLAVE:
				if (strcmp(argv[0], "serialdown") == 0)
					printf ("Info : current transmission mode does not support serialdown cmd, use usbslavedown instead\n");

				*exec_flag = USBSLAVEDOWN_MODE;
				sprintf(new_cmd, "usbslavedown %s %s", argv[1], str);
				break;
			default:
				retcmd = NULL;
				goto error;
			}
		}
	} else if (strcmp(argv[0], "dump") == 0 || strcmp(argv[0], "serialdump") == 0 || strcmp(argv[0], "usbslavedump") == 0) {
		//boot command: serialdump <flash addr> <length> <file name>
		if (argc >= 4) {
			char str[30];
			strcpy(g_boot_info.file, argv[3]);
			g_boot_info.addr = strtol(argv[1], 0, 0);
			g_boot_info.len = strtol(argv[2], 0, 0);
			sprintf(str, "%d", g_boot_info.len);

			switch (transferMode) {
			case SERIAL:
				*exec_flag = SERIALDUMP_MODE;
				sprintf(new_cmd, "serialdump %s %s", argv[1], str);
				break;

			case S1_NONE_S2_NONE_USBSLAVE:
			case S1_SERIAL_S2_USBSLAVE:
			case USBSLAVE:
				*exec_flag = USBSLAVEDUMP_MODE;
				sprintf(new_cmd, "usbslavedump %s %s", argv[1], str);
				break;
			default:
				retcmd = NULL;
				goto error;
			}
		}
	} else if (strcmp(argv[0], "reboot") == 0) {
		*exec_flag = REBOOT_MODE;
		strcpy(new_cmd, argv[0]);
	} else {
		/* delete ' in the file string head and tail, which is added by windows pc tools */
		p = argv2str(argc, argv);
		if (p) {
			strcpy(new_cmd, p);
			free(p);
		} else {
			retcmd = NULL;
			goto  error;
		}
	}

	retcmd = new_cmd;
error:
	argv_free(&argc, &argv);
	return retcmd;
}

void Boot::Command(const char *cmd, int wait)
{
	char *cmd_dup;
	char *p;
	int offset = 0;
	int run_cmd_status;

	cmd_dup = (char*)calloc(4096, sizeof(char));
	strcpy(cmd_dup, cmd);
	p = strtok(cmd_dup, ";");
	while(p){
		printf ("Excute cmd : %s\n", p);
		run_cmd_status = RunOneCommand(p, wait);
		if (run_cmd_status == RUN_CMD_STOP)
			break;

		offset += strlen(p) + 1;
		p = strtok(cmd_dup + offset, ";");
	}

	free(cmd_dup);
}

char *Boot::GetMachines(void)
{
	int pos = 0;
	int i = 0;
	static char filename[PATH_MAX];
	map<string,string>::iterator it;

	while (default_boot[i].name != NULL) {
		pos += sprintf(filename + pos, "%s|", default_boot[i].name);
		i++;
	}
	for(it = bootlist.begin(); it != bootlist.end(); it++) {
		int found = 0;
		i = 0;
		while (default_boot[i].name != NULL) {
			if (it->first == default_boot[i].name) {
				found = 1;
				break;
			}
			i++;
		}

		if (found == 0)
			pos += sprintf(filename + pos, "%s|", it->first.c_str());
	}
	filename[strlen(filename) - 1] = 0;

	return filename;
}

void Boot::BootBinInit(void)
{
#ifndef WIN32
	DIR *dirp;
	struct dirent *dp = NULL;
	struct stat filestat;
	char path[PATH_MAX];
	char filename[PATH_MAX];

	sprintf(path, "%s/"BOOTFILES_DIR_NAME, getenv("HOME"));

	if ((dirp = opendir(path)) == NULL)
		return;

	dp = readdir(dirp);

	for (; dp != NULL; dp = readdir(dirp)) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;

		if (dp->d_name[0] == '.')
			continue;

		sprintf(filename, "%s/%s", path, dp->d_name);
		if (stat(filename, &filestat) != 0)
			continue;

		if (S_ISREG(filestat.st_mode)) {
			char *basename = strdup(dp->d_name);
			char *p = rindex(basename, '.');
			if (p)
				*p = 0;
			bootlist.insert(pair<string, string>(basename, filename));
			free(basename);
		}
	}

	closedir(dirp);
#endif
}

bool Boot::Loader(const char *bootfile)
{
	size_t boot_size = 0;
	int need_free = 0;
	unsigned char *boot_bin = NULL;
	char data_buf[SCPU_SPECIAL_STRING_LEN];
	int ret;

	if (extern_bootfile == true){
		ret = read_file(bootfile, &boot_bin, &boot_size);
		if (ret != 0)
			return false;

		need_free = 1;
	} else {
		ret = get_boot(bootfile, &boot_bin, &boot_size, &need_free);
		if (ret < 0) {
			printf("Load boot file failed!!\n");
			return false;
		}
	}

	ret = SendData(boot_bin, boot_size);
	if (need_free)
		free(boot_bin);
	if (ret == false)
		return false;

	switch (transferMode) {
	case SERIAL:
		ret = Wait("boot> ", HIDE, 3);
		if (ret != 0) {
			printf("Before exec the first command, failed to receive \"boot>\" from stb in 3s, will EXIT_FAILURE\n");
			exit(EXIT_FAILURE);
		}
		break;

	case S1_SERIAL_S2_USBSLAVE:
	case USBSLAVE:
		memset (data_buf, 0, SCPU_SPECIAL_STRING_LEN);
		ret = usbslave->DoRx(data_buf, SCPU_SPECIAL_STRING_LEN);
		if (ret != SCPU_SPECIAL_STRING_LEN || strcmp((char *)data_buf, "boot>") != 0) {
			printf("Before exec the first command, failed to receive \"boot>\", ret : %d\n", ret);
			return false;
		}
		break;

	case S1_NONE_S2_NONE_USBSLAVE:
		break;
	default :
		printf ("not support this download mode, exit now\n");
		return false;
	}

	return true;
}

void Boot::BootCommand(const char *dev, const char *mach, const char *cmd)
{
	int ret;

	if (!cmd) {
		printf("there is no cmd to exec, please check.\n");
		return ;
	}

	devName = dev;

	ret = InitResource();
	if (ret == false)
		return ;

	if (Loader(mach) == false)
		goto exit;

	Command(cmd, Boot::WAIT);

exit:
	ReleaseResource();
	return ;
}


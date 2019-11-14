/*
 *  Copyright (C) 2013 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Module Name:
 *     hciattach_rtk.c
 *
 *  Description:
 *     H4/H5 specific initialization
 *
 *  Revision History:
 *      Date         Version      Author             Comment
 *     ----------   ---------    ---------------    -----------------------
 *     2013-06-06    1.0.0        gordon_yang        Create
 *     2013-06-18    1.0.1        lory_xu            add support for multi fw
 *     2013-06-21    1.0.2        gordon_yang        add timeout for get version cmd
 *     2013-07-01    1.0.3        lory_xu            close file handle
 *     2013-07-01    2.0          champion_chen      add IC check
 *     2013-12-16    2.1          champion_chen      fix bug in Additional packet number
 *     2013-12-25    2.2          champion_chen      open host flow control after send last fw packet
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <byteswap.h>
#include <netinet/in.h>
#include <ctype.h>
#include <poll.h>

#include "hciattach.h"

#define RTK_VERSION "3.1.52fd320.20180918-174755"

/* #define RTL_8703A_SUPPORT */
/* #define RTL8723DSH4_UART_HWFLOWC */ /* 8723DS H4 special */

#define USE_CUSTOMER_ADDRESS

#define BAUDRATE_4BYTES
#define FIRMWARE_DIRECTORY  "/lib/firmware/rtlbt/"
#define BT_CONFIG_DIRECTORY "/lib/firmware/rtlbt/"

#ifdef USE_CUSTOMER_ADDRESS
#define BT_ADDR_FILE        "/data/bluetooth/bdaddr"
#define CMD_RM_BT_ADDR_FILE        "rm -rf /data/bluetooth/bdaddr"
static uint8_t customer_bdaddr = 0;

extern int read_btmac(uint8_t addr[6]);
#endif

#define CONFIG_TXPOWER	(1 << 0)
#define CONFIG_XTAL	(1 << 1)
#define CONFIG_BTMAC	(1 << 2)

#define EXTRA_CONFIG_OPTION
#ifdef EXTRA_CONFIG_OPTION
#define EXTRA_CONFIG_FILE	"/etc/rtk_btconfig.txt"
static uint32_t config_flags;
static uint8_t txpower_cfg[4];
static uint8_t txpower_len;
static uint8_t xtal_cfg;
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_le16(d)  (d)
#define cpu_to_le32(d)  (d)
#define le16_to_cpu(d)  (d)
#define le32_to_cpu(d)  (d)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le16(d)  bswap_16(d)
#define cpu_to_le32(d)  bswap_32(d)
#define le16_to_cpu(d)  bswap_16(d)
#define le32_to_cpu(d)  bswap_32(d)
#else
#error "Unknown byte order"
#endif

typedef uint8_t RT_U8, *PRT_U8;
typedef int8_t RT_S8, *PRT_S8;
typedef uint16_t RT_U16, *PRT_U16;
typedef int32_t RT_S32, *PRT_S32;
typedef uint32_t RT_U32, *PRT_U32;

RT_U8 DBG_ON = 1;
#define LOG_STR     "Realtek Bluetooth"
#define RS_DBG(fmt, arg...) \
    do{ \
        if (DBG_ON) \
            fprintf(stderr, "%s :" fmt "\n" , LOG_STR, ##arg); \
    }while(0)

#define RS_INFO(fmt, arg...) \
    do{ \
        fprintf(stderr, "%s :" fmt "\n", LOG_STR, ##arg); \
    }while(0)

#define RS_ERR(fmt, arg...) \
    do{ \
        fprintf(stderr, "%s ERROR: " fmt "\n", LOG_STR, ##arg); \
    }while(0)

#define HCI_COMMAND_HDR_SIZE        3
#define HCI_EVENT_HDR_SIZE          2
/* #define RTK_PATCH_LENGTH_MAX        24576	*/ //24*1024
#define RTK_PATCH_LENGTH_MAX        (40 * 1024)
#define PATCH_DATA_FIELD_MAX_SIZE   252
#define READ_DATA_SIZE              16
#define H5_MAX_RETRY_COUNT          40

#define RTK_VENDOR_CONFIG_MAGIC     0x8723ab55
const RT_U8 RTK_EPATCH_SIGNATURE[8] =
    { 0x52, 0x65, 0x61, 0x6C, 0x74, 0x65, 0x63, 0x68 };
const RT_U8 Extension_Section_SIGNATURE[4] = { 0x51, 0x04, 0xFD, 0x77 };

#define HCI_CMD_READ_BD_ADDR                0x1009
#define HCI_VENDOR_CHANGE_BDRATE            0xfc17
#define HCI_VENDOR_READ_RTK_ROM_VERISION    0xfc6d
#define HCI_CMD_READ_LOCAL_VERISION         0x1001
#define HCI_VENDOR_READ_CHIP_TYPE           0xfc61

#define ROM_LMP_NONE            0x0000
#define ROM_LMP_8723a           0x1200
#define ROM_LMP_8723b           0x8723
#define ROM_LMP_8821a           0x8821
#define ROM_LMP_8761a           0x8761
#define ROM_LMP_8761btc		0x8763

#define ROM_LMP_8703a           0x87b3
#define ROM_LMP_8763a           0x8763
#define ROM_LMP_8703b           0x8703
#define ROM_LMP_8723c           0x87c3 /* ??????? */
#define ROM_LMP_8822b           0x8822
#define ROM_LMP_8822c           0x8822
#define ROM_LMP_8723cs_xx       0x8704
#define ROM_LMP_8723cs_cg       0x8705
#define ROM_LMP_8723cs_vf       0x8706

/* Chip type */
#define CHIP_8703AS    1
#define CHIP_8723CS_CG 3
#define CHIP_8723CS_VF 4
#define CHIP_8723CS_XX 5
#define CHIP_8703BS   7

/* software id */
#define CHIP_UNKNOWN	0x00
#define CHIP_8761AT	0x1F
#define CHIP_8761ATF	0x2F
#define CHIP_8761BTC	0x3F
#define CHIP_8761B	0x4F
#define CHIP_8723BS	0x5F
#define CHIP_BEFORE	0x6F
#define CHIP_8822BS	0x70
#define CHIP_8723DS	0x71
#define CHIP_8821CS	0x72
#define CHIP_8822CS	0x73

/* HCI data types */
#define H5_ACK_PKT              0x00
#define HCI_COMMAND_PKT         0x01
#define HCI_ACLDATA_PKT         0x02
#define HCI_SCODATA_PKT         0x03
#define HCI_EVENT_PKT           0x04
#define H5_VDRSPEC_PKT          0x0E
#define H5_LINK_CTL_PKT         0x0F

#define H5_HDR_SEQ(hdr)         ((hdr)[0] & 0x07)
#define H5_HDR_ACK(hdr)         (((hdr)[0] >> 3) & 0x07)
#define H5_HDR_CRC(hdr)         (((hdr)[0] >> 6) & 0x01)
#define H5_HDR_RELIABLE(hdr)    (((hdr)[0] >> 7) & 0x01)
#define H5_HDR_PKT_TYPE(hdr)    ((hdr)[1] & 0x0f)
#define H5_HDR_LEN(hdr)         ((((hdr)[1] >> 4) & 0xff) + ((hdr)[2] << 4))
#define H5_HDR_SIZE             4

struct sk_buff {
	RT_U32 max_len;
	RT_U32 data_len;
	RT_U8 data[0];
};

/* Skb helpers */
struct bt_skb_cb {
	RT_U8 pkt_type;
	RT_U8 incoming;
	RT_U16 expect;
	RT_U8 tx_seq;
	RT_U8 retries;
	RT_U8 sar;
	unsigned short channel;
};

typedef struct {
	uint8_t index;
	uint8_t data[252];
} __attribute__ ((packed)) download_vendor_patch_cp;

struct hci_command_hdr {
	RT_U16 opcode;
	RT_U8 plen;
} __attribute__ ((packed));

struct hci_event_hdr {
	RT_U8 evt;
	RT_U8 plen;
} __attribute__ ((packed));

struct hci_ev_cmd_complete {
	RT_U8 ncmd;
	RT_U16 opcode;
} __attribute__ ((packed));

struct rtk_bt_vendor_config_entry {
	RT_U16 offset;
	RT_U8 entry_len;
	RT_U8 entry_data[0];
} __attribute__ ((packed));

struct rtk_bt_vendor_config {
	RT_U32 signature;
	RT_U16 data_len;
	struct rtk_bt_vendor_config_entry entry[0];
} __attribute__ ((packed));

struct rtk_epatch_entry {
	RT_U16 chipID;
	RT_U16 patch_length;
	RT_U32 start_offset;
	RT_U32 svn_ver;
	RT_U32 coex_ver;
} __attribute__ ((packed));

struct rtk_epatch {
	RT_U8 signature[8];
	RT_U32 fw_version;
	RT_U16 number_of_patch;
	struct rtk_epatch_entry entry[0];
} __attribute__ ((packed));

struct rtk_extension_entry {
	uint8_t opcode;
	uint8_t length;
	uint8_t *data;
} __attribute__ ((packed));

typedef enum _RTK_ROM_VERSION_CMD_STATE {
	cmd_not_send,
	cmd_has_sent,
	event_received
} RTK_ROM_VERSION_CMD_STATE;

typedef enum _H5_RX_STATE {
	H5_W4_PKT_DELIMITER,
	H5_W4_PKT_START,
	H5_W4_HDR,
	H5_W4_DATA,
	H5_W4_CRC
} H5_RX_STATE;

typedef enum _H5_RX_ESC_STATE {
	H5_ESCSTATE_NOESC,
	H5_ESCSTATE_ESC
} H5_RX_ESC_STATE;

typedef enum _H5_LINK_STATE {
	H5_SYNC,
	H5_CONFIG,
	H5_INIT,
	H5_PATCH,
	H5_HCI_RESET,
	H5_ACTIVE
} H5_LINK_STATE;

uint16_t project_id[]=
{
	ROM_LMP_8723a,
	ROM_LMP_8723b, /* RTL8723BS */
	ROM_LMP_8821a, /* RTL8821AS */
	ROM_LMP_8761a, /* RTL8761ATV */

	ROM_LMP_8703a,
	ROM_LMP_8763a,
	ROM_LMP_8703b,
	ROM_LMP_8723c, /* index 7 for 8723CS. What is for other 8723CS  */
	ROM_LMP_8822b, /* RTL8822BS */
	ROM_LMP_8723b, /* RTL8723DS */
	ROM_LMP_8821a, /* id 10 for RTL8821CS, lmp subver 0x8821 */
	ROM_LMP_NONE,
	ROM_LMP_NONE,
	ROM_LMP_8822c  /* id 13 for RTL8822CS, lmp subver 0x8822 */
};

#define RTL_FW_MATCH_CHIP_TYPE  (1 << 0)
#define RTL_FW_MATCH_HCI_VER    (1 << 1)
#define RTL_FW_MATCH_HCI_REV    (1 << 2)
struct patch_info {
	uint32_t    match_flags;
	uint8_t     chip_type;
	uint16_t    lmp_subver;
	uint16_t    proj_id;
	uint8_t     hci_ver;
	uint16_t    hci_rev;
	char        *patch_file;
	char        *config_file;
	char        *ic_name;
};

static struct patch_info h4_patch_table[] = {
	/* match flags, chip type, lmp subver, proj id(unused), hci_ver,
	 * hci_rev, ...
	 */

	/* RTL8761AT */
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8761AT,
		0x8761, 0xffff, 0, 0x000a,
		"rtl8761at_fw", "rtl8761at_config", "RTL8761AT" },
	/* RTL8761ATF */
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8761ATF,
		0x8761, 0xffff, 0, 0x000a,
		"rtl8761atf_fw", "rtl8761atf_config", "RTL8761ATF" },
	/* RTL8761B TC
	 * FW/Config is not used.
	 */
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8761BTC,
		0x8763, 0xffff, 0, 0x000b,
		"rtl8761btc_fw", "rtl8761btc_config", "RTL8761BTC" },
	/* RTL8761B */
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8761B,
		0x8761, 0xffff, 0, 0x000b,
		"rtl8761b_fw", "rtl8761b_config", "RTL8761B" },

	/* RTL8723DS */
	{ RTL_FW_MATCH_HCI_VER | RTL_FW_MATCH_HCI_REV, CHIP_8723DS,
		ROM_LMP_8723b, ROM_LMP_8723b, 8, 0x000d,
		"rtl8723dsh4_fw", "rtl8723dsh4_config", "RTL8723DSH4"},

	{ 0, 0, 0, ROM_LMP_NONE, 0, 0, "rtl_none_fw", "rtl_none_config", "NONE"}
};

static struct patch_info patch_table[] = {
	/* match flags, chip type, lmp subver, proj id(unused), hci_ver,
	 * hci_rev, ...
	 */

	/* RTL8723AS */
	{ 0, 0, ROM_LMP_8723a, ROM_LMP_8723a, 0, 0,
		"rtl8723a_fw", "rtl8723a_config", "RTL8723AS"},
	/* RTL8821CS */
	{ RTL_FW_MATCH_HCI_REV, CHIP_8821CS,
		ROM_LMP_8821a, ROM_LMP_8821a, 0, 0x000c,
		"rtl8821c_fw", "rtl8821c_config", "RTL8821CS"},
	/* RTL8821AS */
	{ 0, 0, ROM_LMP_8821a, ROM_LMP_8821a, 0, 0,
		"rtl8821a_fw", "rtl8821a_config", "RTL8821AS"},
	/* RTL8761ATV */
	{ 0, 0, ROM_LMP_8761a, ROM_LMP_8761a, 0, 0,
		"rtl8761a_fw", "rtl8761a_config", "RTL8761ATV"},

	/* RTL8703AS
	 * RTL8822BS
	 * */
#ifdef RTL_8703A_SUPPORT
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8703AS,
		ROM_LMP_8723b, ROM_LMP_8723b, 0, 0,
		"rtl8703a_fw", "rtl8703a_config", "RTL8703AS"},
#endif
	{ RTL_FW_MATCH_HCI_REV, CHIP_8822BS,
		ROM_LMP_8822b, ROM_LMP_8822b, 0, 0x000b,
		"rtl8822b_fw", "rtl8822b_config", "RTL8822BS"},
	{ RTL_FW_MATCH_HCI_REV, CHIP_8822CS,
		ROM_LMP_8822c, ROM_LMP_8822c, 0, 0x000c,
		"rtl8822cs_fw", "rtl8822cs_config", "RTL8822CS"},

	/* RTL8703BS
	 * RTL8723CS_XX
	 * RTL8723CS_CG
	 * RTL8723CS_VF
	 * Use the sampe lmp subversion 0x8703
	 * */
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8703BS,
		ROM_LMP_8703b, ROM_LMP_8703b, 0, 0,
		"rtl8703b_fw", "rtl8703b_config", "RTL8703BS"},
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8723CS_XX,
		ROM_LMP_8703b, ROM_LMP_8723cs_xx, 0, 0,
		"rtl8723cs_fw", "rtl8723cs_config", "RTL8723CS_XX"},
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8723CS_CG,
		ROM_LMP_8703b, ROM_LMP_8723cs_cg, 0, 0,
		"rtl8723cs_fw", "rtl8723cs_config", "RTL8723CS_CG"},
	{ RTL_FW_MATCH_CHIP_TYPE, CHIP_8723CS_VF,
		ROM_LMP_8703b, ROM_LMP_8723cs_vf, 0, 0,
		"rtl8723cs_fw", "rtl8723cs_config", "RTL8723CS_VF"},

	/* RTL8723BS */
	{ RTL_FW_MATCH_HCI_VER | RTL_FW_MATCH_HCI_REV, 0,
		ROM_LMP_8723b, ROM_LMP_8723b, 6, 0x000b,
		"rtl8723b_fw", "rtl8723b_config", "RTL8723BS"},
	/* RTL8723DS */
	{ RTL_FW_MATCH_HCI_VER | RTL_FW_MATCH_HCI_REV, CHIP_8723DS,
		ROM_LMP_8723b, ROM_LMP_8723b, 8, 0x000d,
		"rtl8723d_fw", "rtl8723d_config", "RTL8723DS"},
	/* add entries here*/

	{ 0, 0, 0, ROM_LMP_NONE, 0, 0, "rtl_none_fw", "rtl_none_config", "NONE"}
};

typedef struct btrtl_info {
/**********************h5 releated*************************/
	RT_U8 rxseq_txack;	/* expected rx seq number */
	RT_U8 rxack;	/* last packet sent by us that the peer ack'ed */
	RT_U8 use_crc;
	RT_U8 is_txack_req;	/* txack required */
	RT_U8 msgq_txseq;	/* next pkt seq */
	RT_U16 message_crc;
	RT_U32 rx_count;	/* expected pkts to recv */

	H5_RX_STATE rx_state;
	H5_RX_ESC_STATE rx_esc_state;
	H5_LINK_STATE link_estab_state;

	struct sk_buff *rx_skb;
	struct sk_buff *host_last_cmd;

	uint16_t num_of_cmd_sent;
	RT_U16 lmp_subver;
	uint16_t hci_rev;
	uint8_t hci_ver;
	RT_U8 eversion;
	int h5_max_retries;
	RT_U8 chip_type;

/**********************patch related************************/
	uint32_t baudrate;
	uint8_t dl_fw_flag;
	int serial_fd;
	int hw_flow_control;
	uint32_t parity_en:   16;
	uint32_t parity_even: 16;
	int final_speed;
	int total_num;	/* total pkt number */
	int tx_index;	/* current sending pkt number */
	int rx_index;	/* ack index from board */
	int fw_len;	/* fw patch file len */
	int config_len;	/* config patch file len */
	int total_len;	/* fw & config extracted buf len */
	uint8_t *fw_buf;	/* fw patch file buf */
	uint8_t *config_buf;	/* config patch file buf */
	uint8_t *total_buf;	/* fw & config extracted buf */
	RTK_ROM_VERSION_CMD_STATE rom_version_cmd_state;
	RTK_ROM_VERSION_CMD_STATE hci_version_cmd_state;
	RTK_ROM_VERSION_CMD_STATE chip_type_cmd_state;
	int reset_cmd_state;

	struct patch_info *patch_ent;

	int proto;
} rtk_hw_cfg_t;

static rtk_hw_cfg_t rtk_hw_cfg;

void util_hexdump(const uint8_t *buf, size_t len)
{
	static const char hexdigits[] = "0123456789abcdef";
	char str[16 * 3];
	size_t i;

	if (!buf || !len)
		return;

	for (i = 0; i < len; i++) {
		str[((i % 16) * 3)] = hexdigits[buf[i] >> 4];
		str[((i % 16) * 3) + 1] = hexdigits[buf[i] & 0xf];
		str[((i % 16) * 3) + 2] = ' ';
		if ((i + 1) % 16 == 0) {
			str[16 * 3 - 1] = '\0';
			RS_INFO("%s", str);
		}
	}

	if (i % 16 > 0) {
		str[(i % 16) * 3 - 1] = '\0';
		RS_INFO("%s", str);
	}
}

#ifdef EXTRA_CONFIG_OPTION

static inline int xtalset_supported(void)
{
	struct patch_info *pent = rtk_hw_cfg.patch_ent;

	switch (pent->chip_type) {
	case CHIP_8822BS:
	case CHIP_8723DS:
	case CHIP_8821CS:
	case CHIP_8822CS:
		return 1;
	default:
		return 0;
	}
}

static void line_proc(char *buff, int len)
{
	char *argv[32];
	int nargs = 0;
	RS_INFO("%s", buff);

	while (nargs < 32) {
		/* skip any white space */
		while ((*buff == ' ') || (*buff == '\t') ||
			*buff == ',') {
			++buff;
		}

		if (*buff == '\0') {
			/* end of line, no more args */
			argv[nargs] = NULL;
			break;
		}

		/* begin of argument string */
		argv[nargs++] = buff;

		/* find end of string */
		while (*buff && (*buff != ' ') && (*buff != '\t')) {
			++buff;
		}

		if (*buff == '\r' || *buff == '\n')
			++buff;

		if (*buff == '\0') {
			/* end of line, no more args */
			argv[nargs] = NULL;
			break;
		}

		*buff++ = '\0';	/* terminate current arg */
	}

	/* valid config */
	if (nargs >= 4) {
		unsigned long int offset;
		uint8_t l;
		uint8_t i = 0;

		offset = strtoul(argv[0], NULL, 16);
		offset = offset | (strtoul(argv[1], NULL, 16) << 8);
		RS_INFO("extra config offset %04lx", offset);
		l = strtoul(argv[2], NULL, 16);
		if (l != (uint8_t)(nargs - 3)) {
			RS_ERR("invalid len %u", l);
			return;
		}

		if (offset == 0x015b && l <= 4) {
			/* Tx power */
			for (i = 0; i < l; i++)
				txpower_cfg[i] = (uint8_t)strtoul(argv[3 + i], NULL, 16);
			txpower_len = l;
			config_flags |= CONFIG_TXPOWER;
		} else if (offset == 0x01e6) {
			/* XTAL for 8822B, 8821C 8723D */
			xtal_cfg = (uint8_t)strtoul(argv[3], NULL, 16);
			config_flags |= CONFIG_XTAL;
		} else {
			RS_ERR("extra config %04lx is not supported", offset);
		}
	}
}

static void config_proc(uint8_t *buff, int len)
{
	uint8_t *head = buff;
	uint8_t *ptr = buff;
	int l;

	while (len > 0) {
		ptr = (uint8_t *)strchr((const char *)head, '\n');
		if (!ptr)
			break;
		*ptr++ = '\0';
		while (*head == ' ' || *head == '\t')
			head++;
		l = ptr - head;
		if (l > 0 && *head != '#')
			line_proc((char *)head, l);
		head = ptr;
		len -= l;
	}
}

static void config_file_proc(const char *path)
{
	int fd;
	uint8_t buff[256];
	int result;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		RS_INFO("Couldnt open extra config %s, %s", path, strerror(errno));
		return;
	}

	result = read(fd, buff, sizeof(buff));
	if (result == -1) {
		RS_ERR("Couldnt read %s, %s", path, strerror(errno));
		close(fd);
		return;
	} else if (result == 0) {
		RS_ERR("File is empty");
		close(fd);
		return;
	}
	buff[result] = '\n';

	config_proc(buff, result);

	close(fd);
}
#endif

/* Get the entry from patch_table according to LMP subversion */
struct patch_info *get_patch_entry(struct btrtl_info *btrtl)
{
	struct patch_info *n = NULL;

	if (btrtl->proto == HCI_UART_3WIRE)
		n = patch_table;
	else
		n = h4_patch_table;
	for (; n->lmp_subver; n++) {
		if ((n->match_flags & RTL_FW_MATCH_CHIP_TYPE) &&
		    n->chip_type != btrtl->chip_type)
			continue;
		if ((n->match_flags & RTL_FW_MATCH_HCI_VER) &&
		    n->hci_ver != btrtl->hci_ver)
			continue;
		if ((n->match_flags & RTL_FW_MATCH_HCI_REV) &&
		    n->hci_rev != btrtl->hci_rev)
			continue;
		if (n->lmp_subver != btrtl->lmp_subver)
			continue;

		break;
	}

	return n;
}

// bite reverse in bytes
// 00000001 -> 10000000
// 00000100 -> 00100000
const RT_U8 byte_rev_table[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

static __inline RT_U8 bit_rev8(RT_U8 byte)
{
	return byte_rev_table[byte];
}

static __inline RT_U16 bit_rev16(RT_U16 x)
{
	return (bit_rev8(x & 0xff) << 8) | bit_rev8(x >> 8);
}

static const RT_U16 crc_table[] = {
	0x0000, 0x1081, 0x2102, 0x3183,
	0x4204, 0x5285, 0x6306, 0x7387,
	0x8408, 0x9489, 0xa50a, 0xb58b,
	0xc60c, 0xd68d, 0xe70e, 0xf78f
};

// Initialise the crc calculator
#define H5_CRC_INIT(x) x = 0xffff

/**
* Malloc the socket buffer
*
* @param skb socket buffer
* @return the point to the malloc buffer
*/
static __inline struct sk_buff *skb_alloc(unsigned int len)
{
	struct sk_buff *skb = NULL;
	if ((skb = malloc(len + 8))) {
		skb->max_len = len;
		skb->data_len = 0;
	} else {
		RS_ERR("Allocate skb fails!!!");
		skb = NULL;
	}
	memset(skb->data, 0, len);
	return skb;
}

/**
* Free the socket buffer
*
* @param skb socket buffer
*/
static __inline void skb_free(struct sk_buff *skb)
{
	free(skb);
	return;
}

/**
* Increase the date length in sk_buffer by len,
* and return the increased header pointer
*
* @param skb socket buffer
* @param len length want to increase
* @return the pointer to increased header
*/
static RT_U8 *skb_put(struct sk_buff *skb, RT_U32 len)
{
	RT_U32 old_len = skb->data_len;
	if ((skb->data_len + len) > (skb->max_len)) {
		RS_ERR("Buffer too small");
		return NULL;
	}
	skb->data_len += len;
	return (skb->data + old_len);
}

/**
* decrease data length in sk_buffer by to len by cut the tail
*
* @warning len should be less than skb->len
*
* @param skb socket buffer
* @param len length want to be changed
*/
static void skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (skb->data_len > len) {
		skb->data_len = len;
	} else {
		RS_ERR("Error: skb->data_len(%d) < len(%d)", skb->data_len,
		       len);
	}
}

/**
* Decrease the data length in sk_buffer by len,
* and move the content forward to the header.
* the data in header will be removed.
*
* @param skb socket buffer
* @param len length of data
* @return new data
*/
static RT_U8 *skb_pull(struct sk_buff *skb, RT_U32 len)
{
	skb->data_len -= len;
	unsigned char *buf;
	if (!(buf = malloc(skb->data_len))) {
		RS_ERR("Unable to allocate file buffer");
		exit(1);
	}
	memcpy(buf, skb->data + len, skb->data_len);
	memcpy(skb->data, buf, skb->data_len);
	free(buf);
	return (skb->data);
}

/**
* Add "d" into crc scope, caculate the new crc value
*
* @param crc crc data
* @param d one byte data
*/
static void h5_crc_update(RT_U16 * crc, RT_U8 d)
{
	RT_U16 reg = *crc;

	reg = (reg >> 4) ^ crc_table[(reg ^ d) & 0x000f];
	reg = (reg >> 4) ^ crc_table[(reg ^ (d >> 4)) & 0x000f];

	*crc = reg;
}

struct __una_u16 {
	RT_U16 x;
};
static __inline RT_U16 __get_unaligned_cpu16(const void *p)
{
	const struct __una_u16 *ptr = (const struct __una_u16 *)p;
	return ptr->x;
}

static __inline RT_U16 get_unaligned_be16(const void *p)
{
	return __get_unaligned_cpu16((const RT_U8 *)p);
}

static __inline RT_U16 get_unaligned_le16(RT_U8 * p)
{
	return (RT_U16) (*p) + ((RT_U16) (*(p + 1)) << 8);
}

static __inline RT_U32 get_unaligned_le32(RT_U8 * p)
{
	return (RT_U32) (*p) + ((RT_U32) (*(p + 1)) << 8) +
	    ((RT_U32) (*(p + 2)) << 16) + ((RT_U32) (*(p + 3)) << 24);
}

/**
* Get crc data.
*
* @param h5 realtek h5 struct
* @return crc data
*/
static RT_U16 h5_get_crc(rtk_hw_cfg_t * h5)
{
	RT_U16 crc = 0;
	RT_U8 *data = h5->rx_skb->data + h5->rx_skb->data_len - 2;
	crc = data[1] + (data[0] << 8);
	return crc;
//    return get_unaligned_be16(&h5->rx_skb->data[h5->rx_skb->data_len - 2]);
}

/**
* Just add 0xc0 at the end of skb,
* we can also use this to add 0xc0 at start while there is no data in skb
*
* @param skb socket buffer
*/
static void h5_slip_msgdelim(struct sk_buff *skb)
{
	const char pkt_delim = 0xc0;
	memcpy(skb_put(skb, 1), &pkt_delim, 1);
}

/**
* Slip ecode one byte in h5 proto, as follows:
* 0xc0 -> 0xdb, 0xdc
* 0xdb -> 0xdb, 0xdd
* 0x11 -> 0xdb, 0xde
* 0x13 -> 0xdb, 0xdf
* others will not change
*
* @param skb socket buffer
* @c pure data in the one byte
*/
static void h5_slip_one_byte(struct sk_buff *skb, RT_U8 c)
{
	const RT_S8 esc_c0[2] = { 0xdb, 0xdc };
	const RT_S8 esc_db[2] = { 0xdb, 0xdd };
	const RT_S8 esc_11[2] = { 0xdb, 0xde };
	const RT_S8 esc_13[2] = { 0xdb, 0xdf };

	switch (c) {
	case 0xc0:
		memcpy(skb_put(skb, 2), &esc_c0, 2);
		break;

	case 0xdb:
		memcpy(skb_put(skb, 2), &esc_db, 2);
		break;

	case 0x11:
		memcpy(skb_put(skb, 2), &esc_11, 2);
		break;

	case 0x13:
		memcpy(skb_put(skb, 2), &esc_13, 2);
		break;

	default:
		memcpy(skb_put(skb, 1), &c, 1);
		break;
	}
}

/**
* Decode one byte in h5 proto, as follows:
* 0xdb, 0xdc -> 0xc0
* 0xdb, 0xdd -> 0xdb
* 0xdb, 0xde -> 0x11
* 0xdb, 0xdf -> 0x13
* others will not change
*
* @param h5 realtek h5 struct
* @byte pure data in the one byte
*/
static void h5_unslip_one_byte(rtk_hw_cfg_t * h5, unsigned char byte)
{
	const RT_U8 c0 = 0xc0, db = 0xdb;
	const RT_U8 oof1 = 0x11, oof2 = 0x13;

	if (H5_ESCSTATE_NOESC == h5->rx_esc_state) {
		if (0xdb == byte) {
			h5->rx_esc_state = H5_ESCSTATE_ESC;
		} else {
			memcpy(skb_put(h5->rx_skb, 1), &byte, 1);
			//Check Pkt Header's CRC enable bit
			if ((h5->rx_skb->data[0] & 0x40) != 0
			    && h5->rx_state != H5_W4_CRC) {
				h5_crc_update(&h5->message_crc, byte);
			}
			h5->rx_count--;
		}
	} else if (H5_ESCSTATE_ESC == h5->rx_esc_state) {
		switch (byte) {
		case 0xdc:
			memcpy(skb_put(h5->rx_skb, 1), &c0, 1);
			if ((h5->rx_skb->data[0] & 0x40) != 0
			    && h5->rx_state != H5_W4_CRC)
				h5_crc_update(&h5->message_crc, 0xc0);
			h5->rx_esc_state = H5_ESCSTATE_NOESC;
			h5->rx_count--;
			break;

		case 0xdd:
			memcpy(skb_put(h5->rx_skb, 1), &db, 1);
			if ((h5->rx_skb->data[0] & 0x40) != 0
			    && h5->rx_state != H5_W4_CRC)
				h5_crc_update(&h5->message_crc, 0xdb);
			h5->rx_esc_state = H5_ESCSTATE_NOESC;
			h5->rx_count--;
			break;

		case 0xde:
			memcpy(skb_put(h5->rx_skb, 1), &oof1, 1);
			if ((h5->rx_skb->data[0] & 0x40) != 0
			    && h5->rx_state != H5_W4_CRC)
				h5_crc_update(&h5->message_crc, oof1);
			h5->rx_esc_state = H5_ESCSTATE_NOESC;
			h5->rx_count--;
			break;

		case 0xdf:
			memcpy(skb_put(h5->rx_skb, 1), &oof2, 1);
			if ((h5->rx_skb->data[0] & 0x40) != 0
			    && h5->rx_state != H5_W4_CRC)
				h5_crc_update(&h5->message_crc, oof2);
			h5->rx_esc_state = H5_ESCSTATE_NOESC;
			h5->rx_count--;
			break;

		default:
			RS_ERR("Error: Invalid byte %02x after esc byte", byte);
			skb_free(h5->rx_skb);
			h5->rx_skb = NULL;
			h5->rx_state = H5_W4_PKT_DELIMITER;
			h5->rx_count = 0;
			break;
		}
	}
}

/**
* Prepare h5 packet, packet format as follow:
*  | LSB 4 octets  | 0 ~4095| 2 MSB
*  |packet header | payload | data integrity check |
*
* pakcket header fromat is show below:
*  | LSB 3 bits         | 3 bits				   | 1 bits   				     | 1 bits  		  |
*  | 4 bits 		   | 12 bits     | 8 bits MSB
*  |sequence number | acknowledgement number | data integrity check present | reliable packet |
*  |packet type | payload length | header checksum
*
* @param h5 realtek h5 struct
* @param data pure data
* @param len the length of data
* @param pkt_type packet type
* @return socket buff after prepare in h5 proto
*/
static struct sk_buff *h5_prepare_pkt(rtk_hw_cfg_t * h5, RT_U8 * data,
				      RT_S32 len, RT_S32 pkt_type)
{
	struct sk_buff *nskb;
	RT_U8 hdr[4];
	RT_U16 H5_CRC_INIT(h5_txmsg_crc);
	int rel, i;

	switch (pkt_type) {
	case HCI_ACLDATA_PKT:
	case HCI_COMMAND_PKT:
	case HCI_EVENT_PKT:
		rel = 1;	// reliable
		break;

	case H5_ACK_PKT:
	case H5_VDRSPEC_PKT:
	case H5_LINK_CTL_PKT:
		rel = 0;	// unreliable
		break;

	default:
		RS_ERR("Unknown packet type");
		return NULL;
	}

	// Max len of packet: (original len +4(h5 hdr) +2(crc))*2
	//   (because bytes 0xc0 and 0xdb are escaped, worst case is
	//   when the packet is all made of 0xc0 and 0xdb :) )
	//   + 2 (0xc0 delimiters at start and end).

	nskb = skb_alloc((len + 6) * 2 + 2);
	if (!nskb)
		return NULL;

	//Add SLIP start byte: 0xc0
	h5_slip_msgdelim(nskb);
	// set AckNumber in SlipHeader
	hdr[0] = h5->rxseq_txack << 3;
	h5->is_txack_req = 0;

	//RS_DBG("We request packet no(%u) to card", h5->rxseq_txack);
	//RS_DBG("Sending packet with seqno %u and wait %u", h5->msgq_txseq, h5->rxseq_txack);
	if (rel) {
		// set reliable pkt bit and SeqNumber
		hdr[0] |= 0x80 + h5->msgq_txseq;
		//RS_DBG("Sending packet with seqno(%u)", h5->msgq_txseq);
		++(h5->msgq_txseq);
		h5->msgq_txseq = (h5->msgq_txseq) & 0x07;
	}
	// set DicPresent bit
	if (h5->use_crc)
		hdr[0] |= 0x40;

	// set packet type and payload length
	hdr[1] = ((len << 4) & 0xff) | pkt_type;
	hdr[2] = (RT_U8) (len >> 4);
	// set checksum
	hdr[3] = ~(hdr[0] + hdr[1] + hdr[2]);

	// Put h5 header */
	for (i = 0; i < 4; i++) {
		h5_slip_one_byte(nskb, hdr[i]);

		if (h5->use_crc)
			h5_crc_update(&h5_txmsg_crc, hdr[i]);
	}

	// Put payload */
	for (i = 0; i < len; i++) {
		h5_slip_one_byte(nskb, data[i]);

		if (h5->use_crc)
			h5_crc_update(&h5_txmsg_crc, data[i]);
	}

	// Put CRC */
	if (h5->use_crc) {
		h5_txmsg_crc = bit_rev16(h5_txmsg_crc);
		h5_slip_one_byte(nskb, (RT_U8) ((h5_txmsg_crc >> 8) & 0x00ff));
		h5_slip_one_byte(nskb, (RT_U8) (h5_txmsg_crc & 0x00ff));
	}
	// Add SLIP end byte: 0xc0
	h5_slip_msgdelim(nskb);
	return nskb;
}

/**
* Removed controller acked packet from Host's unacked lists
*
* @param h5 realtek h5 struct
*/
static void h5_remove_acked_pkt(rtk_hw_cfg_t * h5)
{
	int pkts_to_be_removed = 0;
	int seqno = 0;
	int i = 0;

	seqno = h5->msgq_txseq;
	//pkts_to_be_removed = GetListLength(h5->unacked);

	while (pkts_to_be_removed) {
		if (h5->rxack == seqno)
			break;

		pkts_to_be_removed--;
		seqno = (seqno - 1) & 0x07;
	}

	if (h5->rxack != seqno) {
		RS_DBG("Peer acked invalid packet");
	}
	//skb_queue_walk_safe(&h5->unack, skb, tmp) // remove ack'ed packet from h5->unack queue
	for (i = 0; i < 5; ++i) {
		if (i >= pkts_to_be_removed)
			break;
		i++;
		//__skb_unlink(skb, &h5->unack);
		//skb_free(skb);
	}

	//  if (skb_queue_empty(&h5->unack))
	//          del_timer(&h5->th5);
	//  spin_unlock_irqrestore(&h5->unack.lock, flags);

	if (i != pkts_to_be_removed)
		RS_DBG("Removed only (%u) out of (%u) pkts", i,
		       pkts_to_be_removed);
}

/**
* Realtek send pure ack, send a packet only with an ack
*
* @param fd uart file descriptor
*
*/
static void rtk_send_pure_ack_down(int fd)
{
	struct sk_buff *nskb = h5_prepare_pkt(&rtk_hw_cfg, NULL, 0, H5_ACK_PKT);
	write(fd, nskb->data, nskb->data_len);
	skb_free(nskb);
	return;
}

/**
* Parse hci event command complete, pull the cmd complete event header
*
* @param skb socket buffer
*
*/
static void hci_event_cmd_complete(struct sk_buff *skb)
{
	struct hci_event_hdr *hdr = (struct hci_event_hdr *)skb->data;
	struct hci_ev_cmd_complete *ev = NULL;
	RT_U16 opcode = 0;
	RT_U8 status = 0;

	//pull hdr
	skb_pull(skb, HCI_EVENT_HDR_SIZE);
	ev = (struct hci_ev_cmd_complete *)skb->data;
	opcode = le16_to_cpu(ev->opcode);

	RS_DBG("receive hci command complete event with command:%x\n", opcode);

	//pull command complete event header
	skb_pull(skb, sizeof(struct hci_ev_cmd_complete));

	switch (opcode) {
	case HCI_VENDOR_CHANGE_BDRATE:
		status = skb->data[0];
		RS_DBG("Change BD Rate with status:%x", status);
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		rtk_hw_cfg.link_estab_state = H5_PATCH;
		break;

	case HCI_CMD_READ_BD_ADDR:
		status = skb->data[0];
		RS_DBG("Read BD Address with Status:%x", status);
		if (!status) {
			RS_DBG("BD Address: %8x%8x", *(int *)&skb->data[1],
			       *(int *)&skb->data[5]);
		}
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		break;

	case HCI_CMD_READ_LOCAL_VERISION:
		rtk_hw_cfg.hci_version_cmd_state = event_received;
		status = skb->data[0];
		RS_DBG("Read Local Version Information with Status:%x", status);
		if (0 == status) {
			rtk_hw_cfg.hci_ver = skb->data[1];
			rtk_hw_cfg.hci_rev = (skb->data[2] | skb->data[3] << 8);
			rtk_hw_cfg.lmp_subver =
			    (skb->data[7] | (skb->data[8] << 8));
			RS_DBG("HCI Version 0x%02x", rtk_hw_cfg.hci_ver);
			RS_DBG("HCI Revision 0x%04x", rtk_hw_cfg.hci_rev);
			RS_DBG("LMP Subversion 0x%04x", rtk_hw_cfg.lmp_subver);
		} else {
			RS_ERR("Read Local Version Info status error!");
			//Need to do more
		}
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		break;

	case HCI_VENDOR_READ_RTK_ROM_VERISION:
		rtk_hw_cfg.rom_version_cmd_state = event_received;
		status = skb->data[0];
		RS_DBG("Read RTK rom version with Status:%x", status);
		if (0 == status)
			rtk_hw_cfg.eversion = skb->data[1];
		else if (1 == status)
			rtk_hw_cfg.eversion = 0;
		else {
			RS_ERR("READ_RTK_ROM_VERISION return status error!");
			//Need to do more
		}

		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		break;
	case HCI_VENDOR_READ_CHIP_TYPE:
		rtk_hw_cfg.chip_type_cmd_state = event_received;
		status = skb->data[0];
		RS_DBG("Read RTK chip type with Status:%x", status);
		if (0 == status)
			rtk_hw_cfg.chip_type= (skb->data[1] & 0x0f);
		else
			RS_ERR("READ_RTK_CHIP_TYPE return status error!");
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		break;
	case 0x0c03:
		rtk_hw_cfg.reset_cmd_state = event_received;
		status = skb->data[0];
		RS_INFO("Event for hci reset cmd, status 0x%x", status);
		if (!status)
			rtk_hw_cfg.link_estab_state = H5_ACTIVE;
		else
			RS_ERR("hci reset error");
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
		break;
	default:
		return;
	}
	rtk_hw_cfg.num_of_cmd_sent++;
}

/**
* Check if it's a hci frame, if it is, complete it with response or parse the cmd complete event
*
* @param skb socket buffer
*
*/
static void hci_recv_frame(struct sk_buff *skb)
{
	int len;
	unsigned char h5sync[2] = { 0x01, 0x7E }, h5syncresp[2] = {
	0x02, 0x7D}, h5_sync_resp_pkt[0x8] = {
	0xc0, 0x00, 0x2F, 0x00, 0xD0, 0x02, 0x7D, 0xc0},
	    h5_conf_resp_pkt_to_Ctrl[0x8] = {
	0xc0, 0x00, 0x2F, 0x00, 0xD0, 0x04, 0x7B, 0xc0}, h5conf[3] = {
	0x03, 0xFC, 0x10}, h5confresp[3] = {
	0x04, 0x7B, 0x10}, cmd_complete_evt_code = 0xe;

	if (rtk_hw_cfg.link_estab_state == H5_SYNC) {
		if (!memcmp(skb->data, h5sync, 2)) {
			RS_DBG("Get SYNC Pkt\n");
			len =
			    write(rtk_hw_cfg.serial_fd, &h5_sync_resp_pkt, 0x8);
		} else if (!memcmp(skb->data, h5syncresp, 2)) {
			RS_DBG("Get SYNC Resp Pkt\n");
			rtk_hw_cfg.link_estab_state = H5_CONFIG;
		}
		skb_free(skb);
	} else if (rtk_hw_cfg.link_estab_state == H5_CONFIG) {
		if (!memcmp(skb->data, h5sync, 0x2)) {
			len =
			    write(rtk_hw_cfg.serial_fd, &h5_sync_resp_pkt, 0x8);
			RS_DBG("Get SYNC pkt-active mode\n");
		} else if (!memcmp(skb->data, h5conf, 0x2)) {
			len =
			    write(rtk_hw_cfg.serial_fd,
				  &h5_conf_resp_pkt_to_Ctrl, 0x8);
			RS_DBG("Get CONFG pkt-active mode\n");
		} else if (!memcmp(skb->data, h5confresp, 0x2)) {
			RS_DBG("Get CONFG resp pkt-active mode\n");
			rtk_hw_cfg.link_estab_state = H5_INIT;
		} else {
			RS_DBG("H5_CONFIG receive event\n");
			rtk_send_pure_ack_down(rtk_hw_cfg.serial_fd);
		}
		skb_free(skb);
	} else if (rtk_hw_cfg.link_estab_state == H5_INIT) {
		if (skb->data[0] == cmd_complete_evt_code) {
			hci_event_cmd_complete(skb);
		}

		rtk_send_pure_ack_down(rtk_hw_cfg.serial_fd);
		usleep(10000);
		rtk_send_pure_ack_down(rtk_hw_cfg.serial_fd);
		usleep(10000);
		rtk_send_pure_ack_down(rtk_hw_cfg.serial_fd);
		skb_free(skb);
	} else if (rtk_hw_cfg.link_estab_state == H5_PATCH) {
		if (skb->data[0] != 0x0e) {
			RS_DBG("Received event 0x%x\n", skb->data[0]);
			skb_free(skb);
			rtk_send_pure_ack_down(rtk_hw_cfg.serial_fd);
			return;
		}

		rtk_hw_cfg.rx_index = skb->data[6];

		RS_DBG("rtk_hw_cfg.rx_index %d\n", rtk_hw_cfg.rx_index);

		/* Download fw/config done */
		if (rtk_hw_cfg.rx_index & 0x80) {
			rtk_hw_cfg.rx_index &= ~0x80;
			rtk_hw_cfg.link_estab_state = H5_HCI_RESET;
		}

		skb_free(skb);
	} else if (rtk_hw_cfg.link_estab_state == H5_HCI_RESET) {
		if (skb->data[0] == 0x0e)
			hci_event_cmd_complete(skb);
	} else {
		RS_ERR("receive packets in active state");
		skb_free(skb);
	}
}

/**
* after rx data is parsed, and we got a rx frame saved in h5->rx_skb,
* this routinue is called.
* things todo in this function:
* 1. check if it's a hci frame, if it is, complete it with response or ack
* 2. see the ack number, free acked frame in queue
* 3. reset h5->rx_state, set rx_skb to null.
*
* @param h5 realtek h5 struct
*
*/
static void h5_complete_rx_pkt(rtk_hw_cfg_t * h5)
{
	int pass_up = 1;
	uint8_t *h5_hdr = NULL;

	h5_hdr = (uint8_t *) (h5->rx_skb->data);
	if (H5_HDR_RELIABLE(h5_hdr)) {
		RS_DBG("Received reliable seqno %u from card", h5->rxseq_txack);
		h5->rxseq_txack = H5_HDR_SEQ(h5_hdr) + 1;
		h5->rxseq_txack %= 8;
		h5->is_txack_req = 1;
	}

	h5->rxack = H5_HDR_ACK(h5_hdr);

	switch (H5_HDR_PKT_TYPE(h5_hdr)) {
	case HCI_ACLDATA_PKT:
	case HCI_EVENT_PKT:
	case HCI_SCODATA_PKT:
	case HCI_COMMAND_PKT:
	case H5_LINK_CTL_PKT:
		pass_up = 1;
		break;

	default:
		pass_up = 0;
		break;
	}

	h5_remove_acked_pkt(h5);

	if (pass_up) {
		skb_pull(h5->rx_skb, H5_HDR_SIZE);
		hci_recv_frame(h5->rx_skb);
	} else {
		skb_free(h5->rx_skb);
	}

	h5->rx_state = H5_W4_PKT_DELIMITER;
	h5->rx_skb = NULL;
}

/**
* Parse the receive data in h5 proto.
*
* @param h5 realtek h5 struct
* @param data point to data received before parse
* @param count num of data
* @return reserved count
*/
static int h5_recv(rtk_hw_cfg_t * h5, void *data, int count)
{
	unsigned char *ptr;
	//RS_DBG("count %d rx_state %d rx_count %ld", count, h5->rx_state, h5->rx_count);
	ptr = (unsigned char *)data;

	while (count) {
		if (h5->rx_count) {
			if (*ptr == 0xc0) {
				RS_ERR("short h5 packet");
				skb_free(h5->rx_skb);
				h5->rx_state = H5_W4_PKT_START;
				h5->rx_count = 0;
			} else
				h5_unslip_one_byte(h5, *ptr);

			ptr++;
			count--;
			continue;
		}

		switch (h5->rx_state) {
		case H5_W4_HDR:
			/* check header checksum. see Core Spec V4 "3-wire uart" page 67 */
			if ((0xff & (RT_U8) ~
			     (h5->rx_skb->data[0] + h5->rx_skb->data[1] +
			      h5->rx_skb->data[2])) != h5->rx_skb->data[3]) {
				RS_ERR("h5 hdr checksum error!!!");
				skb_free(h5->rx_skb);
				h5->rx_state = H5_W4_PKT_DELIMITER;
				h5->rx_count = 0;
				continue;
			}

			/* reliable pkt  & h5->hdr->SeqNumber != h5->Rxseq_txack */
			if (h5->rx_skb->data[0] & 0x80
			    && (h5->rx_skb->data[0] & 0x07) !=
			    h5->rxseq_txack) {
				RS_ERR
				    ("Out-of-order packet arrived, got(%u)expected(%u)",
				     h5->rx_skb->data[0] & 0x07,
				     h5->rxseq_txack);
				h5->is_txack_req = 1;

				skb_free(h5->rx_skb);
				h5->rx_state = H5_W4_PKT_DELIMITER;
				h5->rx_count = 0;

				/* depend on weather remote will reset ack numb or not!!!!!!special */
				if (rtk_hw_cfg.tx_index == rtk_hw_cfg.total_num) {
					rtk_hw_cfg.rxseq_txack =
					    h5->rx_skb->data[0] & 0x07;
				}
				continue;
			}
			h5->rx_state = H5_W4_DATA;
			h5->rx_count =
			    (h5->rx_skb->data[1] >> 4) +
			    (h5->rx_skb->data[2] << 4);
			continue;

		case H5_W4_DATA:
			/* pkt with crc */
			if (h5->rx_skb->data[0] & 0x40) {
				h5->rx_state = H5_W4_CRC;
				h5->rx_count = 2;
			} else {
				h5_complete_rx_pkt(h5);
				//RS_DBG(DF_SLIP,("--------> H5_W4_DATA ACK\n"));
			}
			continue;

		case H5_W4_CRC:
			if (bit_rev16(h5->message_crc) != h5_get_crc(h5)) {
				RS_ERR
				    ("Checksum failed, computed(%04x)received(%04x)",
				     bit_rev16(h5->message_crc),
				     h5_get_crc(h5));
				skb_free(h5->rx_skb);
				h5->rx_state = H5_W4_PKT_DELIMITER;
				h5->rx_count = 0;
				continue;
			}
			skb_trim(h5->rx_skb, h5->rx_skb->data_len - 2);
			h5_complete_rx_pkt(h5);
			continue;

		case H5_W4_PKT_DELIMITER:
			switch (*ptr) {
			case 0xc0:
				h5->rx_state = H5_W4_PKT_START;
				break;

			default:
				break;
			}
			ptr++;
			count--;
			break;

		case H5_W4_PKT_START:
			switch (*ptr) {
			case 0xc0:
				ptr++;
				count--;
				break;

			default:
				h5->rx_state = H5_W4_HDR;
				h5->rx_count = 4;
				h5->rx_esc_state = H5_ESCSTATE_NOESC;
				H5_CRC_INIT(h5->message_crc);

				// Do not increment ptr or decrement count
				// Allocate packet. Max len of a H5 pkt=
				// 0xFFF (payload) +4 (header) +2 (crc)
				h5->rx_skb = skb_alloc(0x1005);
				if (!h5->rx_skb) {
					h5->rx_state = H5_W4_PKT_DELIMITER;
					h5->rx_count = 0;
					return 0;
				}
				break;
			}
			break;

		default:
			break;
		}
	}
	return count;
}

/**
* Read data to buf from uart.
*
* @param fd uart file descriptor
* @param buf point to the addr where read data stored
* @param count num of data want to read
* @return num of data successfully read
*/
static int read_check_rtk(int fd, void *buf, int count)
{
	int res;
	do {
		res = read(fd, buf, count);
		if (res != -1) {
			buf = (RT_U8 *) buf + res;
			count -= res;
			return res;
		}
	} while (count && (errno == 0 || errno == EINTR));
	return res;
}

/**
* Retry to sync when timeout in h5 proto, max retry times is 10.
*
* @warning Each time to retry, the time for timeout will be set as 1s.
*
* @param sig signaction for timeout
*
*/
static void h5_tsync_sig_alarm(int sig)
{
	unsigned char h5sync[2] = { 0x01, 0x7E };
	static int retries = 0;
	struct itimerval value;

	if (retries < rtk_hw_cfg.h5_max_retries) {
		retries++;
		struct sk_buff *nskb =
		    h5_prepare_pkt(&rtk_hw_cfg, h5sync, sizeof(h5sync),
				   H5_LINK_CTL_PKT);
		int len =
		    write(rtk_hw_cfg.serial_fd, nskb->data, nskb->data_len);
		RS_DBG("3-wire sync pattern resend : %d, len: %d\n", retries,
		       len);

		skb_free(nskb);
		//gordon add 2013-6-7 retry per 250ms
		value.it_value.tv_sec = 0;
		value.it_value.tv_usec = 250000;
		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = 250000;
		setitimer(ITIMER_REAL, &value, NULL);
		//gordon end

		return;
	}

	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("H5 sync timed out\n");
	exit(1);
}

/**
* Retry to config when timeout in h5 proto, max retry times is 10.
*
* @warning Each time to retry, the time for timeout will be set as 1s.
*
* @param sig signaction for timeout
*
*/
static void h5_tconf_sig_alarm(int sig)
{
	unsigned char h5conf[3] = { 0x03, 0xFC, 0x14 };
	static int retries = 0;
	struct itimerval value;

	if (retries < rtk_hw_cfg.h5_max_retries) {
		retries++;
		struct sk_buff *nskb =
		    h5_prepare_pkt(&rtk_hw_cfg, h5conf, 3, H5_LINK_CTL_PKT);
		int len =
		    write(rtk_hw_cfg.serial_fd, nskb->data, nskb->data_len);
		RS_DBG("3-wire config pattern resend : %d , len: %d", retries,
		       len);
		skb_free(nskb);

		//gordon add 2013-6-7 retry per 250ms
		value.it_value.tv_sec = 0;
		value.it_value.tv_usec = 250000;
		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = 250000;
		setitimer(ITIMER_REAL, &value, NULL);

		return;
	}

	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("H5 config timed out\n");
	exit(1);
}

/**
* Retry to init when timeout in h5 proto, max retry times is 10.
*
* @warning Each time to retry, the time for timeout will be set as 1s.
*
* @param sig signaction for timeout
*
*/
static void h5_tinit_sig_alarm(int sig)
{
	static int retries = 0;
	if (retries < rtk_hw_cfg.h5_max_retries) {
		retries++;
		if (rtk_hw_cfg.host_last_cmd) {
			int len =
			    write(rtk_hw_cfg.serial_fd,
				  rtk_hw_cfg.host_last_cmd->data,
				  rtk_hw_cfg.host_last_cmd->data_len);
			RS_DBG("3-wire change baudrate re send:%d, len:%d",
			       retries, len);
			alarm(1);
			return;
		} else {
			RS_DBG
			    ("3-wire init timeout without last command stored\n");
		}
	}

	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("H5 init process timed out");
	exit(1);
}

/**
* Retry to download patch when timeout in h5 proto, max retry times is 10.
*
* @warning Each time to retry, the time for timeout will be set as 3s.
*
* @param sig signaction for timeout
*
*/
static void h5_tpatch_sig_alarm(int sig)
{
	static int retries = 0;
	if (retries < rtk_hw_cfg.h5_max_retries) {
		RS_ERR("patch timerout, retry:\n");
		if (rtk_hw_cfg.host_last_cmd) {
			int len =
			    write(rtk_hw_cfg.serial_fd,
				  rtk_hw_cfg.host_last_cmd->data,
				  rtk_hw_cfg.host_last_cmd->data_len);
			RS_DBG("3-wire download patch re send:%d", retries);
		}
		retries++;
		alarm(3);
		return;
	}
	RS_ERR("H5 patch timed out\n");
	exit(1);
}

/**
* Download patch using hci. For h5 proto, not recv reply for 2s will timeout.
* Call h5_tpatch_sig_alarm for retry.
*
* @param dd uart file descriptor
* @param index current index
* @param data point to the config file
* @param len current buf length
* @return #0 on success
*
*/
static int hci_download_patch(int dd, int index, uint8_t * data, int len,
			      struct termios *ti)
{
	unsigned char hcipatch[256] = { 0x20, 0xfc, 00 };
	unsigned char bytes[READ_DATA_SIZE];
	int retlen;
	struct sigaction sa;

	sa.sa_handler = h5_tpatch_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);
	alarm(2);

	download_vendor_patch_cp cp;
	memset(&cp, 0, sizeof(cp));
	cp.index = index;
	if (data != NULL) {
		memcpy(cp.data, data, len);
	}

	if (index & 0x80)
		rtk_hw_cfg.tx_index = index & 0x7f;
	else
		rtk_hw_cfg.tx_index = index;

	hcipatch[2] = len + 1;
	memcpy(hcipatch + 3, &cp, len + 1);

	struct sk_buff *nskb = h5_prepare_pkt(&rtk_hw_cfg, hcipatch, len + 4, HCI_COMMAND_PKT);	//data:len+head:4

	if (rtk_hw_cfg.host_last_cmd) {
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
	}

	rtk_hw_cfg.host_last_cmd = nskb;

	len = write(dd, nskb->data, nskb->data_len);
	RS_DBG("hci_download_patch tx_index:%d rx_index: %d\n",
	       rtk_hw_cfg.tx_index, rtk_hw_cfg.rx_index);

	while (rtk_hw_cfg.rx_index != rtk_hw_cfg.tx_index) {	//receive data and wait last pkt
		if ((retlen = read_check_rtk(dd, &bytes, READ_DATA_SIZE)) == -1) {
			RS_ERR("read fail\n");
			return -1;
		}
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}

	alarm(0);

	return 0;
}

#define READ_TRY_MAX	6
int os_read(int fd, uint8_t * buff, int len)
{
	int n;
	int i;
	int try = 0;

	i = 0;
	n = 0;
	while (n < len) {
		i = read(fd, buff + n, len - n);
		if (i > 0)
			n += i;
		else if (i == 0) {
			RS_DBG("read nothing.");
			continue;
		} else {
			RS_ERR("read error, %s\n", strerror(errno));
			try++;
			if (try > READ_TRY_MAX) {
				RS_ERR("read reaches max try number.\n");
				return -1;
			}
			continue;
		}
	}

	return n;
}

#define DUMP_HCI_EVT
#ifdef DUMP_HCI_EVT
#define HCI_DUMP_BUF_LEN 128
static char hci_dump_buf[HCI_DUMP_BUF_LEN];
void hci_dump_evt(uint8_t * buf, uint16_t len)
{
	int n;
	int i;

	if (!buf || !len) {
		RS_ERR("Invalid parameters %p, %u.\n", buf, len);
		return;
	}

	n = 0;
	for (i = 0; i < len; i++) {
		n += sprintf(hci_dump_buf + n, "%02x ", buf[i]);
		if ((i + 1) % 16 == 0) {
			RS_DBG(" %s\n", hci_dump_buf);
			n = 0;
		}
	}

	if (i % 16)
		RS_DBG(" %s\n", hci_dump_buf);
}
#endif

int read_hci_evt(int fd, uint8_t * buff, uint8_t evt_code)
{
	uint8_t *evt_buff = buff;
	int ret;
	int try_type = 0;
	uint8_t vendor_evt = 0xff;

start_read:
	do {
		ret = os_read(fd, evt_buff, 1);
		if (ret == 1 && evt_buff[0] == 0x04)
			break;
		else {
			RS_DBG("no pkt type, continue.");
			try_type++;
			continue;
		}
	} while (try_type < 6);

	if (try_type >= 6)
		return -1;

	ret = os_read(fd, evt_buff + 1, 1);
	if (ret < 0) {
		RS_ERR("%s: failed to read event code\n", __func__);
		return -1;
	}

	ret = os_read(fd, evt_buff + 2, 1);
	if (ret < 0) {
		RS_ERR("%s: failed to read parameter total len.\n", __func__);
		return -1;
	}

	ret = os_read(fd, evt_buff + 3, evt_buff[2]);
	if (ret < 0) {
		RS_ERR("%s: failed to read payload of event.\n", __func__);
		return -1;
	}
#ifdef DUMP_HCI_EVT
	hci_dump_evt(evt_buff, ret + 3);
#endif

	/* This event to wake up host. */
	if (evt_buff[1] == vendor_evt) {
		try_type = 0;
		RS_DBG("%s: found vendor evt, continue reading.\n", __func__);
		goto start_read;
	}

	if (evt_buff[1] != evt_code) {
		RS_ERR("%s: event code mismatches, %x, expect %x.\n",
		       __func__, evt_buff[1], evt_code);
		return -1;
	}

	return (ret + 3);
}

/**
* Download h4 patch
*
* @param dd uart file descriptor
* @param index current index
* @param data point to the config file
* @param len current buf length
* @return ret_index
*
*/
static int hci_download_patch_h4(int dd, int index, uint8_t * data, int len)
{
	unsigned char bytes[257] = { 0 };
	unsigned char buf[257] = { 0x01, 0x20, 0xfc, 00 };
	uint16_t readbytes = 0;
	int cur_index = index;
	int ret_Index = -1;
	uint16_t res = 0;
	int i = 0;
	size_t total_len;
	uint16_t w_len;
	uint8_t rstatus;
	int ret;
	uint8_t opcode[2] = {
		0x20, 0xfc,
	};

	RS_DBG("dd:%d, index:%d, len:%d", dd, index, len);
	if (NULL != data) {
		memcpy(&buf[5], data, len);
	}

	buf[3] = len + 1;
	buf[4] = cur_index;
	total_len = len + 5;

	w_len = write(dd, buf, total_len);
	RS_DBG("h4 write success with len: %d\n", w_len);

	ret = read_hci_evt(dd, bytes, 0x0e);
	if (ret < 0) {
		RS_ERR("%s: read hci evt error.\n", __func__);
		return -1;
	}

	/* RS_DBG("%s: bytes: %x %x %x %x %x %x.\n",
	 *      __func__, bytes[0], bytes[1], bytes[2],
	 *      bytes[3], bytes[4], bytes[5]); */

	if ((0x04 == bytes[0]) && (opcode[0] == bytes[4])
	    && (opcode[1] == bytes[5])) {
		ret_Index = bytes[7];
		rstatus = bytes[6];
		RS_DBG("---->ret_Index:%d, ----->rstatus:%d\n", ret_Index,
		       rstatus);
		if (0x00 != rstatus) {
			RS_ERR("---->read event status is wrong\n");
			return -1;
		}
	} else {
		RS_ERR("==========>Didn't read curret data\n");
		return -1;
	}

	return ret_Index;
}

/**
* Realtek change speed with h4 proto. Using vendor specified command packet to achieve this.
*
* @warning before write, need to wait 1s for device up
*
* @param fd uart file descriptor
* @param baudrate the speed want to change
* @return #0 on success
*/
static int rtk_vendor_change_speed_h4(int fd, RT_U32 baudrate)
{
	int res;
	unsigned char bytes[257];
	RT_U8 cmd[8] = { 0 };

	cmd[0] = 1;	//cmd;
	cmd[1] = 0x17;	//ocf
	cmd[2] = 0xfc;	//ogf
	cmd[3] = 4;	//length;

	baudrate = cpu_to_le32(baudrate);
#ifdef BAUDRATE_4BYTES
	memcpy((RT_U16 *) & cmd[4], &baudrate, 4);
#else
	memcpy((RT_U16 *) & cmd[4], &baudrate, 2);
	cmd[6] = 0;
	cmd[7] = 0;
#endif

	//wait for a while for device to up, just h4 need it
	sleep(1);
	RS_DBG("baudrate in change speed command: 0x%x 0x%x 0x%x 0x%x \n",
	       cmd[4], cmd[5], cmd[6], cmd[7]);

	if (write(fd, cmd, 8) != 8) {
		RS_ERR
		    ("H4 change uart speed error when writing vendor command");
		return -1;
	}
	RS_DBG("H4 Change uart Baudrate after write ");

	res = read_hci_evt(fd, bytes, 0x0e);
	if (res < 0) {
		RS_ERR("%s: Failed to read hci evt.\n", __func__);
		return -1;
	}

	if ((0x04 == bytes[0]) && (0x17 == bytes[4]) && (0xfc == bytes[5])) {
		RS_DBG("H4 change uart speed success, receving status:%x",
		       bytes[6]);
		if (bytes[6] == 0)
			return 0;
	}
	return -1;
}

static int is_mac(uint8_t chip_type, uint16_t offset)
{
	int result = 0;

	switch (chip_type) {
	case CHIP_8822BS:
	case CHIP_8723DS:
	case CHIP_8821CS:
	case CHIP_8723CS_XX:
	case CHIP_8723CS_CG:
	case CHIP_8723CS_VF:
		if (offset == 0x0044)
			return 1;
		break;
	case CHIP_8822CS:
		if (offset == 0x0030)
			return 1;
		break;
	case 0: /* special for not setting chip_type */
	case CHIP_8761AT:
	case CHIP_8761ATF:
	case CHIP_8761BTC:
	case CHIP_8761B:
	case CHIP_8723BS:
		if (offset == 0x003c)
			return 1;
		break;
	default:
		break;
	}

	return result;
}

static void fill_mac_offset(uint8_t chip_type, uint8_t b[2])
{
	switch (chip_type) {
	case CHIP_8822BS:
	case CHIP_8723DS:
	case CHIP_8821CS:
		b[0] = 0x44;
		b[1] = 0x00;
		break;
	case CHIP_8822CS:
		b[0] = 0x30;
		b[1] = 0x00;
		break;
	case 0: /* special for not setting chip_type */
	case CHIP_8761AT:
	case CHIP_8761ATF:
	case CHIP_8761BTC:
	case CHIP_8761B:
	case CHIP_8723BS:
		b[0] = 0x3c;
		b[1] = 0x00;
		break;
	}
}

/**
* Parse realtek Bluetooth config file.
* The config file if begin with vendor magic: RTK_VENDOR_CONFIG_MAGIC(8723ab55)
* bt_addr is followed by 0x3c offset, it will be changed by bt_addr param
* proto, baudrate and flow control is followed by 0xc offset,
*
* @param config_buf point to config file content
* @param *plen length of config file
* @param bt_addr where bt addr is stored
* @return
*  1. new config buf
*  2. new config length *plen
*  3. result (for baudrate)
*
*/

uint8_t *rtk_parse_config_file(RT_U8 *config_buf, size_t *plen,
			       uint8_t bt_addr[6], uint32_t *result)
{

	struct rtk_bt_vendor_config *config =
	    (struct rtk_bt_vendor_config *)config_buf;
	RT_U16 config_len = 0, temp = 0;
	struct rtk_bt_vendor_config_entry *entry = NULL;
	RT_U16 i;
	RT_U32 baudrate = 0;
	uint32_t flags = 0;
	uint32_t add_flags;
#ifdef USE_CUSTOMER_ADDRESS
	uint8_t j = 0;
#endif
	struct patch_info *pent = rtk_hw_cfg.patch_ent;
#ifdef EXTRA_CONFIG_OPTION
	uint8_t *head = config_buf;

	add_flags = config_flags;
#endif

	if (!config || !plen)
		return NULL;

	RS_INFO("Original Cfg len %u", (uint16_t)*plen);
	config_len = le16_to_cpu(config->data_len);
	entry = config->entry;

	if (le32_to_cpu(config->signature) != RTK_VENDOR_CONFIG_MAGIC) {
		RS_ERR("signature magic number(%x) is incorrect",
		     (unsigned int)config->signature);
		return NULL;
	}

	if (config_len != *plen - sizeof(struct rtk_bt_vendor_config)) {
		RS_ERR("config len(%d) is incorrect(%zd)", config_len,
		       *plen - sizeof(struct rtk_bt_vendor_config));
		return NULL;
	}

	for (i = 0; i < config_len;) {

		switch (le16_to_cpu(entry->offset)) {
#ifdef USE_CUSTOMER_ADDRESS
		case 0x003c:
		case 0x0044:
		case 0x0030:
			if (!customer_bdaddr)
				break;
			if (!is_mac(pent->chip_type, le16_to_cpu(entry->offset)))
				break;
			for (j = 0; j < entry->entry_len; j++)
				entry->entry_data[j] = bt_addr[j];
			flags |= CONFIG_BTMAC;
			RS_INFO("BT MAC found %02x:%02x:%02x:%02x:%02x:%02x",
				entry->entry_data[5],
				entry->entry_data[4],
				entry->entry_data[3],
				entry->entry_data[2],
				entry->entry_data[1],
				entry->entry_data[0]);
			break;
#endif
		case 0xc:
			{
#ifdef BAUDRATE_4BYTES
				baudrate =
				    get_unaligned_le32(entry->entry_data);
#else
				baudrate =
				    get_unaligned_le16(entry->entry_data);
#endif
				RS_INFO("Config baud : %08x", baudrate);

				/* offset 0x0c - 0x18 */
				if (entry->entry_len >= 12) {
					rtk_hw_cfg.hw_flow_control = (entry->entry_data[12] & 0x4) ? 1 : 0;	//0x18 byte bit2
					RS_INFO("hw flow control: %u",
						rtk_hw_cfg.hw_flow_control);
				}
				break;
			}
#ifdef EXTRA_CONFIG_OPTION
		case 0x015b:
			if (!(add_flags & CONFIG_TXPOWER))
				break;
			add_flags &= ~CONFIG_TXPOWER;
			if (txpower_len != entry->entry_len) {
				RS_ERR("invalid tx power cfg len %u, %u",
				       txpower_len, entry->entry_len);
				break;
			}
			memcpy(entry->entry_data, txpower_cfg, txpower_len);
			RS_INFO("Replace Tx power Cfg %02x %02x %02x %02x",
				txpower_cfg[0], txpower_cfg[1], txpower_cfg[2],
				txpower_cfg[3]);
			break;
		case 0x01e6:
			if (!(add_flags & CONFIG_XTAL))
				break;
			add_flags &= ~CONFIG_XTAL;
			RS_INFO("Replace XTAL Cfg 0x%02x", xtal_cfg);
			entry->entry_data[0] = xtal_cfg;
			break;
#endif
#ifdef RTL8723DSH4_UART_HWFLOWC
		case 0x0018:
			if (pent->chip_type == CHIP_8723DS &&
			    rtk_hw_cfg.proto == HCI_UART_H4) {
				if (entry->entry_data[0] & (1 << 2))
					rtk_hw_cfg.hw_flow_control = 1;
				RS_INFO("8723DSH4: hw flow control %u",
					rtk_hw_cfg.hw_flow_control);
				if (entry->entry_data[0] & 0x01) {
					rtk_hw_cfg.parity_en = 1;
					if (entry->entry_data[0] & 0x02)
						rtk_hw_cfg.parity_even = 1;
					else
						rtk_hw_cfg.parity_even = 0;
				}
				RS_INFO("8723DSH4: parity %u, even %u",
					rtk_hw_cfg.parity_en,
					rtk_hw_cfg.parity_even);
			}
			break;
#endif
		default:
			RS_DBG("cfg offset (%04x),length (%02x)", entry->offset,
			       entry->entry_len);
			break;
		}
		temp = entry->entry_len +
		       sizeof(struct rtk_bt_vendor_config_entry);
		i += temp;
		entry = (struct rtk_bt_vendor_config_entry *)((RT_U8 *)entry +
							      temp);
	}

#ifdef USE_CUSTOMER_ADDRESS
	if (!(flags & CONFIG_BTMAC) && customer_bdaddr) {
		uint8_t *b;

		b = config_buf + *plen;
		fill_mac_offset(pent->chip_type, b);

		RS_INFO("add bdaddr sec, offset %02x%02x", b[1], b[0]);
		b[2] = 6;
		for (j = 0; j < 6; j++)
			b[3 + j] = bt_addr[j];

		*plen += 9;
		config_len += 9;
		temp = *plen - 6;

		config_buf[4] = (temp & 0xff);
		config_buf[5] = ((temp >> 8) & 0xff);
	}
#endif

#ifdef EXTRA_CONFIG_OPTION
	temp = *plen;
	if (add_flags & CONFIG_TXPOWER)
		temp += (2 + 1 + txpower_len);
	if ((add_flags & CONFIG_XTAL))
		temp += (2 + 1 + 1);

	if (add_flags) {
		RS_INFO("Add extra configs");
		config_buf = head;
		temp -= sizeof(struct rtk_bt_vendor_config);
		config_buf[4] = (temp & 0xff);
		config_buf[5] = ((temp >> 8) & 0xff);
		config_buf += *plen;
		if (add_flags & CONFIG_TXPOWER) {
			RS_INFO("Add Tx power Cfg");
			*config_buf++ = 0x5b;
			*config_buf++ = 0x01;
			*config_buf++ = txpower_len;
			memcpy(config_buf, txpower_cfg, txpower_len);
			config_buf += txpower_len;
		}
		if ((add_flags & CONFIG_XTAL)) {
			RS_INFO("Add XTAL Cfg");
			*config_buf++ = 0xe6;
			*config_buf++ = 0x01;
			*config_buf++ = 1;
			*config_buf++ = xtal_cfg;
		}
		*plen = config_buf - head;
		config_buf = head;
	}
#endif

done:
	*result = baudrate;
	return config_buf;
}

#ifdef USE_CUSTOMER_ADDRESS
int bachk(const char *str)
{
	if (!str)
		return -1;

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isxdigit(*str++))
			return -1;

		if (!isxdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}
/**
* get random realtek Bluetooth addr.
*
* @param bt_addr where bt addr is stored
*
*/
/*
static void rtk_get_ram_addr(char bt_addr[0])
{
 	srand(time(NULL) + getpid() + getpid() * 987654 + rand());

    RT_U32 addr = rand();
    memcpy(bt_addr, &addr, sizeof(RT_U8));
    //RS_DBG("0x%X, 0x%2X", addr,  *bt_addr);
}
*/
/**
* Write the random bt addr to the file /data/misc/bluetoothd/bt_mac/btmac.txt.
*
* @param bt_addr where bt addr is stored
*
*/
static void rtk_write_btmac2file(char bt_addr[6])
{
    int fd;
    fd = open(BT_ADDR_FILE, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd > 0) {
 		chmod(BT_ADDR_FILE, 0666);
 		char addr[18] = { 0 };
 		addr[17] = '\0';
 		sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X", bt_addr[5], bt_addr[4],
 			bt_addr[3], bt_addr[2], bt_addr[1], bt_addr[0]);
 		write(fd, addr, strlen(addr));
 		close(fd);
 	} else {
 		RS_ERR("open file error:%s\n", BT_ADDR_FILE);
    }
}
#endif

/**
* Get realtek Bluetooth config file. The bt addr arg is stored in /data/btmac.txt, if there is not this file,
* change to /data/misc/bluetoothd/bt_mac/btmac.txt. If both of them are not found, using
* random bt addr.
*
* @param config_buf point to the content of realtek Bluetooth config file
* @param config_baud_rate the baudrate set in the config file
* @return file_len the length of config file
*/
int rtk_get_bt_config(struct btrtl_info *btrtl, uint8_t **config_buf,
		RT_U32 *config_baud_rate)
{
	char bt_config_file_name[PATH_MAX] = { 0 };
	RT_U8 *bt_addr_temp = NULL;
	uint8_t bt_addr[6] = { 0x00, 0xe0, 0x4c, 0x88, 0x88, 0x88 };
	struct stat st;
	size_t filelen;
	size_t tlength;
	int fd;
	int ret = 0;
	int i = 0;

#ifdef USE_CUSTOMER_ADDRESS
#define BDADDR_STRING_LEN	17
	size_t size;
	size_t result;
	uint8_t tbuf[BDADDR_STRING_LEN + 1] = {0};
	char *str;


    if (read_btmac(bt_addr) == 0) {//if have MAC  use custom set
        RS_INFO("read NV MAC addr success!!!");
        system(CMD_RM_BT_ADDR_FILE);
        rtk_write_btmac2file((char *)bt_addr);
        customer_bdaddr = 1;
    } else {
        RS_INFO("read NV MAC addr failed!!!");
	if (stat(BT_ADDR_FILE, &st) < 0) { //use module MAC
		RS_INFO("Couldnt access customer BT MAC file %s",
		        BT_ADDR_FILE);
            #if 0
		for (i = 0; i < 6; i++)
			rtk_get_ram_addr((char *)&bt_addr[i]);

		/*reserve LAP addr from 0x9e8b00 to 0x9e8b3f, change to 0x008b** */
		if (0x9e == bt_addr[3] && 0x8b == bt_addr[4]
		    && (bt_addr[5] <= 0x3f)) {
			bt_addr[3] = 0x00;
		}

		rtk_write_btmac2file((char *)bt_addr);
                customer_bdaddr = 1;
            #endif
		goto GET_CONFIG;
	}

	size = st.st_size;
	/* Only read the first 17-byte if the file length is larger */
	if (size > BDADDR_STRING_LEN)
		size = BDADDR_STRING_LEN;

	fd = open(BT_ADDR_FILE, O_RDONLY);
	if (fd == -1) {
		RS_ERR("Couldnt open BT MAC file %s, %s", BT_ADDR_FILE,
		       strerror(errno));
	} else {
		memset(tbuf, 0, sizeof(tbuf));
		result = read(fd, tbuf, size);
		close(fd);
		if (result == -1) {
			RS_ERR("Couldnt read BT MAC file %s, err %s",
			       BT_ADDR_FILE, strerror(errno));
			goto GET_CONFIG;
		}

		if (bachk((const char *)tbuf) < 0) {
			goto GET_CONFIG;
		}

		str = (char *)tbuf;
		for (i = 5; i >= 0; i--) {
			bt_addr[i] = (uint8_t)strtoul(str, NULL, 16);
			str += 3;
		}
            #if 0
		/*reserve LAP addr from 0x9e8b00 to 0x9e8b3f, change to 0x008b** */
		if (0x9e == bt_addr[3] && 0x8b == bt_addr[4]
		    && (bt_addr[5] <= 0x3f)) {
			bt_addr[3] = 0x00;
		}
            #endif
		customer_bdaddr = 1;
	}
    }
#endif

GET_CONFIG:
	RS_DBG("BT MAC is %02x:%02x:%02x:%02x:%02x:%02x",
		       bt_addr[5], bt_addr[4],
		       bt_addr[3], bt_addr[2],
		       bt_addr[1], bt_addr[0]);
	//ret = sprintf(bt_config_file_name, BT_CONFIG_DIRECTORY "rtlbt_config");
	ret = sprintf(bt_config_file_name, "%s", BT_CONFIG_DIRECTORY);
	strcat(bt_config_file_name, btrtl->patch_ent->config_file);
	if (stat(bt_config_file_name, &st) < 0) {
		RS_ERR("can't access bt config file:%s, errno:%d\n",
		       bt_config_file_name, errno);
		return -1;
	}

	filelen = st.st_size;

	if ((fd = open(bt_config_file_name, O_RDONLY)) < 0) {
		perror("Can't open bt config file");
		return -1;
	}

	tlength = filelen;
#ifdef USE_CUSTOMER_ADDRESS
	tlength += 9;
#endif

#ifdef EXTRA_CONFIG_OPTION
	config_file_proc(EXTRA_CONFIG_FILE);
	if (!xtalset_supported())
		config_flags &= ~CONFIG_XTAL;
	if (config_flags & CONFIG_TXPOWER)
		tlength += 7;
	if (config_flags & CONFIG_XTAL)
		tlength += 4;
#endif

	if ((*config_buf = malloc(tlength)) == NULL) {
		RS_DBG("Couldnt malloc buffer for config (%zd)\n", tlength);
		close(fd);
		return -1;
	}
	//we may need to parse this config file.
	//for easy debug, only get need data.

	if (read(fd, *config_buf, filelen) < (ssize_t) filelen) {
		perror("Can't load bt config file");
		free(*config_buf);
		close(fd);
		return -1;
	}

	*config_buf = rtk_parse_config_file(*config_buf, &filelen, bt_addr,
					    config_baud_rate);
	util_hexdump((const uint8_t *)*config_buf, filelen);
	RS_INFO("Cfg length %u", (uint16_t)filelen);
	RS_DBG("Get config baud rate from config file:%x",
	       (unsigned int)*config_baud_rate);

	close(fd);
	return filelen;
}

/**
* Realtek change speed with h5 proto. Using vendor specified command packet to achieve this.
*
* @warning it will waiting 2s for reply.
*
* @param fd uart file descriptor
* @param baudrate the speed want to change
*
*/
int rtk_vendor_change_speed_h5(int fd, RT_U32 baudrate)
{
	struct sk_buff *cmd_change_bdrate = NULL;
	unsigned char cmd[7] = { 0 };
	int retlen;
	unsigned char bytes[READ_DATA_SIZE];
	struct sigaction sa;

	sa.sa_handler = h5_tinit_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	cmd[0] = 0x17;	//ocf
	cmd[1] = 0xfc;	//ogf, vendor specified

	cmd[2] = 4;	//length;

	baudrate = cpu_to_le32(baudrate);
#ifdef BAUDRATE_4BYTES
	memcpy((RT_U16 *) & cmd[3], &baudrate, 4);
#else
	memcpy((RT_U16 *) & cmd[3], &baudrate, 2);

	cmd[5] = 0;
	cmd[6] = 0;
#endif

	RS_DBG("baudrate in change speed command: 0x%x 0x%x 0x%x 0x%x \n",
	       cmd[3], cmd[4], cmd[5], cmd[6]);

	cmd_change_bdrate =
	    h5_prepare_pkt(&rtk_hw_cfg, cmd, 7, HCI_COMMAND_PKT);
	if (!cmd_change_bdrate) {
		RS_ERR("Prepare command packet for change speed fail");
		return -1;
	}

	rtk_hw_cfg.host_last_cmd = cmd_change_bdrate;
	alarm(1);
	write(fd, cmd_change_bdrate->data, cmd_change_bdrate->data_len);

	while (rtk_hw_cfg.link_estab_state == H5_INIT) {
		if ((retlen = read_check_rtk(fd, &bytes, READ_DATA_SIZE)) == -1) {
			perror("read fail");
			return -1;
		}
		//add pure ack check
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}

	alarm(0);
	return 0;
}

/**
* Init realtek Bluetooth h5 proto. h5 proto is added by realtek in the right kernel.
* Generally there are two steps: h5 sync and h5 config
*
* @param fd uart file descriptor
* @param ti termios struct
*
*/
int rtk_init_h5(int fd, struct termios *ti)
{
	unsigned char bytes[READ_DATA_SIZE];
	struct sigaction sa;
	int retlen;
	struct itimerval value;

	/* set even parity */
	ti->c_cflag |= PARENB;
	ti->c_cflag &= ~(PARODD);
	if (tcsetattr(fd, TCSANOW, ti) < 0) {
		RS_ERR("Can't set port settings");
		return -1;
	}

	rtk_hw_cfg.h5_max_retries = H5_MAX_RETRY_COUNT;

	alarm(0);
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = h5_tsync_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	/* h5 sync */
	h5_tsync_sig_alarm(0);
	rtk_hw_cfg.link_estab_state = H5_SYNC;
	while (rtk_hw_cfg.link_estab_state == H5_SYNC) {
		if ((retlen = read_check_rtk(fd, &bytes, READ_DATA_SIZE)) == -1) {
			RS_ERR("H5 Read Sync Response Failed");

			value.it_value.tv_sec = 0;
			value.it_value.tv_usec = 0;
			value.it_interval.tv_sec = 0;
			value.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &value, NULL);

			return -1;
		}
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &value, NULL);

	/* h5 config */
	sa.sa_handler = h5_tconf_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);
	h5_tconf_sig_alarm(0);
	while (rtk_hw_cfg.link_estab_state == H5_CONFIG) {
		if ((retlen = read_check_rtk(fd, &bytes, READ_DATA_SIZE)) == -1) {
			RS_ERR("H5 Read Config Response Failed");
			value.it_value.tv_sec = 0;
			value.it_value.tv_usec = 0;
			value.it_interval.tv_sec = 0;
			value.it_interval.tv_usec = 0;
			setitimer(ITIMER_REAL, &value, NULL);
			return -1;
		}
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}

	value.it_value.tv_sec = 0;
	value.it_value.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &value, NULL);

	rtk_send_pure_ack_down(fd);
	RS_DBG("H5 init finished\n");

	rtk_hw_cfg.rom_version_cmd_state = cmd_not_send;
	rtk_hw_cfg.hci_version_cmd_state = cmd_not_send;
	return 0;
}

static int h4_hci_reset(int fd)
{
	uint8_t buf[64];
	int result;
	int state = 1;
	int count = 0;
	int params_len;
	uint8_t cmd[4] = { 0x01, 0x03, 0x0c, 0x00};
	struct pollfd p;

	RS_INFO("%s: Issue hci reset cmd", __func__);

	result = write(rtk_hw_cfg.serial_fd, cmd, sizeof(cmd));
	if (result != sizeof(cmd)) {
		RS_ERR("%s: Failed to send reset cmd", __func__);
		return -1;
	}

start:
	memset(buf, 0, sizeof(buf));
	state = 1;
	count = 0;
	p.fd = fd;
	p.events = POLLERR | POLLHUP | POLLIN;
	for (;;) {
		p.revents = 0;
		result = poll(&p, 1, 1000); /* 1000ms */
		if (result < 0) {
			RS_ERR("Poll call error, %s", strerror(errno));
			result = -1;
			break;
		}

		if (result == 0) {
			RS_ERR("%s: Timeout", __func__);
			result = -1;
			break;
		}

		if (p.revents & (POLLERR | POLLHUP)) {
			RS_ERR("POLLERR or POLLUP happens, %s",
			       strerror(errno));
			result = -1;
			break;
		}

		if (state == 1) {
			result = read(p.fd, buf, 1);
			if (result == -1 || result != 1) {
				RS_ERR("%s: Read pkt type error, %s", __func__,
				       strerror(errno));
				result = -1;
				break;
			}
			if (result == 1 && buf[0] == 0x04) {
				count = 1;
				state = 2;
			}
		} else if (state == 2) {
			result = read(p.fd, buf + count, 2);
			if (result == -1 || result != 2) {
				RS_ERR("%s: Read pkt header error, %s",
				       __func__, strerror(errno));
				break;
			}
			count += result;
			state = 3;
			params_len = buf[2];
			if (params_len + 3 > sizeof(buf)) {
				result = -1;
				RS_ERR("%s: hci event too long", __func__);
				break;
			}
		} else if (state == 3) {
			result = read(p.fd, buf + count, params_len);
			if (result == -1)
				RS_ERR("%s: Read pkt payload error, %s",
				       __func__, strerror(errno));
			count += result;
			params_len -= result;
			if (!params_len)
				break;
		}
	}

	if (result >= 0) {
		if (buf[1] == 0x0e) {
			uint16_t opcode;

			opcode = (uint16_t)buf[4] | buf[5] << 8;
			if (opcode == 0x0c03) {
				RS_INFO("Cmd complete event for hci reset");
				return 0;
			}
		} else {
			RS_INFO("%s: Unexpected hci event packet", __func__);
			util_hexdump(buf, count);
		}
		goto start;
	}

	return result;
}

static void h5_hci_reset_timeout(int sig)
{
	static int retries = 0;
	RS_ERR("h5 hci reset timed out\n");
	exit(1);
}

static int h5_hci_reset(int fd)
{
	uint8_t resp[READ_DATA_SIZE];
	struct sigaction sa;
	uint8_t cmd[3] = { 0x03, 0x0c, 0x00};
	struct sk_buff *nskb;
	int result;

	RS_INFO("%s: Issue hci reset cmd", __func__);

	sa.sa_handler = h5_hci_reset_timeout;
	sigaction(SIGALRM, &sa, NULL);
	alarm(2);

	nskb = h5_prepare_pkt(&rtk_hw_cfg, cmd, sizeof(cmd), HCI_COMMAND_PKT);

	if (rtk_hw_cfg.host_last_cmd) {
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
	}
	/* The skb will be released in cmd complete event process */
	rtk_hw_cfg.host_last_cmd = nskb;

	result = write(fd, nskb->data, nskb->data_len);
	if (result == -1) {
		RS_ERR("%s: Send hci reset cmd error, %s\n", __func__,
		       strerror(result));
		skb_free(nskb);
		rtk_hw_cfg.host_last_cmd = NULL;
		return -1;
	}
	if (result != nskb->data_len) {
		RS_ERR("%s: Send hci reset cmd error, %d, %d\n", __func__,
		       result, nskb->data_len);
		skb_free(nskb);
		rtk_hw_cfg.host_last_cmd = NULL;
		return -1;
	}
	rtk_hw_cfg.reset_cmd_state = cmd_has_sent;

	while (rtk_hw_cfg.reset_cmd_state != event_received) {
		result = read_check_rtk(fd, &resp, READ_DATA_SIZE);
		if (result == -1) {
			RS_ERR("%s: Read error %s", __func__, strerror(errno));
			return -1;
		}
		h5_recv(&rtk_hw_cfg, &resp, result);
	}

	alarm(0);

	return 0;
}

/**
* Download realtek firmware and config file from uart with the proto.
* Parse the content to serval packets follow the proto and then write the packets from uart
*
* @param fd uart file descriptor
* @param buf addr where stor the content of firmware and config file
* @param filesize length of buf
* @param is_sent_changerate if baudrate need to be changed
* @param proto realtek Bluetooth protocol, shall be either HCI_UART_H4 or HCI_UART_3WIRE
*
*/
static int rtk_download_fw_config(int fd, RT_U8 * buf, size_t filesize,
				  int is_sent_changerate, int proto,
				  struct termios *ti)
{
	uint8_t iCurIndex = 0;
	uint8_t iCurLen = 0;
	uint8_t iEndIndex = 0;
	uint8_t iLastPacketLen = 0;
	uint8_t add_pkts = 0;
	uint8_t iTotalIndex = 0;
	uint8_t iCmdSentNum = 0;
	unsigned char *bufpatch;
	uint8_t i, j;
	int result;

	iEndIndex = (uint8_t) ((filesize - 1) / PATCH_DATA_FIELD_MAX_SIZE);
	iLastPacketLen = (filesize) % PATCH_DATA_FIELD_MAX_SIZE;

	/* if (is_sent_changerate)
	 * 	iCmdSentNum++;
	 * if (rtk_hw_cfg.rom_version_cmd_state >= cmd_has_sent)
	 * 	iCmdSentNum++;
	 * if (rtk_hw_cfg.hci_version_cmd_state >= cmd_has_sent)
	 * 	iCmdSentNum++; */
	iCmdSentNum += rtk_hw_cfg.num_of_cmd_sent;

	add_pkts =
	    (iEndIndex + 1 + iCmdSentNum) % 8 ? (8 -
						 (iEndIndex + 1 +
						  iCmdSentNum) % 8) : 0;

	/* Make sure the next seqno is zero after download patch and
	 * hci reset
	 */
	if (proto == HCI_UART_3WIRE) {
		if (add_pkts)
			add_pkts -= 1;
		else
			add_pkts += 7;
	} else
		add_pkts = 0; /* No additional packets need */

	iTotalIndex = add_pkts + iEndIndex;
	rtk_hw_cfg.total_num = iTotalIndex;	//init TotalIndex

	RS_DBG("iEndIndex: %d, iLastPacketLen: %d, additional pkts: %d\n",
	       iEndIndex, iLastPacketLen, add_pkts);

	if (iLastPacketLen == 0)
		iLastPacketLen = PATCH_DATA_FIELD_MAX_SIZE;

	bufpatch = buf;

	for (i = 0; i <= iTotalIndex; i++) {
		/* Index will roll over when it reaches 0x80. */
		if (i > 0x7f)
			j = (i & 0x7f) + 1;
		else
			j = i;

		if (i < iEndIndex) {
			iCurIndex = j;
			iCurLen = PATCH_DATA_FIELD_MAX_SIZE;
		} else if (i == iEndIndex) {
			/* Send last data packets */
			if (i == iTotalIndex)
				iCurIndex = j | 0x80;
			else
				iCurIndex = j;
			iCurLen = iLastPacketLen;
		} else if (i < iTotalIndex) {
			/* Send additional packets */
			iCurIndex = j;
			bufpatch = NULL;
			iCurLen = 0;
			RS_DBG("Send additional packet %u", iCurIndex);
		} else {
			/* Send end packet */
			iCurIndex = j | 0x80;
			bufpatch = NULL;
			iCurLen = 0;
			RS_DBG("Send end packet %u", iCurIndex);
		}

		if (iCurIndex & 0x80)
			RS_DBG("Send FW last command");

		if (proto == HCI_UART_H4) {
			iCurIndex = hci_download_patch_h4(fd, iCurIndex,
							  bufpatch, iCurLen);
			if ((iCurIndex != j) && (i != rtk_hw_cfg.total_num)) {
				RS_DBG(
				  "index mismatch j %d, iCurIndex:%d, fail\n",
				  j, iCurIndex);
				return -1;
			}
		} else if (proto == HCI_UART_3WIRE) {
			if (hci_download_patch(fd, iCurIndex, bufpatch, iCurLen,
					       ti) < 0)
				return -1;
		}

		if (iCurIndex < iEndIndex) {
			bufpatch += PATCH_DATA_FIELD_MAX_SIZE;
		}
	}

	if (proto == HCI_UART_H4)
		result = h4_hci_reset(fd);
	else
		result = h5_hci_reset(fd);

	if (result)
		return result;

	/* Set last ack packet down */
	if (proto == HCI_UART_3WIRE) {
		RS_INFO("%s: send last ack", __func__);
		rtk_send_pure_ack_down(fd);
	}

	return 0;
}

/**
* Get realtek Bluetooth firmaware file. The content will be saved in *fw_buf which is malloc here.
* The length malloc here will be lager than length of firmware file if there is a config file.
* The content of config file will copy to the tail of *fw_buf in rtk_config.
*
* @param fw_buf point to the addr where stored the content of firmware.
* @param addi_len length of config file.
* @return length of *fw_buf.
*
*/
int rtk_get_bt_firmware(struct btrtl_info *btrtl, RT_U8 **fw_buf)
{
	const char *filename;
	struct stat st;
	int fd = -1;
	size_t fwsize;
	static char firmware_file_name[PATH_MAX] = { 0 };
	int ret = 0;

	sprintf(firmware_file_name, "%s", FIRMWARE_DIRECTORY);
	strcat(firmware_file_name, btrtl->patch_ent->patch_file);
	filename = firmware_file_name;

	if (stat(filename, &st) < 0) {
		RS_ERR("Can't access firmware %s, %s", filename,
		       strerror(errno));
		return -1;
	}

	fwsize = st.st_size;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		RS_ERR("Can't open firmware, errno:%d", errno);
		return -1;
	}

	if (!(*fw_buf = malloc(fwsize))) {
		RS_ERR("Can't alloc memory for fw&config, errno:%d", errno);
		close(fd);
		return -1;
	}

	if (read(fd, *fw_buf, fwsize) < (ssize_t) fwsize) {
		free(*fw_buf);
		*fw_buf = NULL;
		close(fd);
		return -1;
	}
	RS_DBG("Load FW OK");
	close(fd);
	return fwsize;
}

//These two function(rtk<-->uart speed transfer) need check Host uart speed at first!!!! IMPORTANT
//add more speed if neccessary
typedef struct _baudrate_ex {
	RT_U32 rtk_speed;
	int uart_speed;
} baudrate_ex;

#ifdef BAUDRATE_4BYTES
baudrate_ex baudrates[] = {
#ifdef RTL_8703A_SUPPORT
	{0x00004003, 1500000}, /* for rtl8703as */
#endif
	{0x0252C014, 115200},
	{0x0252C00A, 230400},
	{0x05F75004, 921600},
	{0x00005004, 1000000},
	{0x04928002, 1500000},
	{0x01128002, 1500000},	//8761AT
	{0x00005002, 2000000},
	{0x0000B001, 2500000},
	{0x04928001, 3000000},
	{0x052A6001, 3500000},
	{0x00005001, 4000000},
};
#else
baudrate_ex baudrates[] = {
	{0x701d, 115200}
	{0x6004, 921600},
	{0x4003, 1500000},
	{0x5002, 2000000},
	{0x8001, 3000000},
	{0x9001, 3000000},
	{0x7001, 3500000},
	{0x5001, 4000000},
};
#endif

/**
* Change realtek Bluetooth speed to uart speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] =
* {
*  	{0x7001, 3500000},
*	{0x6004, 921600},
*	{0x4003, 1500000},
*	{0x5001, 4000000},
*	{0x5002, 2000000},
*	{0x8001, 3000000},
*	{0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, uart speed will be set as #115200.
*
* @param rtk_speed realtek Bluetooth speed
* @param uart_speed uart speed
*
*/
static void rtk_speed_to_uart_speed(RT_U32 rtk_speed, RT_U32 * uart_speed)
{
	*uart_speed = 115200;

	unsigned int i;
	for (i = 0; i < sizeof(baudrates) / sizeof(baudrate_ex); i++) {
		if (baudrates[i].rtk_speed == rtk_speed) {
			*uart_speed = baudrates[i].uart_speed;
			return;
		}
	}
	return;
}

/**
* Change uart speed to realtek Bluetooth speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] =
* {
*  	{0x7001, 3500000},
*	{0x6004, 921600},
*	{0x4003, 1500000},
*	{0x5001, 4000000},
*	{0x5002, 2000000},
*	{0x8001, 3000000},
*	{0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, realtek Bluetooth speed will be set as #0x701D.
*
* @param uart_speed uart speed
* @param rtk_speed realtek Bluetooth speed
*
*/
static inline void uart_speed_to_rtk_speed(int uart_speed, RT_U32 * rtk_speed)
{
	*rtk_speed = 0x701D;

	unsigned int i;
	for (i = 0; i < sizeof(baudrates) / sizeof(baudrate_ex); i++) {
		if (baudrates[i].uart_speed == uart_speed) {
			*rtk_speed = baudrates[i].rtk_speed;
			return;
		}
	}

	return;
}

static void rtk_get_chip_type_timeout(int sig)
{
	static int retries = 0;
	int len = 0;
	struct btrtl_info *btrtl = &rtk_hw_cfg;

	RS_INFO("RTK get HCI_VENDOR_READ_RTK_CHIP_TYPE_Command\n");
	if (retries < btrtl->h5_max_retries) {
		RS_DBG("rtk get chip type timerout, retry:%d\n", retries);
		if (btrtl->host_last_cmd) {
			len = write(btrtl->serial_fd,
				    btrtl->host_last_cmd->data,
				    btrtl->host_last_cmd->data_len);
		}
		retries++;
		alarm(3);
		return;
	}
	tcflush(btrtl->serial_fd, TCIOFLUSH);
	RS_ERR("rtk get chip type cmd complete event timed out\n");
	exit(1);
}

void rtk_get_chip_type(int dd)
{
	unsigned char bytes[READ_DATA_SIZE];
	int retlen;
	struct sigaction sa;
	/* 0xB000A094 */
	unsigned char cmd_buff[] = {0x61, 0xfc,
		0x05, 0x00, 0x94, 0xa0, 0x00, 0xb0};
	struct sk_buff *nskb;
	struct btrtl_info *btrtl = &rtk_hw_cfg;

	nskb = h5_prepare_pkt(btrtl, cmd_buff, sizeof(cmd_buff),
			HCI_COMMAND_PKT);
	if (btrtl->host_last_cmd){
		skb_free(btrtl->host_last_cmd);
		btrtl->host_last_cmd = NULL;
	}

	btrtl->host_last_cmd = nskb;

	write(dd, nskb->data, nskb->data_len);
	RS_INFO("RTK send HCI_VENDOR_READ_CHIP_TYPE_Command\n");

	alarm(0);
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = rtk_get_chip_type_timeout;
	sigaction(SIGALRM, &sa, NULL);

	alarm(3);
	while (btrtl->chip_type_cmd_state != event_received) {
		if ((retlen = read_check_rtk(dd, &bytes, READ_DATA_SIZE)) == -1) {
			perror("rtk get chip type: read fail");
			return;
		}
		h5_recv(btrtl, &bytes, retlen);
	}
	alarm(0);
	return;
}

static void rtk_get_eversion_timeout(int sig)
{
	static int retries = 0;
	int len = 0;

	if (retries < rtk_hw_cfg.h5_max_retries) {
		RS_DBG("No response for HCI_Vendor_Read_RTK_ROM_Verision, retry%d\n",
		       retries);
		if (rtk_hw_cfg.host_last_cmd) {
			len = write(rtk_hw_cfg.serial_fd,
				    rtk_hw_cfg.host_last_cmd->data,
				    rtk_hw_cfg.host_last_cmd->data_len);
		}
		retries++;
		alarm(3);
		return;
	}
	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("Timed out to get cmd complete event of read rtk rom version cmd\n");
	exit(1);
}

/**
* Send vendor cmd to get eversion: 0xfc6d
* If Rom code does not support this cmd, use default.
*/
void rtk_get_eversion(int dd)
{
	unsigned char bytes[READ_DATA_SIZE];
	int retlen;
	struct sigaction sa;
	unsigned char read_rom_patch_cmd[3] = { 0x6d, 0xfc, 00 };
	struct sk_buff *nskb =
	    h5_prepare_pkt(&rtk_hw_cfg, read_rom_patch_cmd, 3, HCI_COMMAND_PKT);

	if (rtk_hw_cfg.host_last_cmd) {
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
	}

	rtk_hw_cfg.host_last_cmd = nskb;

	write(dd, nskb->data, nskb->data_len);
	rtk_hw_cfg.rom_version_cmd_state = cmd_has_sent;
	RS_DBG("RTK send HCI_Vendor_Read_RTK_ROM_Verision command\n");

	alarm(0);
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = rtk_get_eversion_timeout;
	sigaction(SIGALRM, &sa, NULL);

	alarm(3);
	while (rtk_hw_cfg.rom_version_cmd_state != event_received) {
		if ((retlen = read_check_rtk(dd, &bytes, READ_DATA_SIZE)) == -1) {
			perror("rtk get eversion: read fail");
			return;
		}
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}
	alarm(0);
	return;
}

static void rtk_get_lmp_version_timeout(int sig)
{
	static int retries = 0;

	if (retries < rtk_hw_cfg.h5_max_retries) {
		RS_DBG("No response for HCI_Read_Local_Version_Information, retry %d\n",
		       retries);
		if (rtk_hw_cfg.host_last_cmd) {
			int len = write(rtk_hw_cfg.serial_fd,
					rtk_hw_cfg.host_last_cmd->data,
					rtk_hw_cfg.host_last_cmd->data_len);
		}
		retries++;
		alarm(3);
		return;
	}
	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("Timed out to get cmd complete event of read local version info cmd\n");
	exit(1);
}

void rtk_get_lmp_version(int dd)
{
	unsigned char bytes[READ_DATA_SIZE];
	int retlen;
	struct sigaction sa;
	unsigned char read_rom_patch_cmd[3] = { 0x01, 0x10, 00 };
	struct sk_buff *nskb = h5_prepare_pkt(&rtk_hw_cfg, read_rom_patch_cmd, 3, HCI_COMMAND_PKT);	//data:len+head:4

	if (rtk_hw_cfg.host_last_cmd) {
		skb_free(rtk_hw_cfg.host_last_cmd);
		rtk_hw_cfg.host_last_cmd = NULL;
	}

	rtk_hw_cfg.host_last_cmd = nskb;

	write(dd, nskb->data, nskb->data_len);
	rtk_hw_cfg.hci_version_cmd_state = cmd_has_sent;
	RS_DBG("RTK send HCI_Read_Local_Version_Information command\n");

	alarm(0);
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = rtk_get_lmp_version_timeout;
	sigaction(SIGALRM, &sa, NULL);

	alarm(3);
	while (rtk_hw_cfg.hci_version_cmd_state != event_received) {
		if ((retlen = read_check_rtk(dd, &bytes, READ_DATA_SIZE)) == -1) {
			perror("read fail");
			return;
		}
		h5_recv(&rtk_hw_cfg, &bytes, retlen);
	}
	alarm(0);
	return;
}

static int rtk_max_retries = 5;

static void rtk_local_ver_sig_alarm(int sig)
{
	uint8_t cmd[4] = { 0x01, 0x01, 0x10, 0x00 };
	static int retries;

	if (retries < rtk_max_retries) {
		retries++;
		if (write(rtk_hw_cfg.serial_fd, cmd, sizeof(cmd)) < 0)
			return;
		alarm(1);
		return;
	}

	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("init timed out, read local ver fails");
	exit(1);
}

static void rtk_hci_local_ver(int fd)
{
	struct sigaction sa;
	uint8_t result[258];
	int ret;

	alarm(0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = rtk_local_ver_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	rtk_local_ver_sig_alarm(0);

	while (1) {
		ret = read_hci_evt(fd, result, 0x0e);
		/* break down is not needed if read fails, because the program
		 * will exit when alarm timeout
		 */
		if (ret < 0)
			RS_ERR("%s: Read HCI event error.", __func__);
		else
			break;
	}

	/* Cancel pending alarm */
	alarm(0);

	if (ret != 15) {
		RS_ERR("%s: incorrect complete event, len %u", __func__, ret);
		exit(1);
	}

	if (result[6]) {
		RS_ERR("%s: status is %u", __func__, result[6]);
		exit(1);
	}

	rtk_hw_cfg.hci_ver = result[7];
	rtk_hw_cfg.hci_rev = (uint32_t)result[9] << 8 | result[8];
	rtk_hw_cfg.lmp_subver = (uint32_t)result[14] << 8 | result[13];
}

static void rtk_rom_ver_sig_alarm(int sig)
{
	uint8_t cmd[4] = { 0x01, 0x6d, 0xfc, 0x00 };
	static int retries;

	if (retries < rtk_max_retries) {
		retries++;
		if (write(rtk_hw_cfg.serial_fd, cmd, sizeof(cmd)) < 0)
			return;
		alarm(1);
		return;
	}

	tcflush(rtk_hw_cfg.serial_fd, TCIOFLUSH);
	RS_ERR("init timed out, read rom ver fails");
	exit(1);
}

static void rtk_hci_rom_ver(int fd)
{
	struct sigaction sa;
	uint8_t result[256];
	int ret;

	alarm(0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = rtk_rom_ver_sig_alarm;
	sigaction(SIGALRM, &sa, NULL);

	rtk_rom_ver_sig_alarm(0);

	while (1) {
		ret = read_hci_evt(fd, result, 0x0e);
		/* break down is not needed if read fails, because the program
		 * will exit when alarm timeout
		 */
		if (ret < 0)
			RS_ERR("%s: Read HCI event error.", __func__);
		else
			break;
	}

	/* Cancel pending alarm */
	alarm(0);

	if (ret != 8) {
		RS_ERR("%s: incorrect complete event, len %u", __func__, ret);
		exit(1);
	}

	if (result[6]) {
		RS_ERR("%s: status is %u", __func__, result[6]);
		rtk_hw_cfg.eversion = 0;
	} else
		rtk_hw_cfg.eversion = result[7];
}

uint8_t rtk_get_fw_project_id(uint8_t * p_buf)
{
	uint8_t opcode;
	uint8_t len;
	uint8_t data = 0;

	do {
		opcode = *p_buf;
		len = *(p_buf - 1);
		if (opcode == 0x00) {
			if (len == 1) {
				data = *(p_buf - 2);
				RS_DBG
				    ("rtk_get_fw_project_id: opcode %d, len %d, data %d",
				     opcode, len, data);
				break;
			} else {
				RS_ERR("rtk_get_fw_project_id: invalid len %d",
				       len);
			}
		}
		p_buf -= len + 2;
	} while (*p_buf != 0xFF);

	return data;
}

struct rtk_epatch_entry *rtk_get_patch_entry(void)
{
	uint16_t i;
	struct rtk_epatch *patch;
	struct rtk_epatch_entry *entry;
	uint8_t *p;
	uint16_t chip_id;
	uint32_t tmp;

	patch = (struct rtk_epatch *)rtk_hw_cfg.fw_buf;
	entry = (struct rtk_epatch_entry *)malloc(sizeof(*entry));
	if (!entry) {
		RS_ERR("failed to allocate mem for patch entry");
		return NULL;
	}

	patch->number_of_patch = le16_to_cpu(patch->number_of_patch);

	RS_DBG("fw_ver 0x%08x, patch_num %d",
	       le32_to_cpu(patch->fw_version), patch->number_of_patch);

	for (i = 0; i < patch->number_of_patch; i++) {
		RS_DBG("chip id 0x%04x",
			get_unaligned_le16(rtk_hw_cfg.fw_buf + 14 + 2 * i));
		if (get_unaligned_le16(rtk_hw_cfg.fw_buf + 14 + 2 * i) ==
		    rtk_hw_cfg.eversion + 1) {
			entry->chipID = rtk_hw_cfg.eversion + 1;
			entry->patch_length =
			    get_unaligned_le16(rtk_hw_cfg.fw_buf + 14 +
					       2 * patch->number_of_patch +
					       2 * i);
			entry->start_offset =
			    get_unaligned_le32(rtk_hw_cfg.fw_buf + 14 +
					       4 * patch->number_of_patch +
					       4 * i);
			RS_DBG("patch length is 0x%x", entry->patch_length);
			RS_DBG("start offset is 0x%x", entry->start_offset);

			entry->svn_ver = get_unaligned_le32(rtk_hw_cfg.fw_buf +
						entry->start_offset +
						entry->patch_length - 8);
			entry->coex_ver = get_unaligned_le32(rtk_hw_cfg.fw_buf +
						entry->start_offset +
						entry->patch_length - 12);

			RS_INFO("Svn version: %8d\n", entry->svn_ver);
			tmp = ((entry->coex_ver >> 16) & 0x7ff) +
			      (entry->coex_ver >> 27) * 10000;
			RS_INFO("Coexistence: BTCOEX_20%06d-%04x\n", tmp,
				(entry->coex_ver & 0xffff));

			break;
		}

	}

	if (i == patch->number_of_patch) {
		RS_ERR("failed to get entry");
		free(entry);
		entry = NULL;
	}

	return entry;
}

void rtk_get_final_patch(int fd, int proto)
{
	struct btrtl_info *rtl = &rtk_hw_cfg;
	uint8_t proj_id = 0;
	struct rtk_epatch_entry *entry = NULL;
	struct rtk_epatch *patch = (struct rtk_epatch *)rtl->fw_buf;
	uint32_t svn_ver, coex_ver, tmp;

	if ((proto == HCI_UART_H4 && rtl->lmp_subver != ROM_LMP_8723b) ||
	    ((proto == HCI_UART_3WIRE) && (rtl->lmp_subver == ROM_LMP_8723a))) {
		if (memcmp(rtl->fw_buf, RTK_EPATCH_SIGNATURE, 8) == 0) {
			RS_ERR("Check signature error!");
			rtl->dl_fw_flag = 0;
			goto free_buf;
		} else {
			rtl->total_len = rtl->config_len + rtl->fw_len;
			if (!(rtl->total_buf = malloc(rtl->total_len))) {
				RS_ERR("Can't alloc mem for fw/config, errno:%d",
				       errno);
				rtl->dl_fw_flag = 0;
				rtl->total_len = 0;
				goto free_buf;
			} else {
				RS_DBG("fw copy directy");

				svn_ver = get_unaligned_le32(
						rtl->fw_buf + rtl->fw_len - 8);
				coex_ver = get_unaligned_le32(
						rtl->fw_buf + rtl->fw_len - 12);

				RS_INFO("Svn version: %8d\n", svn_ver);
				tmp = ((coex_ver >> 16) & 0x7ff) +
					(coex_ver >> 27) * 10000;
				RS_INFO("Coexistence: BTCOEX_20%06d-%04x\n", tmp,
						(coex_ver & 0xffff));

				memcpy(rtl->total_buf, rtl->fw_buf,
				       rtl->fw_len);
				if (rtl->config_len)
					memcpy(rtl->total_buf +
					       rtl->fw_len,
					       rtl->config_buf,
					       rtl->config_len);
				rtl->dl_fw_flag = 1;
				goto free_buf;
			}
		}
	}

	if (memcmp(rtl->fw_buf, RTK_EPATCH_SIGNATURE, 8)) {
		RS_DBG("check signature error!");
		rtl->dl_fw_flag = 0;
		goto free_buf;
	}

	if (memcmp
	    (rtl->fw_buf + rtl->fw_len - 4,
	     Extension_Section_SIGNATURE, 4)) {
		RS_ERR("check extension section signature error");
		rtl->dl_fw_flag = 0;
		goto free_buf;
	}

	proj_id = rtk_get_fw_project_id(rtl->fw_buf + rtl->fw_len - 5);

#ifdef RTL_8703A_SUPPORT
	if (rtl->hci_ver == 0x4 && rtl->lmp_subver == ROM_LMP_8723b) {
		RS_DBG("HCI version = 0x4, IC is 8703A.");
	} else {
		RS_ERR("error: lmp_version %x, hci_version %x, project_id %x",
		       rtl->lmp_subver, rtl->hci_ver, project_id[proj_id]);
		rtk_hw_cfg.dl_fw_flag = 0;
		goto free_buf;
	}
#else
	if (rtl->lmp_subver != ROM_LMP_8703b) {
		if (rtl->lmp_subver != project_id[proj_id]) {
			RS_ERR("lmp_subver %04x, project id %04x, mismatch\n",
			       rtl->lmp_subver, project_id[proj_id]);
			rtl->dl_fw_flag = 0;
			goto free_buf;
		}
	} else {
		/* if (rtk_hw_cfg.patch_ent->proj_id != project_id[proj_id]) {
		 * 	RS_ERR("proj_id %04x, version %04x from firmware "
		 * 	       "project_id[%u], mismatch\n",
		 * 	       rtk_hw_cfg.patch_ent->proj_id,
		 * 	       project_id[proj_id], proj_id);
		 * 	rtk_hw_cfg.dl_fw_flag = 0;
		 * 	goto free_buf;
		 * } */
	}
#endif

	entry = rtk_get_patch_entry();

	if (entry)
		rtl->total_len =
		    entry->patch_length + rtl->config_len;
	else {
		rtl->dl_fw_flag = 0;
		goto free_buf;
	}

	if (!(rtl->total_buf = malloc(rtl->total_len))) {
		RS_ERR("Can't alloc memory for multi fw&config, errno:%d",
		       errno);
		rtl->dl_fw_flag = 0;
		rtl->total_len = 0;
		goto free_buf;
	} else {
		memcpy(rtl->total_buf,
		       rtl->fw_buf + entry->start_offset,
		       entry->patch_length);
		memcpy(rtl->total_buf + entry->patch_length - 4,
		       &patch->fw_version, 4);

		if (rtl->config_len)
			memcpy(rtl->total_buf + entry->patch_length,
			       rtl->config_buf, rtl->config_len);
		rtl->dl_fw_flag = 1;
	}

	RS_DBG("fw:%s exists, config file:%s exists",
	       (rtl->fw_len > 0) ? "" : "not",
	       (rtl->config_len > 0) ? "" : "not");

free_buf:
	if (rtl->fw_len > 0) {
		free(rtl->fw_buf);
		rtl->fw_len = 0;
	}

	if (rtl->config_len > 0) {
		free(rtl->config_buf);
		rtl->config_len = 0;
	}

	if (entry)
		free(entry);
}

/**
* Config realtek Bluetooth. The configuration parameter is get from config file and fw.
* Config file is rtk8723_bt_config. which is set in rtk_get_bt_config.
* fw file is "rlt8723a_chip_b_cut_bt40_fw", which is set in get_firmware_name.
*
* @warning maybe only one of config file and fw file exists. The bt_addr arg is stored in "/data/btmac.txt"
* or "/data/misc/bluetoothd/bt_mac/btmac.txt",
*
* @param fd uart file descriptor
* @param proto realtek Bluetooth protocol, shall be either HCI_UART_H4 or HCI_UART_3WIRE
* @param speed init_speed in uart struct
* @param ti termios struct
* @returns #0 on success
*/
static int rtk_config(int fd, int proto, int speed, struct termios *ti)
{
	int final_speed = 0;
	int ret = 0;
	struct btrtl_info *btrtl = &rtk_hw_cfg;

	btrtl->proto = proto;

	/* Get Local Version Information and RTK ROM version */
	if (proto == HCI_UART_3WIRE) {
		RS_INFO("H5 IC");
		rtk_get_lmp_version(fd);
		rtk_get_eversion(fd);
	} else {
		RS_INFO("H4 IC");

		/* The following set is for special requirement that enables
		 * flow control before initializing */
#ifdef RTL8723DSH4_UART_HWFLOWC
		ti->c_cflag &= ~PARENB;
		ti->c_cflag |= CRTSCTS;
		if (tcsetattr(fd, TCSANOW, ti) < 0) {
			RS_ERR("H4 Can't enable RTSCTS");
			return -1;
		}
		usleep(20 * 1000);
#endif
		rtk_hci_local_ver(fd);
		rtk_hci_rom_ver(fd);
		if (rtk_hw_cfg.lmp_subver == ROM_LMP_8761btc) {
			rtk_hw_cfg.chip_type = CHIP_8761BTC;
			rtk_hw_cfg.hw_flow_control = 1;
			/* TODO: Change to different uart baud */
			uart_speed_to_rtk_speed(1500000, &rtk_hw_cfg.baudrate);
			goto change_baud;
		} else if (rtk_hw_cfg.lmp_subver == ROM_LMP_8761a) {
			if (rtk_hw_cfg.hci_rev == 0x000b) {
				rtk_hw_cfg.chip_type = CHIP_8761B;
				rtk_hw_cfg.hw_flow_control = 1;
				/* TODO: Change to different uart baud */
				uart_speed_to_rtk_speed(1500000, &rtk_hw_cfg.baudrate);
				goto change_baud;
			} else if (rtk_hw_cfg.hci_rev == 0x000a) {
				if (rtk_hw_cfg.eversion == 3)
					rtk_hw_cfg.chip_type = CHIP_8761ATF;
				else if (rtk_hw_cfg.eversion == 2)
					rtk_hw_cfg.chip_type = CHIP_8761AT;
				else
					rtk_hw_cfg.chip_type = CHIP_UNKNOWN;
			}
		} else if (rtk_hw_cfg.lmp_subver == ROM_LMP_8723b) {
			if (rtk_hw_cfg.hci_ver == 0x08 &&
			    rtk_hw_cfg.hci_rev == 0x000d) {
				rtk_hw_cfg.chip_type = CHIP_8723DS;
			} else if (rtk_hw_cfg.hci_ver == 0x06 &&
				 rtk_hw_cfg.hci_rev == 0x000b) {
				rtk_hw_cfg.chip_type = CHIP_8723BS;
			} else {
				RS_ERR("H4: unknown chip");
				return -1;
			}
		}

	}

	RS_INFO("LMP Subversion 0x%04x", btrtl->lmp_subver);
	RS_INFO("EVersion %d", btrtl->eversion);

	switch (rtk_hw_cfg.lmp_subver) {
	case ROM_LMP_8723a:
		break;
	case ROM_LMP_8723b:
#ifdef RTL_8703A_SUPPORT
		/* Set chip type for matching fw/config entry */
		rtl->chip_type = CHIP_8703AS;
#endif
		break;
	case ROM_LMP_8821a:
		break;
	case ROM_LMP_8761a:
		break;
	case ROM_LMP_8703b:
		rtk_get_chip_type(fd);
		break;
	}

	btrtl->patch_ent = get_patch_entry(btrtl);
	if (btrtl->patch_ent) {
		RS_INFO("IC: %s\n", btrtl->patch_ent->ic_name);
		RS_INFO("Firmware/config: %s, %s\n",
			btrtl->patch_ent->patch_file,
			btrtl->patch_ent->config_file);
	} else {
		RS_ERR("Can not find firmware/config entry\n");
		return -1;
	}

	rtk_hw_cfg.config_len =
	    rtk_get_bt_config(btrtl, &btrtl->config_buf, &btrtl->baudrate);
	if (rtk_hw_cfg.config_len < 0) {
		RS_ERR("Get Config file error, just use efuse settings");
		rtk_hw_cfg.config_len = 0;
	}

	rtk_hw_cfg.fw_len = rtk_get_bt_firmware(btrtl, &btrtl->fw_buf);
	if (rtk_hw_cfg.fw_len < 0) {
		RS_ERR("Get BT firmware error");
		rtk_hw_cfg.fw_len = 0;
		return -1;
	} else {
		rtk_get_final_patch(fd, proto);
	}

	if (rtk_hw_cfg.total_len > RTK_PATCH_LENGTH_MAX) {
		RS_ERR("total length of fw&config larger than allowed");
		return -1;
	}

	RS_INFO("Total len %d for fw/config", rtk_hw_cfg.total_len);

	if (rtk_hw_cfg.chip_type == CHIP_8723DS &&
	    rtk_hw_cfg.proto == HCI_UART_H4) {
		if (rtk_hw_cfg.parity_en) {
			/* set parity */
			ti->c_cflag |= PARENB;
			if (rtk_hw_cfg.parity_even)
				ti->c_cflag &= ~(PARODD);
			else
				ti->c_cflag |= PARODD;
			if (tcsetattr(fd, TCSANOW, ti) < 0) {
				RS_ERR("8723DSH4 Can't set parity");
				return -1;
			}
		}
	}

change_baud:
	/* change baudrate if needed
	 * rtk_hw_cfg.baudrate is a __u32/__u16 vendor-specific variable
	 * parsed from config file
	 * */
	if (rtk_hw_cfg.baudrate == 0) {
		uart_speed_to_rtk_speed(speed, &rtk_hw_cfg.baudrate);
		RS_DBG("No cfg file, set baudrate, : %u, 0x%08x",
		       (unsigned int)speed, (unsigned int)rtk_hw_cfg.baudrate);
		goto SET_FLOW_CONTRL;
	} else
		rtk_speed_to_uart_speed(rtk_hw_cfg.baudrate,
					(RT_U32 *) & (rtk_hw_cfg.final_speed));

	if (proto == HCI_UART_3WIRE)
		rtk_vendor_change_speed_h5(fd, rtk_hw_cfg.baudrate);
	else
		rtk_vendor_change_speed_h4(fd, rtk_hw_cfg.baudrate);

	usleep(50000);
	final_speed = rtk_hw_cfg.final_speed ? rtk_hw_cfg.final_speed : speed;
	RS_DBG("final_speed %d\n", final_speed);
	if (set_speed(fd, ti, final_speed) < 0) {
		RS_ERR("Can't set baud rate:%x, %x, %x", final_speed,
		       rtk_hw_cfg.final_speed, speed);
		return -1;
	}

SET_FLOW_CONTRL:
	if (rtk_hw_cfg.hw_flow_control) {
		RS_DBG("hw flow control enable");
		ti->c_cflag |= CRTSCTS;

		if (tcsetattr(fd, TCSANOW, ti) < 0) {
			RS_ERR("Can't set port settings");
			return -1;
		}
	} else {
		RS_DBG("hw flow control disable");
		ti->c_cflag &= ~CRTSCTS;
	}

	/* wait for while for controller to setup */
	usleep(10000);

	/* For 8761B Test chip, no patch to download */
	if (rtk_hw_cfg.chip_type == CHIP_8761BTC ||
	    rtk_hw_cfg.chip_type == CHIP_8761B)
		goto done;

	if ((rtk_hw_cfg.total_len > 0) && (rtk_hw_cfg.dl_fw_flag)) {
		rtk_hw_cfg.link_estab_state = H5_PATCH;
		rtk_hw_cfg.rx_index = -1;

		ret =
		    rtk_download_fw_config(fd, rtk_hw_cfg.total_buf,
					   rtk_hw_cfg.total_len,
					   rtk_hw_cfg.baudrate, proto, ti);
		free(rtk_hw_cfg.total_buf);

		if (ret < 0)
			return ret;
	}

done:
	RS_DBG("Init Process finished");
	return 0;
}

/**
* Init uart by realtek Bluetooth.
*
* @param fd uart file descriptor
* @param proto realtek Bluetooth protocol, shall be either HCI_UART_H4 or HCI_UART_3WIRE
* @param speed init_speed in uart struct
* @param ti termios struct
* @returns #0 on success, depend on rtk_config
*/
int rtk_init(int fd, int proto, int speed, struct termios *ti)
{
	struct sigaction sa;
	int retlen;
	RS_DBG("Realtek hciattach version %s \n", RTK_VERSION);

	memset(&rtk_hw_cfg, 0, sizeof(rtk_hw_cfg));
	rtk_hw_cfg.serial_fd = fd;
	rtk_hw_cfg.dl_fw_flag = 1;

	/* h4 will do nothing for init */
	if (proto == HCI_UART_3WIRE) {
		if (rtk_init_h5(fd, ti) < 0)
			return -1;;
	}

	return rtk_config(fd, proto, speed, ti);
}

/**
* Post uart by realtek Bluetooth. If gFinalSpeed is set, set uart speed with it.
*
* @param fd uart file descriptor
* @param proto realtek Bluetooth protocol, shall be either HCI_UART_H4 or HCI_UART_3WIRE
* @param ti termios struct
* @returns #0 on success.
*/
int rtk_post(int fd, int proto, struct termios *ti)
{
	if (rtk_hw_cfg.final_speed)
		return set_speed(fd, ti, rtk_hw_cfg.final_speed);

	return 0;
}

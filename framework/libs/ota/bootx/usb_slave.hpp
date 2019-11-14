#ifndef USB_SLAVE_H
#define USB_SLAVE_H

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#include "storage.h"
#include <libusb.h>
// -----------------------------------------------------------------------------

/*   私有命令定义  */
#define WRITE10           0x2a
#define GET_INFO          0xe0
#define GET_STATUS        0xe1
#define DO_EXIT           0xe2
#define DO_RUN            0xe3

// SCPU 中定义的命令
#define SCPU_SPECIAL_STRING_LEN 128
#define EXCUTE_CMD            0xf0
#define DOWNLOAD_STAGE2       0xf1
#define RUN_STAGE2            0xf2

/*
 * DOWNLOAD_STAGE2 的命令组成为
 * 0x0       : DOWNLOAD_STAGE2 命令
 * 0x1 ~ 0x3 : reserved
 * 0x4 ~ 0x7 : crc result
 * 0x8 ~ 0xb : length
 */

/*
 * EXCUTE_CMD/RUN_STAGE2 命令组成
 * 0x0       : EXCUTE_CMD/RUN_STAGE2
 * 0x1       : not used
 */


/* get info 命令相关数据结构 */
//#define ROM_INFO_HEADER_SIZE 8
#define ROM_INFO_VERSION_V1 0x00000001

#define swab32(x) ((__u32)(             \
    (((__u32)(x) & (__u32)0x000000ffUL) << 24) |        \
    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) |        \
    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) |        \
    (((__u32)(x) & (__u32)0xff000000UL) >> 24)))


#define LEO_VID 0xa700
#define LEO_PID 0x0001
// 我们要接管的 usb interface 的 interface number
#define INTERFACE_NUMBER 0
#define RX_TIMEOUT_MS 5000
#define TX_TIMEOUT_MS 5000

enum {
	DIR_IN  = 0,
	DIR_OUT = 1,
};

using namespace std;

class UsbSlave{
public:
	virtual ~UsbSlave() {};
	virtual int  Open(void) = 0;
	virtual void Close(void) = 0;
	virtual int  GetInfo(void) = 0;
	virtual int  GetStatus(void) = 0;
	virtual int  Write(void *buf, int len) = 0;
	virtual int  SendRun(void) = 0;
	virtual int  SendExit(void) = 0;
	virtual int  DoRx(void *buf, int len) = 0;
	virtual int  DoTx(void *buf, int len) = 0;
	virtual int  DownloadStage2(unsigned char *boot_file_data, int len) = 0;
	virtual int  RunStage2(void) = 0;
	virtual int  ExcuteCmd(const char *cmd) = 0;

	libusb_device        **list;
	libusb_device_handle  *handle;
	unsigned char          ep_in, ep_out;

	void endian_get(void)
	{
		union {
			char c;
			int  i;
		}value;

		value.i = 1;

		if (value.c == 1) {
			endian = ENDIAN_LITTLE;
		} else {
			endian = ENDIAN_BIG;
		}
	}
	inline __u32 le32_to_cpu(__u32 value)
	{
		if (endian == ENDIAN_LITTLE)
			return value;
		else
			return swab32(value);
	}

	inline __u32 cpu_to_le32(__u32 value)
	{
		if (endian == ENDIAN_LITTLE)
			return value;
		else
			return swab32(value);
	}

public:
	char devname[32];
	int endian;
	enum {
		ENDIAN_LITTLE = 0,
		ENDIAN_BIG    = 1,
	};

	struct rom_info_header {
		__le32 info_version;
		__le32 len;
	};

	struct rom_info_v1 {
		__le32 info_version;
		__le32 len;

	};
};


#if defined(__linux) or defined(__APPLE__) or defined(WIN32)
class PosixUsbSlave: public UsbSlave {
public:
	PosixUsbSlave(){
		int rc;

		endian_get();

		handle = NULL;
		list   = NULL;
		ep_in = ep_out = 0;

		rc = libusb_init(NULL);
		if (rc < 0)
			fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
	}
	~PosixUsbSlave() {
		libusb_exit(NULL);
	}

	void Close() {
		if (handle) {
			libusb_release_interface(handle, INTERFACE_NUMBER);
			libusb_close(handle);
			handle = NULL;
		}

		if (list) {
			libusb_free_device_list(list, 1);
			list   = NULL;
		}
	}

	int Open(void)
	{
		libusb_device *device, *found = NULL;
		ssize_t cnt;
		int rc, i;

retry:
		rc = libusb_get_device_list(NULL, &list);
		if (rc < 0) {
			fprintf(stderr, "Error geting device list: %s\n", libusb_error_name(rc));
			return -1;
		}

		cnt = rc;
		for (i = 0; i < cnt; i++) {
			device = list[i];
			if (is_interesting(device)) {
				found = device;
				break;
			}
		}

		if (i == cnt) {
			libusb_free_device_list(list, 1);
			sleep_s(1);
			goto retry;
		}

		//print_devs(list);

		rc = parse_desc(found);
		if (rc != 0) {
			fprintf(stderr, "Error parsing descriptor\n");
			goto _free_dev;
		}

		rc = libusb_open(found, &handle);
		if (rc != 0) {
			fprintf(stderr, "Error opening libusb: %s\n", libusb_error_name(rc));
			goto _free_dev;
		}

		rc = libusb_set_auto_detach_kernel_driver(handle, 1);
#if 0
		// 这个操作在 windows 上会失败，但是不影响使用
		if (rc != 0)
			fprintf(stderr, "Failed to set auto detach kernel driver: %s\n", libusb_error_name(rc));
#endif

		rc = libusb_claim_interface(handle, INTERFACE_NUMBER);
		if (rc != 0) {
			fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(rc));
			goto _close;
		}

		// 前面的配置都成功后，不释放资源并退出
		goto ok;

		// 如果有某一步配置失败，则释放资源
_close:
		libusb_close(handle);
_free_dev:
		libusb_free_device_list(list, 1);
		return rc;

ok:
		return 0;
	}

	int GetInfo(void)
	{
		// 为了保证 buf 起始地址是 4 字节对齐所以没有使用 unsigned char
		unsigned int buf[16];
		unsigned char get_info[2]   = {GET_INFO,   0x00};
		struct rom_info_header rominfo_header;
		scsi_transmission(get_info, sizeof(get_info), DIR_IN, (unsigned char *)buf, sizeof(rominfo_header));
		rominfo_header.info_version = le32_to_cpu(*(unsigned int *)((unsigned char *)buf + 0));
		rominfo_header.len          = le32_to_cpu(*(unsigned int *)((unsigned char *)buf + 4));
		printf("[rom info] info_version : %x\n", rominfo_header.info_version);
		printf("[rom info] len : %d\n", rominfo_header.len);
		scsi_transmission(get_info, sizeof(get_info), DIR_IN, buf, rominfo_header.len);
		switch (rominfo_header.info_version) {
		case ROM_INFO_VERSION_V1:
			{
				struct rom_info_v1 rominfo_v1;
				rominfo_v1.info_version = le32_to_cpu(*(unsigned int *)((unsigned char *)buf + 0));
				rominfo_v1.len          = le32_to_cpu(*(unsigned int *)((unsigned char *)buf + 4));
				printf ("%s, [rom info v1] , info_version : %x, len : %d\n", __func__, rominfo_v1.info_version, rominfo_v1.len);
			}
			break;
		default :
			printf("not support this info version : %x\n", rominfo_header.info_version);
			break;
		}

		return 0;
	}

	int GetStatus(void)
	{
		unsigned int result;
		unsigned char get_status[2] = {GET_STATUS, 0x00};
		int ret;

		ret = scsi_transmission(get_status, sizeof(get_status), DIR_IN, &result, sizeof(unsigned int));
		if (ret != 0)
			return -1;

		return result;
	}

	int Write(void *buf, int len)
	{
		unsigned char write10[10]  = {WRITE10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		return scsi_transmission(write10, sizeof(write10), DIR_OUT, buf, len);
	}

	int SendRun(void)
	{
		unsigned char do_run[2]     = {DO_RUN,     0x00};
		return scsi_transmission(do_run, sizeof(do_run), DIR_OUT, NULL, 0);
	}

	int SendExit(void)
	{
		unsigned char do_exit[2]    = {DO_EXIT,    0x00};
		return scsi_transmission(do_exit, sizeof(do_exit), DIR_OUT, NULL, 0);
	}

	int DoRx(void *buf, int len)
	{
		int transferred, ret;

		ret = libusb_bulk_transfer(handle, ep_in, (unsigned char *)buf, len, &transferred, 0);
		if (ret != 0) {
			printf ("bulk transfer failed, ret : %d\n", ret);
			return -1;
		}

		return transferred;
	}

	int DoTx(void *buf, int len)
	{
		int transferred, ret;

		ret = libusb_bulk_transfer(handle, ep_out, (unsigned char *)buf, len, &transferred, 0);
		if (ret != 0) {
			printf ("bulk transfer failed, ret : %d\n", ret);
			return -1;
		}

		return transferred;
	}

	int DownloadStage2(unsigned char *boot_file_data, int len)
	{
		unsigned char download_stage2[12] = {DOWNLOAD_STAGE2};
		unsigned int crc = 0;
		int i;

		for (i = 0; i < len; i++)
			crc += boot_file_data[i];

		*((unsigned int *)(download_stage2 + 4)) = cpu_to_le32(crc);
		*((unsigned int *)(download_stage2 + 8)) = cpu_to_le32(len);

		//printf ("%s, crc : %x, len : %d\n", __func__, crc, len);
		return scsi_transmission(download_stage2, sizeof(download_stage2), DIR_OUT, boot_file_data, len);
	}

	int RunStage2(void)
	{
		unsigned char run_stage2[2] = {RUN_STAGE2, 0};

		return scsi_transmission(run_stage2, sizeof(run_stage2), DIR_OUT, NULL, 0);
	}

	int ExcuteCmd(const char *cmd)
	{
		unsigned char excute_cmd[2] = {EXCUTE_CMD};

		return scsi_transmission(excute_cmd, sizeof(excute_cmd), DIR_OUT, (void *)cmd, strlen(cmd));
	}

private:

	int scsi_transmission(unsigned char* cmd, uint32_t cmd_len,
			int direction,void *buf, int data_len)
	{
		struct bulk_cb_wrap cbw;
		struct bulk_cs_wrap csw;
		int transferred;
		int ret;

		// cbw
		cbw.Signature = cpu_to_le32(US_BULK_CB_SIGN);
		cbw.DataTransferLength = cpu_to_le32(data_len);
		if (direction == DIR_OUT)
			cbw.Flags = US_BULK_FLAG_OUT;
		else if (direction == DIR_IN)
			cbw.Flags = US_BULK_FLAG_IN;
		cbw.Tag = 0x11223344;
		cbw.Lun = 0;
		cbw.Length = cmd_len;
		memset(cbw.CDB, 0, sizeof(cbw.CDB));
		memcpy(cbw.CDB, cmd, cmd_len);

		// cbw
		ret = libusb_bulk_transfer(handle, ep_out, (unsigned char *)&cbw, sizeof(struct bulk_cb_wrap), &transferred,
				TX_TIMEOUT_MS);
		if (ret != 0 || transferred != sizeof(struct bulk_cb_wrap)) {
			printf ("bulk transfer failed, ret : %d, transferred : %d\n", ret, transferred);
			return -1;
		}

		// data stage
		if (data_len > 0 && buf) {
			// data stage
			ret = libusb_bulk_transfer(handle, direction == DIR_IN ? ep_in : ep_out, (unsigned char *)buf, data_len,
					&transferred, 0);
			if (ret != 0 || transferred != data_len) {
				printf ("bulk transfer failed, ret : %d, transferred : %d\n", ret, transferred);
				return -1;
			}
		}

		// csw
		ret = libusb_bulk_transfer(handle, ep_in, (unsigned char *)&csw, sizeof(struct bulk_cs_wrap), &transferred, RX_TIMEOUT_MS);
		if (ret != 0 || transferred != sizeof(struct bulk_cs_wrap)) {
			printf ("bulk transfer failed, ret : %d, transferred : %d\n", ret, transferred);
			return -1;
		}
		//printf ("csw.Sig : %x\n", csw.Signature);
		//printf ("csw.TAg : %x\n", csw.Tag);
		//printf ("csw.Res : %x\n", csw.Residue);
		//printf ("csw.Status : %x\n", csw.Status);

		return 0;
	}

	int parse_desc(libusb_device *dev) {
		int ret;
		struct libusb_device_descriptor     dev_desc;
		struct libusb_config_descriptor    *config_desc;
		const struct libusb_interface_descriptor *intf_desc;
		const struct libusb_endpoint_descriptor  *ep_desc;
		int i, j;
		int do_debug = 0;

		memset (&dev_desc, 0, sizeof(struct libusb_device_descriptor));
		ret = libusb_get_device_descriptor(dev, &dev_desc);
		if (ret != 0) {
			printf ("Get device descriptor faield\n");
			return -1;
		}

		if (do_debug) {
			printf ("dev bLength            : %d\n", dev_desc.bLength);
			printf ("dev bDescriptorType    : %d\n", dev_desc.bDescriptorType);
			printf ("dev bcdUSB             : %d\n", dev_desc.bcdUSB);
			printf ("dev bDeviceClass       : %d\n", dev_desc.bDeviceClass);
			printf ("dev bDeviceSubClass    : %d\n", dev_desc.bDeviceSubClass);
			printf ("dev bDeviceProtocol    : %d\n", dev_desc.bDeviceProtocol);
			printf ("dev bMaxPacketSize0    : %d\n", dev_desc.bMaxPacketSize0);
			printf ("dev idVendor           : %x\n", dev_desc.idVendor);
			printf ("dev idProduct          : %x\n", dev_desc.idProduct);
			printf ("dev bcdDevice          : %x\n", dev_desc.bcdDevice);
			printf ("dev iManufacturer      : %x\n", dev_desc.iManufacturer);
			printf ("dev iProduct           : %x\n", dev_desc.iProduct);
			printf ("dev iSerialNumber      : %x\n", dev_desc.iSerialNumber);
			printf ("dev bNumConfigurations : %x\n", dev_desc.bNumConfigurations);
		}

		// 现在只解析该 usb 设备的第一个配置
		ret = libusb_get_config_descriptor(dev, 0, &config_desc);
		if (LIBUSB_SUCCESS != ret) {
			printf("Couldn't retrieve descriptors\n");
			return -1;
		}

		if (do_debug) {
			printf ("config desc ----------->\n");
			printf ("conf bLength             : %d\n", config_desc->bLength);
			printf ("conf bDescriptorType     : %d\n", config_desc->bDescriptorType);
			printf ("conf wTotalLength        : %d\n", config_desc->wTotalLength);
			printf ("conf bNumInterfaces      : %d\n", config_desc->bNumInterfaces);
			printf ("conf bConfigurationValue : %d\n", config_desc->bConfigurationValue);
			printf ("conf iConfiguration      : %d\n", config_desc->iConfiguration);
			printf ("conf bmAttributes        : %d\n", config_desc->bmAttributes);
			printf ("conf MaxPower            : %d\n", config_desc->MaxPower);
		}

		// hanlde all interface
		for (i = 0; i < config_desc->bNumInterfaces; i++) {
			int sub_i;

			//printf ("interface %d, have %d altsetting\n", i, config_desc->interface[i].num_altsetting);
			for (sub_i = 0; sub_i < config_desc->interface[i].num_altsetting; sub_i++) {
				intf_desc = config_desc->interface[i].altsetting;

				if (do_debug) {
					printf ("intf desc ----------->\n");
					printf ("intf bLength            : %d\n", intf_desc->bLength);
					printf ("intf bDescriptorType    : %d\n", intf_desc->bDescriptorType);
					printf ("intf bInterfaceNumber   : %d\n", intf_desc->bInterfaceNumber);
					printf ("intf bAlternateSetting  : %d\n", intf_desc->bAlternateSetting);
					printf ("intf bNumEndpoints      : %d\n", intf_desc->bNumEndpoints);
					printf ("intf bInterfaceClass    : %d\n", intf_desc->bInterfaceClass);
					printf ("intf bInterfaceSubClass : %d\n", intf_desc->bInterfaceSubClass);
					printf ("intf bInterfaceProtocol : %d\n", intf_desc->bInterfaceProtocol);
					printf ("intf iInterface         : %d\n", intf_desc->iInterface);
				}

				// handle all ep
				for (j = 0; j < intf_desc->bNumEndpoints; j++) {
					ep_desc = &intf_desc->endpoint[j];

					if (do_debug) {
						printf ("ep desc ----------->\n");
						printf ("ep bLength          : %d\n", ep_desc->bLength);
						printf ("ep bDescriptorType  : %d\n", ep_desc->bDescriptorType);
						printf ("ep bEndpointAddress : %d\n", ep_desc->bEndpointAddress);
						printf ("ep bmAttributes     : %d\n", ep_desc->bmAttributes);
						printf ("ep wMaxPacketSize   : %d\n", ep_desc->wMaxPacketSize);
						printf ("ep bInterval        : %d\n", ep_desc->bInterval);
						printf ("ep bRefresh         : %d\n", ep_desc->bRefresh);
						printf ("ep bSynchAddress    : %d\n", ep_desc->bSynchAddress);
					}

					// 记录第一个 in 和 out endpoint
					if (ep_desc->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
						if (ep_in == 0)
							ep_in = ep_desc->bEndpointAddress;
					} else {
						if (ep_out == 0)
							ep_out = ep_desc->bEndpointAddress;
					}
				}
			}
		}

		libusb_free_config_descriptor(config_desc);

		if (ep_in == 0 || ep_out == 0) {
			printf ("Get endpoint info failed, ep_in : %x, ep_out : %x\n", ep_in, ep_out);
			return -1;
		}
		return 0;
	}

	static int is_interesting(libusb_device *dev)
	{
		struct libusb_device_descriptor desc;
		int r;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "%s, failed to get device descriptor\n", __func__);
			return 0;
		}

		if (desc.idVendor == LEO_VID && desc.idProduct == LEO_PID)
			return 1;

		return 0;
	}

	static int sleep_s(int second) {
#if defined(__linux) or defined(__APPLE__)
		sleep(second);
#elif defined(WIN32)
		Sleep(second * 1000);
#endif
		return 0;
	}
};

#endif

#endif

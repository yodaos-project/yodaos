#ifndef BOOT_H
#define BOOT_H

#include <map>
#include <string>
#include "serial.hpp"
#include "usb_slave.hpp"
#include "os.h"
#include "tftpd.h"

#define BOOTFILES_DIR_NAME     ".bootx"
#define SIZE_8K                8192
#define SIZE_32K               32768

enum transport_mode {
	NONE,
	SERIAL,
	USBSLAVE,
	S1_SERIAL_S2_USBSLAVE,

	S1_NONE_S2_NONE_USBSLAVE,
};

// return value of RunOneCommand
enum {
	RUN_CMD_CONTINUE, // 还可以继续执行命令
	RUN_CMD_STOP,     // 停止执行命令
};

using namespace std;

extern int tftpd(int port);

class Boot {
	public:
		Serial   *serial;
		UsbSlave *usbslave;
		bool extern_bootfile;
		bool enter_debug_mode;
		Boot() {
			if (OSInit() != 0)
				exit(EXIT_FAILURE);
			tftpd_param.port = TFTP_LISTEN_PORT;
			tftpd_start(&tftpd_param);
			serial           = NULL;
			usbslave         = NULL;
			showMessage      = false;
			extern_bootfile  = false;
			transferMode     = SERIAL;
			enter_debug_mode = false;
			BootBinInit();
		}
		~Boot() {
			tftpd_stop(&tftpd_param);
		}

		bool InitResource(void);
		void ReleaseResource(void);

		bool Loader(const char *bootfile);

		void NetConfig(void);
		int  Wait(const char *s, int echo, int timeout=0);
		bool SendData(const void *data, unsigned int len);
		void Command(const char *cmd, int wait);
		int Terminal();

		void Show(bool enable) {
			showMessage = enable;
		}
		void SetTransferMode(enum transport_mode mode) {
			transferMode = mode;
		}
		char *GetMachines(void);
		void BootCommand(const char *dev, const char *mach, const char *cmd);
	private:
		bool showMessage;
		int  transferMode;
		const char *devName;
		struct tftpd_param tftpd_param;

		// 返回命令执行状态
		int RunOneCommand(const char *cmd, int wait);
		const char *cmd_parser(const char *cmd, char *exec_flag);
		void BootBinInit(void);
		int get_boot(const char *name, unsigned char **boot, size_t *size, int *need_free);
		int read_file(const char *name, unsigned char **boot, size_t *size);
		unsigned int CRC32(unsigned char* pBuffer, unsigned int nSize);
	public:
		enum {
			NO_WAIT,
			WAIT
		};

		enum {
			HIDE,
			SHOW
		};
		std::map<string, string> bootlist;
};


#endif

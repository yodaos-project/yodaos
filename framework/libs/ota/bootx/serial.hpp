#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/file.h>
#include <dirent.h>
#endif
#include <errno.h>

#include "os.h"

class Serial {
	public:
		virtual ~Serial() {}
		virtual bool Open(const char *dev) = 0;
		virtual bool Close() = 0;
		virtual bool Config(int speed, int wordSize, int stopBits, char parity) = 0;
		virtual bool setTimeouts(int read_timeout, int write_timeout=0) = 0;
		virtual int Read (void *buf, int size) = 0;
		virtual int BlockRead(void *buf, int size) = 0;
		virtual int Write(const void *buf, int size) = 0;
	public:
		char devname[32];
};

#ifdef WIN32
#include <windows.h>
class WindowSerial: public Serial {
	public:
		WindowSerial(){
			fileHandle = INVALID_HANDLE_VALUE;
			readTimeout=15;
			writeTimeout=0;
		}
		~WindowSerial(){
		}
		bool Open(const char * dev) {
			if (dev != NULL) {
				int PortNum = -1;
				if(sscanf(dev,"COM%d",&PortNum) == 0){
					printf("error: open serial dev name is %s\n", dev);
					return false;
				}
				if(PortNum < 10){
					sprintf(devname, "%s", dev);
				}else{
					sprintf(devname, "\\\\.\\%s", dev);
				}
				fileHandle = OpenSerial(devname);
				if(fileHandle == INVALID_HANDLE_VALUE){
					printf("open serial %s failed, please check.\n", dev);
					return false;
				}
			}

			if (fileHandle == INVALID_HANDLE_VALUE) {
				for (int i = 0; i < 10; i++) {
					sprintf(devname, "COM%d", i);
					fileHandle = OpenSerial(devname);
					if (fileHandle != INVALID_HANDLE_VALUE)
						return true;
				}
				return false;
			} else
				return true;
		}

		bool Close() {
			if(!CloseHandle(fileHandle))
				return false;
			fileHandle = NULL;
			return true;
		}

		bool Config(int speed, int wordSize, int stopBits, char parity) {
			char comSetStr[30];
			sprintf(comSetStr, "%d,%c,%d,%d", speed, parity, wordSize, stopBits);
			DCB dcb;

			FillMemory(&dcb, sizeof(dcb), 0);
			dcb.DCBlength = sizeof(dcb);
			// try to build the DCB
			if (!BuildCommDCB(comSetStr, &dcb)) {
				fprintf(stderr, "Serial config BuildCommDCB error.\n");
				return false;
			}
			// set the state of fileHandle to be dcb
			if(!SetCommState(fileHandle, &dcb)) {
				fprintf(stderr, "Serial config SetCommState error.\n");
				return false;
			}

			// set the buffers to be size 1024 of fileHandle
			if(!SetupComm(fileHandle, 1024, 1024)) {
				fprintf(stderr, "Serial config SetupComm error.\n");
				return false;
			}

			if(!this->setTimeouts(this->readTimeout, this->writeTimeout)) {
				fprintf(stderr, "Serial config setTimeouts error.\n");
				return false;
			}

			return true;
		}

		/*readTimeout、writeTimeout单位为秒*/
		bool setTimeouts(int readTimeout, int writeTimeout) {
			COMMTIMEOUTS cmt;
			readTimeout = readTimeout*1000;
			writeTimeout = writeTimeout*1000;
			this->readTimeout = readTimeout;
			this->writeTimeout = writeTimeout;

			cmt.ReadIntervalTimeout = 0;
			cmt.ReadTotalTimeoutMultiplier = 10;
			cmt.ReadTotalTimeoutConstant = readTimeout;

			cmt.WriteTotalTimeoutConstant = writeTimeout;
			cmt.WriteTotalTimeoutMultiplier = writeTimeout;

			if(!SetCommTimeouts(fileHandle, &cmt))
			{
				return false;
			}
			return true;
		}
		int Read(void *buf, int size) {
			DWORD read = 0;
			ReadFile(fileHandle, buf, size, &read, NULL);
			return read;
		}
		int BlockRead(void *buf, int size) {
			DWORD len = 0;
			DWORD act_len = 0;
			while(size){
				ReadFile(fileHandle, (void *)((unsigned int)buf + act_len), size, &len, NULL);
				size -= len;
				act_len += len;
			}
			return act_len;
		}
		int Write(const void *buf, int size) {
			DWORD write = 0;
			WriteFile(fileHandle, buf, size, &write, NULL);
			return write;
		}

	private:
		HANDLE fileHandle;
		int readTimeout;
		int writeTimeout;
		HANDLE OpenSerial(const char *dev) {
			return CreateFile(dev, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
		}
};
#elif defined(linux) or defined(__APPLE__)
#include <termios.h>
#include <sys/types.h>
#include <signal.h>

class PosixSerial: public Serial {
	public:
		PosixSerial() {
			fd = -1;
		}
		~PosixSerial() {
			Close();
		}
		bool Open(const char* dev) {
			if (dev != NULL) {
				strcpy(devname, dev);
				fd = OpenSerial(dev);
				if(fd < 0){
					printf("open serial %s failed, please check.\n", dev);
					return false;
				}
			}

			if (fd < 0) {
				for (int i = 0; i < 10; i++) {
					sprintf(devname, "/dev/ttyUSB%d", i);
					fd = OpenSerial(devname);

					if (fd == -2)
						return false;
					else if (fd > 0)
						return true;

					sprintf(devname, "/dev/ttyS%d", i);
					fd = OpenSerial(devname);

					if (fd == -2)
						return false;
					else if (fd > 0)
						return true;
				}
				return false;
			}else
				return true;
		}

		bool Close() {
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			return true;
		}

		bool Config(int speed, int databits, int stopBits, char parity) {
			int speed_arr[] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
			int name_arr[]  = {  115200,  57600,  38400,  19200,  9600,  4800,  2400,  1200,  300};

			struct termios opt;

			/* waits until all output written to the object referred to by fd has been transmitted */
			tcdrain(fd);

			/* flushes both data received but not read, and data written but not transmitted. */
			tcflush(fd, TCIOFLUSH);

			if (tcgetattr(fd, &opt) != 0) {
				fprintf(stderr, "Serial config tcgetattr error.\n");
				return false;
			}

			// set speed
			for (int i= 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++) {
				if  (speed == name_arr[i]) {
					if (cfsetispeed(&opt, speed_arr[i]) != 0)
						fprintf(stderr, "Serial config cfsetispeed error\n");
					if (cfsetospeed(&opt, speed_arr[i]) != 0)
						fprintf(stderr, "Serial config cfsetospeed error\n");
					break;
				}
			}

			opt.c_cflag &= ~CSIZE;
			switch (databits) { /*设置数据位数*/
				case 7:
					opt.c_cflag |= CS7;
					break;
				case 8:
					opt.c_cflag |= CS8;
					break;
				default:
					fprintf(stderr,"Unsupported data size\n");
					return false;
			}
			switch (parity) {
				case 'n':
				case 'N':
					opt.c_cflag &= ~PARENB;           /* Clear parity enable */
					opt.c_iflag &= ~INPCK;            /* Enable parity checking */
					break;
				case 'o':
				case 'O':
					opt.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
					opt.c_iflag |= INPCK;             /* Disnable parity checking */
					break;
				case 'e':
				case 'E':
					opt.c_cflag |= PARENB;            /* Enable parity */
					opt.c_cflag &= ~PARODD;           /* 转换为偶效验*/
					opt.c_iflag |= INPCK;             /* Disnable parity checking */
					break;
				case 'S':
				case 's':  /*as no parity*/
					opt.c_cflag &= ~PARENB;
					opt.c_cflag &= ~CSTOPB;break;
				default:
					fprintf(stderr,"Unsupported parity\n");
					return false;
			}

			/* 设置停止位*/
			switch (stopBits) {
				case 1:
					opt.c_cflag &= ~CSTOPB;
					break;
				case 2:
					opt.c_cflag |= CSTOPB;
					break;
				default:
					fprintf(stderr,"Unsupported stop bits\n");
					return false;
			}
			/* Set input parity option */
			if (parity != 'n')
				opt.c_iflag |= INPCK;

			opt.c_cc[VTIME] = 150; /* 设置超时 15 seconds          */
			opt.c_cc[VMIN] = 0;    /* Update the opt and do it NOW */

			opt.c_cflag &= ~CRTSCTS;
			opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Input  */
			opt.c_oflag &= ~OPOST;                           /* Output */
			opt.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXON);

			if (tcsetattr(fd, TCSANOW, &opt) != 0) {
				fprintf(stderr, "Serial config tcsetattr error.\n");
				return false;
			}

			return true;
		}

		/*readTimeout、writeTimeout单位为秒*/
		bool setTimeouts(int readTimeout, int writeTimeout) {
			struct termios opt;
			readTimeout=readTimeout*10;
			writeTimeout=writeTimeout*10;
			if (tcgetattr(fd, &opt) == -1)
			{
				return false;
			}
			opt.c_cc[VTIME] = readTimeout; /* 设置超时*/
			opt.c_cc[VMIN] = 0;    /* Update the opt and do it NOW */
			if (tcsetattr(fd, TCSANOW, &opt) != 0)
			{
				return false;
			}
			return true;
		}

		int Read(void *buf, int size) {
			return read(fd, buf, size);
		}

		int BlockRead(void *buf, int size) {
			int len = 0;
			int act_len = 0;
			while(size) {
				len = read(fd, (void *)((char*)buf + act_len), size);
				size -= len;
				act_len += len;
			}
			return act_len;
		}

		int Write(const void *buf, int size) {
			return write(fd, buf, size);
		}

	private:
		int fd;
		int OpenSerial(const char *dev) {
			struct termios opt;
			char command[256];
			unsigned int pid;

			while (lsof(command, &pid, 256, dev) == true) {
				printf("%s used by %s, Force close? (Y/n)", dev, command);
				char c = getchar();
				if (c == 'y' || c == 'Y' || c == 10)
					kill(pid, 9);
				else
					return -2;
			}
			int fd = open(dev, O_RDWR);

			if (tcgetattr(fd, &opt) == -1) {
				close(fd);
				fd = -1;
			}
			if (fd > 0) {
			}

			return fd;
		}

		bool lsof(char *command, unsigned int *rpid, size_t size, const char *dev) {
			DIR *dir;
			struct dirent *entry;
			unsigned int pid;
			bool found = false;

			dir = opendir("/proc");
			if (dir == NULL)
				return false;

			while((entry = readdir(dir)) != NULL) {
				struct dirent *fd_entry;
				char name[sizeof("/proc/%u/fd/0123456789") + sizeof(int)*3];
				unsigned baseofs;
				char *fdlink;
				DIR *fd_dir;

				// is process
				pid = bb_strtou(entry->d_name, NULL, 10);
				if (errno)
					continue;

				baseofs = sprintf(name, "/proc/%u/fd/", pid);
				fd_dir = opendir(name);
				if (fd_dir == NULL)
					continue;

				while((fd_entry = readdir(fd_dir)) != NULL) {
					bb_strtou(fd_entry->d_name, NULL, 10);
					if (errno)
						continue;

					strncpy(name + baseofs, fd_entry->d_name, 10);
					fdlink = xmalloc_readlink(name);
					if (fdlink == NULL)
						continue;

					if (strcasecmp(fdlink, dev) == 0) {
						read_cmdline(command, size, pid);
						if (rpid)
							*rpid = pid;
						found = true;

						free(fdlink);
						goto out;
					}
					free(fdlink);
				}
				closedir(fd_dir);
			}
out:
			closedir(dir);

			return found;
		}

		void read_cmdline(char *buf, size_t size, unsigned int pid)
		{
			int sz;
			char filename[sizeof("/proc/%u/cmdline") + sizeof(int)*3];

			sprintf(filename, "/proc/%u/cmdline", pid);
			sz = open_read_close(filename, buf, size - 1);
			if (sz > 0) {
				char *p = NULL;
				buf[sz] = '\0';
				while (--sz >= 0 && buf[sz] == '\0')
					continue;
				/* Prevent basename("process foo/bar") = "bar" */
				p = strchr(buf, ' ');
				if(p == NULL)
					return;
				else
					*p = '\0';
			}
		}

};
#else
#error "you must defined one of WIN32/linux/__APPLE__"
#endif

#define BOOT_DEFAULT_BAUDRATE    115200
void save_rom_baudrate(unsigned int baudrate);
unsigned int get_rom_baudrate(void);

#endif


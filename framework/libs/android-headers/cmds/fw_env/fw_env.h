/*
 * (C) Copyright 2002-2008
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To build the utility with the static configuration
 * comment out the next line.
 * See included "fw_env.config" sample file
 * for notes on configuration.
 */
#define CONFIG_FILE     "/etc/fw_env.config"

#define HAVE_REDUND /* For systems with 2 env sectors */
#define DEVICE1_NAME      "/dev/mtd1"
#define DEVICE2_NAME      "/dev/mtd2"
#define DEVICE1_OFFSET    0x0000
#define ENV1_SIZE         0x4000
#define DEVICE1_ESIZE     0x4000
#define DEVICE1_ENVSECTORS     2
#define DEVICE2_OFFSET    0x0000
#define ENV2_SIZE         0x4000
#define DEVICE2_ESIZE     0x4000
#define DEVICE2_ENVSECTORS     2

//ROKID CONFIG for pebble C(amlogic 912)
#define CONFIG_EXTRA_ENV_SETTINGS \
        "firstboot=0\0"\
        "upgrade_step=0\0"\
        "jtag=apee\0"\
        "loadaddr=1080000\0"\
        "outputmode=1080p60hz\0" \
        "hdmimode=1080p60hz\0" \
        "cvbsmode=576cvbs\0" \
        "display_width=1920\0" \
        "display_height=1080\0" \
        "display_bpp=16\0" \
        "display_color_index=16\0" \
        "display_layer=osd1\0" \
        "display_color_fg=0xffff\0" \
        "display_color_bg=0\0" \
        "dtb_mem_addr=0x1000000\0" \
        "fb_addr=0x3d800000\0" \
        "fb_width=1920\0" \
        "fb_height=1080\0" \
        "usb_burning=update 1000\0" \
        "fdt_high=0x20000000\0"\
        "try_auto_burn=update 700 750;\0"\
        "sdcburncfg=aml_sdc_burn.ini\0"\
        "sdc_burning=sdc_burn ${sdcburncfg}\0"\
        "wipe_data=successful\0"\
        "wipe_cache=successful\0"\
        "EnableSelinux=enforcing\0"\
        "recovery_part=recovery\0"\
        "recovery_offset=0\0"\
        "cvbs_drv=0\0"\
        "initargs="\
            "rootfstype=ramfs init=/init console=ttyS0,115200 no_console_suspend earlyprintk=aml-uart,0xc81004c0 ramoops.pstore_en=1 ramoops.record_size=0x8000 ramoops.console_size=0x4000 "\
            "\0"\
        "upgrade_check="\
            "echo upgrade_step=${upgrade_step}; "\
            "if itest ${upgrade_step} == 3; then "\
                "run init_display; run storeargs; run update;"\
            "else fi;"\
            "\0"\
    "storeargs="\
            "setenv bootargs ${initargs} androidboot.selinux=${EnableSelinux} logo=${display_layer},loaded,${fb_addr},${outputmode} maxcpus=${maxcpus} vout=${outputmode},enable hdmimode=${hdmimode} cvbsmode=${cvbsmode} hdmitx=${cecconfig} cvbsdrv=${cvbs_drv} androidboot.firstboot=${firstboot} jtag=${jtag}; "\
	"setenv bootargs ${bootargs} androidboot.serialno=${serialno} androidboot.rokidseed=${rokidseed} androidboot.hardware=amlogic;"\
	"setenv bootargs ${bootargs} boardinfo.hardwareid=${hardwareid};"\
            "run cmdline_keys;"\
            "\0"\
        "switch_bootmode="\
            "get_rebootmode;"\
            "if test ${reboot_mode} = factory_reset; then "\
                    "run recovery_from_flash;"\
            "else if test ${reboot_mode} = update; then "\
                    "run update;"\
            "else if test ${reboot_mode} = cold_boot; then "\
                /*"run try_auto_burn; "*/\
            "fi;fi;fi;"\
            "\0" \
        "storeboot="\
            "if imgread kernel boot ${loadaddr}; then bootm ${loadaddr}; fi;"\
            "run update;"\
            "\0"\
        "factory_reset_poweroff_protect="\
            "echo wipe_data=${wipe_data}; echo wipe_cache=${wipe_cache};"\
            "if test ${wipe_data} = failed; then "\
                "run init_display; run storeargs;"\
                "if mmcinfo; then "\
                    "run recovery_from_sdcard;"\
                "fi;"\
                "if usb start 0; then "\
                    "run recovery_from_udisk;"\
                "fi;"\
                "run recovery_from_flash;"\
            "fi; "\
            "if test ${wipe_cache} = failed; then "\
                "run init_display; run storeargs;"\
                "if mmcinfo; then "\
                    "run recovery_from_sdcard;"\
                "fi;"\
                "if usb start 0; then "\
                    "run recovery_from_udisk;"\
                "fi;"\
                "run recovery_from_flash;"\
            "fi; \0" \
         "update="\
            /*first usb burning, second sdc_burn, third ext-sd autoscr/recovery, last udisk autoscr/recovery*/\
            "run usb_burning; "\
            "run sdc_burning; "\
            "if mmcinfo; then "\
                "run recovery_from_sdcard;"\
            "fi;"\
            "if usb start 0; then "\
                "run recovery_from_udisk;"\
            "fi;"\
            "run recovery_from_flash;"\
            "\0"\
        "recovery_from_sdcard="\
            "setenv bootargs ${bootargs} aml_dt=${aml_dt} recovery_part={recovery_part} recovery_offset={recovery_offset};"\
            "if fatload mmc 0 ${loadaddr} aml_autoscript; then autoscr ${loadaddr}; fi;"\
            "if fatload mmc 0 ${loadaddr} recovery.img; then "\
                    "if fatload mmc 0 ${dtb_mem_addr} dtb.img; then echo sd dtb.img loaded; fi;"\
                    "wipeisb; "\
                    "bootm ${loadaddr};fi;"\
            "\0"\
        "recovery_from_udisk="\
            "setenv bootargs ${bootargs} aml_dt=${aml_dt} recovery_part={recovery_part} recovery_offset={recovery_offset};"\
            "if fatload usb 0 ${loadaddr} aml_autoscript; then autoscr ${loadaddr}; fi;"\
            "if fatload usb 0 ${loadaddr} recovery.img; then "\
                "if fatload usb 0 ${dtb_mem_addr} dtb.img; then echo udisk dtb.img loaded; fi;"\
                "wipeisb; "\
                "bootm ${loadaddr};fi;"\
            "\0"\
        "recovery_from_flash="\
            "setenv bootargs ${bootargs} aml_dt=${aml_dt} recovery_part={recovery_part} recovery_offset={recovery_offset};"\
            "if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then wipeisb; bootm ${loadaddr}; fi"\
            "\0"\
        "init_display="\
            "osd open;osd clear;imgread pic logo bootup $loadaddr;bmp display $bootup_offset;bmp scale"\
            "\0"\
        "cmdline_keys="\
            "if keyman init 0x1234; then "\
                "if keyman read usid ${loadaddr} str; then "\
                    "setenv bootargs ${bootargs} androidboot.usid=${usid};"\
                "fi;"\
            "fi;"\
            "\0"\
        "upgrade_key="\
            "if gpio input GPIOAO_2; then "\
                "echo detect upgrade key; sleep 3;"\
                "if gpio input GPIOAO_2; then run update; fi;"\
            "fi;"\
            "\0"\

extern int   fw_printenv(int argc, char *argv[]);
extern char *fw_getenv  (char *name);
extern int fw_setenv  (int argc, char *argv[]);
extern int fw_parse_script(char *fname);
extern int fw_env_open(void);
extern int fw_env_write(char *name, char *value);
extern int fw_env_close(void);

extern unsigned	long  crc32	 (unsigned long, const unsigned char *, unsigned);

typedef unsigned long ulong;
//typedef unsigned int  uint32_t;


/*
 * (C) Copyright 2008-2013
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 * 	on behalf of ifm electronic GmbH
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <pthread.h>
#include "cpiohdr.h"

#include "bsdqueue.h"
#include "swupdate.h"
#include "util.h"
#include "handler.h"
#ifdef CONFIG_MTD
#include "flash.h"
#endif
#include "parsers.h"
#include "network_ipc.h"
#include "network_interface.h"
#include "mongoose_interface.h"
#include "installer.h"
#include "progress.h"
#include "pctl.h"
#include "bootloader.h"

#define BUFF_SIZE	 4096
#define PERCENT_LB_INDEX	4
#define CPIO_BLOCK 512

enum {
	STREAM_WAIT_DESCRIPTION,
	STREAM_WAIT_SIGNATURE,
	STREAM_DATA,
	STREAM_END
};

static pthread_t network_thread_id;

/*
 * NOTE: these sync vars are _not_ static, they are _shared_ between the
 * installer, the display thread and the network thread
 *
 * 'mutex' protects the 'inst' installer data, the 'cond' variable as
 * well as the 'inst.mnu_main' close request; 'cond' signals the
 * reception of an install request
 *
 */

pthread_mutex_t stream_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stream_wkup = PTHREAD_COND_INITIALIZER;

static struct installer inst;

static int extract_file_to_tmp(int fd, const char *fname, unsigned long *poffs)
{
	char output_file[MAX_IMAGE_FNAME];
	struct filehdr fdh;
	int fdout;
	uint32_t checksum;

	if (extract_cpio_header(fd, &fdh, poffs)) {
		return -1;
	}
	if (strcmp(fdh.filename, fname)) {
		TRACE("description file name not the first of the list: %s instead of %s",
			fdh.filename,
			fname);
		return -1;
	}
	if (snprintf(output_file, sizeof(output_file), "%s%s", TMPDIR,
		     fdh.filename) >= (int)sizeof(output_file)) {
		ERROR("Path too long: %s%s", TMPDIR, fdh.filename);
		return -1;
	}
	TRACE("Found file:\n\tfilename %s\n\tsize %u", fdh.filename, (unsigned int)fdh.size);

	fdout = openfileoutput(output_file);
	if (fdout < 0)
		return -1;

	if (copyfile(fd, &fdout, fdh.size, poffs, 0, 0, 0, &checksum, NULL, 0, NULL) < 0) {
		close(fdout);
		return -1;
	}
	if (checksum != (uint32_t)fdh.chksum) {
		close(fdout);
		ERROR("Checksum WRONG ! Computed 0x%ux, it should be 0x%ux\n",
			(unsigned int)checksum, (unsigned int)fdh.chksum);
			return -1;
	}
	close(fdout);

	return 0;
}

static int save_cpio_header(int fdin, int fdout, unsigned long *offset, struct filehdr *fdh)
{
	unsigned char buf[256] = {0};
	int header_len = sizeof(struct new_ascii_header);

	if (fill_buffer(fdin, buf, header_len, offset, NULL, NULL) < 0)
		return -EINVAL;

	if (get_cpiohdr(buf, &fdh->size, &fdh->namesize, &fdh->chksum) < 0) {
		ERROR("CPIO Header corrupted, cannot be parsed\n");
		return -EINVAL;
	}

	if (write(fdout, buf, header_len) != header_len)
		return -EIO;

	return 0;
}

static int save_file_name(int fdin, int fdout, unsigned long *offset, struct filehdr *fdh)
{
	int padding = 0;
	unsigned char buf[256] = {0};

	if (fdh->namesize >= sizeof(fdh->filename))
	{
		ERROR("CPIO Header filelength too big %u >= %u (max)",
			(unsigned int)fdh->namesize,
			(unsigned int)sizeof(fdh->filename));
		return -EINVAL;
	}

	if (fill_buffer(fdin, buf, fdh->namesize, offset, NULL, NULL) < 0)
		return -EINVAL;
	strncpy(fdh->filename, (char *)buf, sizeof(fdh->filename));
	TRACE("Found file:\n\tfilename %s\n\tsize %u", fdh->filename, (unsigned int)fdh->size);

	if (fdh->namesize != write(fdout, buf, fdh->namesize))
		return -EIO;

	// write file name padding, if any
	padding = (4 - (*offset % 4)) % 4;
	if (padding) {
		if (fill_buffer(fdin, buf, padding, offset, NULL, NULL) < 0)
			return -EINVAL;
		if (padding != write(fdout, buf, padding))
			return -EIO;
	}

	return 0;
}


static int save_file(int fdin, int fdout, unsigned long *offset, struct filehdr *fdh)
{
	unsigned long size;
	unsigned long nbytes = fdh->size;
	unsigned char *buf = NULL;
	int padding = 0;

	buf = (unsigned char *)malloc(BUFF_SIZE);
	if (!buf)
		return -ENOMEM;

	while (nbytes > 0) {
		size = (nbytes < BUFF_SIZE ? nbytes : BUFF_SIZE);

		if (fill_buffer(fdin, buf, size, offset, NULL, NULL) < 0)
			goto savefile_error;

		if (size != write(fdout, buf, size))
			goto savefile_error;

		nbytes -= size;
	}

	// write file padding, if any
	if (strcmp("TRAILER!!!", fdh->filename) == 0)
		padding = (CPIO_BLOCK - (*offset % CPIO_BLOCK)) % CPIO_BLOCK;
	else
		padding = (4 - (*offset % 4)) % 4;

	if (padding) {
		if (fill_buffer(fdin, buf, padding, offset, NULL, NULL) < 0)
			goto savefile_error;
		if (padding != write(fdout, buf, padding))
			goto savefile_error;
	}

	free(buf);
	return 0;

savefile_error:
	if (buf)
		free(buf);
	return -1;
}

static int save_files(int fd, struct swupdate_cfg *software)
{
	int fdout;
	struct filehdr fdh;
	unsigned long offset = 0;

	fdout = openfileoutput(software->filesave_dst);
	if (fdout < 0)
		return -1;

	for (;;) {
		memset(&fdh, 0, sizeof(struct filehdr));

		if (save_cpio_header(fd, fdout, &offset, &fdh) != 0)
			goto error;

		if (save_file_name(fd, fdout, &offset, &fdh) != 0)
			goto error;

		if (save_file(fd, fdout, &offset, &fdh) != 0)
			goto error;

		if (strcmp("TRAILER!!!", fdh.filename) == 0)
			break;
	}

	close(fdout);
	return 0;

error:
	close(fdout);
	unlink(software->filesave_dst);
	return -1;
}

static int check_saved_files(struct swupdate_cfg *swcfg)
{
	int fdsw;
	off_t pos;

	fdsw = open(swcfg->filesave_dst, O_RDONLY);
	if (fdsw < 0) {
		ERROR("Image Software cannot be read...!\n");
		return -1;
	}

	pos = extract_sw_description(fdsw, SW_DESCRIPTION_FILENAME, 0);
#ifdef CONFIG_SIGNED_IMAGES
	pos = extract_sw_description(fdsw, SW_DESCRIPTION_FILENAME ".sig",
		pos);
#endif

	if (parse(swcfg, TMPDIR SW_DESCRIPTION_FILENAME)) {
		ERROR("failed to parse " SW_DESCRIPTION_FILENAME "!\n");
		goto check_error;
	}

	if (check_hw_compatibility(swcfg)) {
		ERROR("SW not compatible with hardware\n");
		goto check_error;
	}

	if (cpio_scan(fdsw, swcfg, pos) < 0) {
		ERROR("failed to scan for pos '%ld'!", pos);
		goto check_error;
	}

	close(fdsw);
	return 0;

check_error:
	close(fdsw);
	unlink(swcfg->filesave_dst);
	return -1;
}

static int extract_files(int fd, struct swupdate_cfg *software)
{
	int status = STREAM_WAIT_DESCRIPTION;
	unsigned long offset;
	struct filehdr fdh;
	int skip;
	uint32_t checksum;
	int fdout;
	struct img_type *img, *part;
	char output_file[MAX_IMAGE_FNAME];


	/* preset the info about the install parts */

	offset = 0;

#ifdef CONFIG_UBIVOL
	mtd_init();
	ubi_init();
#endif

	for (;;) {
		switch (status) {
		/* Waiting for the first Header */
		case STREAM_WAIT_DESCRIPTION:
			if (extract_file_to_tmp(fd, SW_DESCRIPTION_FILENAME, &offset) < 0 )
				return -1;

			status = STREAM_WAIT_SIGNATURE;
			break;

		case STREAM_WAIT_SIGNATURE:
#ifdef CONFIG_SIGNED_IMAGES
			snprintf(output_file, sizeof(output_file), "%s.sig", SW_DESCRIPTION_FILENAME);
			if (extract_file_to_tmp(fd, output_file, &offset) < 0 )
				return -1;
#endif
			snprintf(output_file, sizeof(output_file), "%s%s", TMPDIR, SW_DESCRIPTION_FILENAME);
			if (parse(software, output_file)) {
				ERROR("Compatible SW not found");
				return -1;
			}

			if (check_hw_compatibility(software)) {
				ERROR("SW not compatible with hardware\n");
				return -1;
			}
			status = STREAM_DATA;
			break;

		case STREAM_DATA:
			if (extract_cpio_header(fd, &fdh, &offset)) {
				ERROR("CPIO HEADER");
				return -1;
			}
			if (strcmp("TRAILER!!!", fdh.filename) == 0) {
				status = STREAM_END;
				break;
			}

			skip = check_if_required(&software->images, &fdh,
						&software->installed_sw_list,
						&img);
			if (skip == SKIP_FILE) {
				/*
				 *  Check for script, but scripts are not checked
				 *  for version
				 */
				skip = check_if_required(&software->scripts, &fdh,
								NULL,
								&img);
			}
			TRACE("Found file:\n\tfilename %s\n\tsize %d %s",
				fdh.filename,
				(unsigned int)fdh.size,
				(skip == SKIP_FILE ? "Not required: skipping" : "required"));

			fdout = -1;
			offset = 0;

			/*
			 * If images are not streamed directly into the target
			 * copy them into TMPDIR to check if it is all ok
			 */
			switch (skip) {
			case COPY_FILE:
				fdout = openfileoutput(img->extract_file);
				if (fdout < 0)
					return -1;
				if (copyfile(fd, &fdout, fdh.size, &offset, 0, 0, 0, &checksum, img->sha256, 0, NULL) < 0) {
					close(fdout);
					return -1;
				}
				if (checksum != (unsigned long)fdh.chksum) {
					ERROR("Checksum WRONG ! Computed 0x%ux, it should be 0x%ux",
						(unsigned int)checksum, (unsigned int)fdh.chksum);
					close(fdout);
					return -1;
				}
				close(fdout);
				break;

			case SKIP_FILE:
				if (copyfile(fd, &fdout, fdh.size, &offset, 0, skip, 0, &checksum, NULL, 0, NULL) < 0) {
					return -1;
				}
				if (checksum != (unsigned long)fdh.chksum) {
					ERROR("Checksum WRONG ! Computed 0x%ux, it should be 0x%ux",
						(unsigned int)checksum, (unsigned int)fdh.chksum);
					return -1;
				}
				break;
			case INSTALL_FROM_STREAM:
				TRACE("Installing STREAM %s, %lld bytes\n", img->fname, img->size);
				/*
				 * If we are streaming data to store in a UBI volume, make
				 * sure that the UBI partitions are adjusted beforehand
				 */
				LIST_FOREACH(part, &software->images, next) {
					if ( (!part->install_directly)
						&& (!strcmp(part->type, "ubipartition")) ) {
						TRACE("Need to adjust partition %s before streaming %s",
							part->volname, img->fname);
						if (install_single_image(part)) {
							ERROR("Error adjusting partition %s", part->volname);
							return -1;
						}
						/* Avoid trying to adjust again later */
						part->install_directly = 1;
					}
				}
				img->fdin = fd;
				if (install_single_image(img)) {
					ERROR("Error streaming %s", img->fname);
					return -1;
				}
				TRACE("END INSTALLING STREAMING");
				break;
			}

			break;

		case STREAM_END:

			/*
			 * Check if all required files were provided
			 * Update of a single file is not possible.
			 */

			LIST_FOREACH(img, &software->images, next) {
				if (! img->required)
					continue;
				if (! img->fname[0])
					continue;
				if (! img->provided) {
					ERROR("Required image file %s missing...aborting !",
						img->fname);
					return -1;
				}
			}
			return 0;
		default:
			return -1;
		}
	}
}

void *network_initializer(void *data)
{
	int ret;
	struct swupdate_cfg *software = data;

	/* No installation in progress */
	memset(&inst, 0, sizeof(inst));
	inst.fd = -1;
	inst.status = IDLE;

	/* fork off the local dialogs and network service */
	network_thread_id = start_thread(network_thread, &inst);

	/* handle installation requests (from either source) */
	while (1) {

		printf ("Main loop Daemon\n");

		/* wait for someone to issue an install request */
		pthread_mutex_lock(&stream_mutex);
		pthread_cond_wait(&stream_wkup, &stream_mutex);
		inst.status = RUN;
		pthread_mutex_unlock(&stream_mutex);
		notify(START, RECOVERY_NO_ERROR, "Software Update started !");

		if (software->globals.filesave) {
			/* save files directly */
			bootloader_env_set("upgrade_status", "downloading");
			if (save_files(inst.fd, software) == 0)
				if (check_saved_files(software) == 0)
					inst.last_install = SUCCESS;
				else
					inst.last_install = FAILURE;
			else
				inst.last_install = FAILURE;

			if (inst.last_install == SUCCESS) {
				bootloader_env_set("upgrade_status", "available");
				notify(SUCCESS, RECOVERY_NO_ERROR, "SWUPDATE download successful!");
			} else {
				bootloader_env_unset("upgrade_status");
				notify(FAILURE, RECOVERY_ERROR, "Image invalid or corrupted. Not download ...");
			}

			close(inst.fd);
		} else if (software->globals.checked) {
			inst.last_install = SUCCESS;
			bootloader_env_set("upgrade_status", "available");
		} else {
#ifdef CONFIG_MTD
			mtd_cleanup();
			scan_mtd_devices();
#endif
			/*
			 * extract the meta data and relevant parts
			 * (flash images) from the install image
			 */
			ret = extract_files(inst.fd, software);
			close(inst.fd);

			/* do carry out the installation (flash programming) */
			if (ret == 0) {
				TRACE("Valid image found: copying to FLASH");

				/*
				 * If an image is loaded, the install
				 * must be successful. Set we have
				 * initiated an update
				 */
				bootloader_env_set("recovery_status", "in_progress");

				notify(RUN, RECOVERY_NO_ERROR, "Installation in progress");
				ret = install_images(software, 0, 0);
				if (ret != 0) {
					bootloader_env_set("recovery_status", "failed");
					notify(FAILURE, RECOVERY_ERROR, "Installation failed !");
					inst.last_install = FAILURE;

				} else {
					/*
					 * Clear the recovery variable to indicate to bootloader
					 * that it is not required to start recovery again
					 */
					bootloader_env_unset("recovery_status");
					notify(SUCCESS, RECOVERY_NO_ERROR, "SWUPDATE successful !");
					inst.last_install = SUCCESS;
				}
			} else {
				inst.last_install = FAILURE;
				notify(FAILURE, RECOVERY_ERROR, "Image invalid or corrupted. Not installing ...");
			}
		}
		swupdate_progress_end(inst.last_install);

		pthread_mutex_lock(&stream_mutex);
		inst.status = IDLE;
		pthread_mutex_unlock(&stream_mutex);
		TRACE("Main thread sleep again !");
		notify(IDLE, RECOVERY_NO_ERROR, "Waiting for requests...");

		/* release temp files we may have created */
		cleanup_files(software);
	}

	pthread_exit((void *)0);
}

/*
 * Retrieve additional info sent by the source
 * The data is not locked because it is retrieve
 * at different times
 */
int get_install_info(sourcetype *source, char *buf, size_t len)
{
	len = min(len, inst.len);

	memcpy(buf, inst.info, len);
	*source = inst.source;

	return len;
}


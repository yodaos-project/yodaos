/*
 * Copyright (C) 2016 Red Hat Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>

#define CONFIGFS "/sys/kernel/config"
#define GADGET   CONFIGFS "/usb_gadget"
#define GADGET1  GADGET "/g1"
#define VENDOR   "0x18d1"
#define PRODUCT  "0x4E26"

static int
error (const char *msg)
{
	g_printerr ("Could not start adb: %s\n", msg);
	return 1;
}

static int
write_content (const char *filename,
	       const char *content)
{
	int fd, ret;

	fd = open (filename, O_WRONLY);
	if (fd < 0)
		return fd;
	ret = write (fd, content, strlen (content));
	close (fd);
	return ret;
}

/* Based upon
 * https://wiki.linaro.org/LMG/Kernel/AndroidConfigFSGadgets#ADB_gadget_configuration */
static int
start (void)
{
	char *serial;

	/* Create gadget */
	if (!g_file_test (GADGET, G_FILE_TEST_IS_DIR)) /* no usb gadget framework */
                return 0;
	if (g_mkdir_with_parents (GADGET1, 0755) < 0)
		return error ("failed to create gadget directory");
	/* Set default Vendor and Product IDs */
	if (write_content (GADGET1 "/idVendor", VENDOR) < 0)
		return error ("failed to write vendor");
	if (write_content (GADGET1 "/idProduct", PRODUCT) < 0)
		return error ("failed to write product");
	/* Create English strings and add random deviceID */
	if (g_mkdir_with_parents (GADGET1 "/strings/0x409", 0755) < 0)
		return error ("failed to write English strings");
	if (!g_file_get_contents ("/etc/machine-id", &serial, NULL, NULL))
		return error ("failed to read machine ID");
	if (write_content (GADGET1 "/strings/0x409/serialnumber", serial) < 0)
		return error ("failed to write serial number");
	g_free (serial);
	/* Product info in English */
	if (write_content (GADGET1 "/strings/0x409/manufacturer", "Freedesktop.org") < 0)
		return error ("failed to write manufacturer");
	if (write_content (GADGET1 "/strings/0x409/product", "adbd gadget") < 0)
		return error ("failed to write product");
	/* Create gadget configuration */
	if (g_mkdir_with_parents (GADGET1 "/configs/c.1/strings/0x409", 0755) < 0)
		return error ("failed to create config1 dir");
	if (write_content (GADGET1 "/configs/c.1/strings/0x409/configuration", "adb") < 0)
		return error ("failed to name configuration");
	//FIXME MaxPower
	/* Create Adb FunctionFS function */
	if (g_mkdir_with_parents (GADGET1 "/functions/ffs.adb", 0755) < 0)
		return error ("failed create functions dir");
	/* And link it to the gadget configuration */
	if (symlink (GADGET1 "/functions/ffs.adb", GADGET1 "/configs/c.1/f1") < 0)
		return error ("failed to create configuration link");
	/* Start adbd application */
	if (g_mkdir_with_parents ("/dev/usb-ffs/adb", 0755) < 0)
		return error ("failed to create usb-ffs dir");
	if (mount ("adb", "/dev/usb-ffs/adb", "functionfs", 0, NULL) < 0)
		return error ("failed to mount functionfs");
	//FIXME read from /sys/class/udc/
	if (write_content (GADGET1 "/UDC", "musb-hdrc.1") < 0)
		return error ("failed to enable UDC");

	return 0;
}

static int
stop (void)
{
	if (!g_file_test (GADGET, G_FILE_TEST_IS_DIR))
                return 0;

	return 1;
}

int main (int argc, char **argv)
{
	if (argc < 2)
		return error ("not enough arguments (needs start or stop)");
	if (g_strcmp0 (argv[1], "start") == 0)
		return start ();
	if (g_strcmp0 (argv[1], "stop") == 0)
		return stop ();

	return error ("wrong arguments (needs start or stop)");
}

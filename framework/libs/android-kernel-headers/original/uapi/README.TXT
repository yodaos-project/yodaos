This directory contains the uapi version of original kernel headers that are
used to generate Bionic's "cleaned-up" user-land headers. The script
bionic/libc/kernel/tools/generate_uapi_headers.sh automatically imports the
headers from an android kernel repository.

Running the script:

generate_uapi_headers.sh --download-kernel

In order to run the script, you must have properly initialized the build
environment using the lunch command. The script will automatically retrieve
an android kernel, generate all include files, and then copy the headers to
this directory.

The headers are mostly covered by the GPLv2 + exception, and thus cannot be
distributed as part of the platform itself.
(NOTE: The cleaned up headers do not contain copyrightable information and
are distributed with Bionic)

For more details, please read bionic/libc/kernel/README.TXT

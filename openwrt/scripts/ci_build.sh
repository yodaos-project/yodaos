#!/bin/bash 

set -o errexit

usage() {
cat <<USAGE

Usage:
    bash $0 -p <PRODUCT> [OPTIONS]

Description:
    Builds Linux buildroot tree for given PRODUCT

OPTIONS:
    -d, --debug
        Enable debugging - captures all commands while doing the build

    -c, --clean Clean output PRODUCT dir

    -r, --remove
	Remove output PRODUCT dir

    -h, --help
        Display this help message

    -j, --jobs
        Specifies the number of jobs to run simultaneously (Default: 8)

    -p, --product
        The product name (nana_l_a112, nana_t_s905d, rm101_s905d)
		
    -f, --preinstall_factory_apps
		Preinstall apps for factory (Default: APPS_FOR_FACTORY=true)

    -s, --solid file system, a113===>squashfs
		Read only filesystem

USAGE
}

check_exit()
{
    echo "$?"
	if  [ $? != 0 ]
	then
		exit $?

	fi
}

CHIP_VENDOR_SHORT_NAME="_kamino_"

remove_output_product_dir()
{
    echo "Now is make clean ^_^"
    echo "the dir is: ${ROOT_DIR}/openwrt/bin/${SELecTED_PRODUCT}-glibc"
    echo "Please waiting......................."
    cd ${ROOT_DIR}/openwrt/
    make clean
}
clean_output_product_dir()
{
    echo "Now is distclean ^_^"
    echo "Please waiting........................"
    cd ${ROOT_DIR}/openwrt/
    make distclean
}

enviroment()
{

    temp=$1
    echo "temp $1"
    cp ${ROOT_DIR}/openwrt/configs/${temp}_defconfig ${ROOT_DIR}/openwrt/.config
    cd ${ROOT_DIR}/openwrt/
    make defconfig
}


build_fullimage()
{
    build_image

	echo "$(pwd)"
	echo 'EXTRACT FULL IMG'
	echo
	
	if [ ! -d ${OUT_DIR}/full_images ]
	then
		mkdir -p ${OUT_DIR}/full_images
	else
		rm -rf ${OUT_DIR}/full_images/*
	fi

    VERSION_NODE=$(date +%Y%m%d)
    echo "VERSION_NODE ===${VERSION_NODE}"

    cp -f ${OUT_DIR}/bootx ${OUT_DIR}/full_images/bootx
    cp -f ${OUT_DIR}/bootx_win/bin/bootx.exe ${OUT_DIR}/full_images/bootx.exe
    cp -f ${OUT_DIR}/download.sh ${OUT_DIR}/full_images/download.sh
    cp -f ${OUT_DIR}/download.bat ${OUT_DIR}/full_images/download.bat
    cp -f ${OUT_DIR}/bootmusic.wav ${OUT_DIR}/full_images/bootmusic.wav
    cp -f ${OUT_DIR}/mcu.bin ${OUT_DIR}/full_images/
    cp -rf ${OUT_DIR}/openwrt-$Image_name-u-boot* ${OUT_DIR}/full_images/
    md5sum ${OUT_DIR}/openwrt-$Image_name-u-boot.img > ${OUT_DIR}/full_images/openwrt-$Image_name-u-boot.img.md5sum
    cp -f ${OUT_DIR}/openwrt-$Image_name.dtb ${OUT_DIR}/full_images/
    cp -f ${OUT_DIR}/openwrt-$Image_name-zImage ${OUT_DIR}/full_images/
    cp -f ${OUT_DIR}/openwrt-$Image_name-ubi.img ${OUT_DIR}/full_images/
    cp -f ${OUT_DIR}/openwrt-$Image_name-app.img ${OUT_DIR}/full_images/
    cp -f ${OUT_DIR}/openwrt-$Image_name-squashfs.img ${OUT_DIR}/full_images/
    md5sum ${OUT_DIR}/openwrt-$Image_name-ubi.img > ${OUT_DIR}/full_images/openwrt-$Image_name-ubi.img.md5sum
    md5sum ${OUT_DIR}/openwrt-$Image_name-squashfs.img > ${OUT_DIR}/full_images/openwrt-$Image_name-squashfs.img.md5sum
    cp -f ${OUT_DIR}/openwrt-$Image_name.swu ${OUT_DIR}/full_images/
    md5sum ${OUT_DIR}/openwrt-$Image_name.swu > ${OUT_DIR}/full_images/openwrt-$Image_name.swu.md5sum

	pushd ${ROOT_DIR}
    RELEASE_VERSION=$(awk -F "=" '$1=="ro.build.version.release"{print $2}' ${ROOT_DIR}/openwrt/version.txt)
    echo RELEASE_VERSION $RELEASE_VERSION
	popd
}
rokid_package()
{
    mkdir -p  ${OUT_DIR}/.tmp/
    cp -f ${OUT_DIR}/mcu.bin ${OUT_DIR}/.tmp/mcu.bin
    cp -f ${OUT_DIR}/*u-boot-spl* ${OUT_DIR}/.tmp/uboot-spl.bin
    cp -f ${OUT_DIR}/*u-boot.img ${OUT_DIR}/.tmp/uboot.bin
    cp -f ${OUT_DIR}/bootmusic.wav ${OUT_DIR}/.tmp/bootmusic.wav
    cp -f ${OUT_DIR}/openwrt-$Image_name.dtb ${OUT_DIR}/.tmp/openwrt-rokid.dtb
    cp -f ${OUT_DIR}/openwrt-$Image_name-zImage ${OUT_DIR}/.tmp/openwrt-rokid.zImage
    cp -f ${OUT_DIR}/full_images/openwrt-leo-ota-fit-uImage-initramfs.itb  ${OUT_DIR}/.tmp/openwrt-recovery.img
    cp -f ${OUT_DIR}/openwrt-$Image_name-ubi.img ${OUT_DIR}/.tmp/openwrt-ubi.img

    cp -f ${ROOT_DIR}/openwrt/build_dir/host/rokid_package/*  ${OUT_DIR}/.tmp/

    pushd ${OUT_DIR}/.tmp/
    ./rokid_package -c ./rokid_flash_package_bin.conf -o ../rokid-$Image_name-${RELEASE_VERSION}-ota.img
    ./rokid_package -c ./rokid_flash_ota_bin.conf -o ../rokid-$Image_name-${RELEASE_VERSION}-upgrade.img
    cp ../rokid-$Image_name-${RELEASE_VERSION}-ota.img ${OUT_DIR}/full_images/
    cp ../rokid-$Image_name-${RELEASE_VERSION}-upgrade.img ${OUT_DIR}/full_images/
    md5sum ../rokid-$Image_name-${RELEASE_VERSION}-ota.img > ${OUT_DIR}/full_images/rokid-$Image_name-${RELEASE_VERSION}-ota.img.md5sum
    popd
    
    #rm -rf ${OUT_DIR}/.tmp
}

build_ftpfiles()
{
    VERSION_NODE=$(date +%Y%m%d)
    echo "VERSION_NODE ===${VERSION_NODE}"
    echo 'full_images.zip ->' ${OUT_DIR}
    echo "$pwd"
    pushd ${OUT_DIR}
    echo "$pwd"
    zip -r ${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip full_images/*
    md5sum ${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip > ${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip.md5sum
    check_exit $?
    popd

    pushd ${ROOT_DIR}
    cp ${OUT_DIR}/${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip check_by_jenkins/${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip
    cp ${OUT_DIR}/${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip.md5sum check_by_jenkins/${SELecTED_PRODUCT}${CHIP_VENDOR_SHORT_NAME}linux_full_images_${VERSION_NODE}.zip.md5sum
    popd
}

build_otaimage()
{
    enviroment ${ota_defconfig}
    #modify recovery ota configure
    cp ${ROOT_DIR}/openwrt/target/linux/leo/config-4.4  ${ROOT_DIR}/openwrt/target/linux/leo/config-4.4.old
    cp ${ROOT_DIR}/openwrt/target/linux/leo/config_ota-4.4  ${ROOT_DIR}/openwrt/target/linux/leo/config-4.4
    remove_output_product_dir

    (echo -e \'\\0x65\'; echo -e \'\\0x79\') | make kernel_menuconfig
    
    build_image

    echo "$(pwd)"
    echo 'EXTRACT FULL IMG'
    echo

    if [ ! -d ${OTA_OUT_DIR}/full_images ]
    then
        mkdir -p ${OTA_OUT_DIR}/full_images
    else
        rm -rf ${OTA_OUT_DIR}/full_images/*
    fi

    cp -f ${OTA_OUT_DIR}/openwrt-leo-ota-fit-uImage-initramfs.itb ${OUT_DIR}/full_images/
    md5sum ${OTA_OUT_DIR}/openwrt-leo-ota-fit-uImage-initramfs.itb > ${OUT_DIR}/full_images/openwrt-leo-ota-fit-uImage-initramfs.itb.md5sum

    # recovery recovery configure
    cp ${ROOT_DIR}/openwrt/target/linux/leo/config-4.4.old  ${ROOT_DIR}/openwrt/target/linux/leo/config-4.4
}
build_ota_image()
{
    echo -e "\nINFO: Build ota packages\n"
    cd ${ROOT_DIR}/openwrt
    make ${CMD}
    check_exit
}
build_image()
{
    echo -e "\nINFO: Build full packages\n"
    cd ${ROOT_DIR}/openwrt
    make ${CMD}
    check_exit
}
# Set defaults
CLEAN_OUTPUT_PRODUCT_DIR=
REMOVE_OUTPUT_PRODUCT_DIR=
PACKAGES_TO_CLEAN=
JOBS=8
SELecTED_PRODUCT=

RELEASE_VERSION=
# Setup getopt.
long_opts="debug,clean,preinstall_factory_apps,help,jobs:,product:,name:,module:,solid_filesystem:"
getopt_cmd=$(getopt -o dcrfhj:p:n:m:s: --long "$long_opts" \
            -n $(basename $0) -- "$@") || \
            { echo -e "\nERROR: Getopt failed. Extra args\n"; usage; exit 1;}

ROOT_DIR=$(pwd)

eval set -- "$getopt_cmd"
while true; do
    case "$1" in
        -d|--debug) DEBUG="true";;
        -c|--clean) CLEAN_OUTPUT_PRODUCT_DIR="true";;
        -m|--module) PACKAGES_TO_CLEAN=$(echo $2 | tr "," "\n");;
        -r|--remove) REMOVE_OUTPUT_PRODUCT_DIR="true";;
        -f|--preinstall_rokid_factory_apps) APPS_FOR_FACTORY="true";;
        -h|--help) usage; exit 0;;
        -j|--jobs) JOBS="$2"; shift;;
        -p|--product) SELecTED_PRODUCT="$2"; shift;;
        -n|--name) Image_name="$2"; shift;;
        -s|--solid_filesystem) BUILD_ROOT_FILESYSTEM="$2"; shift;;
        --) shift; break;;
    esac
    shift
done


if [ -z $SELecTED_PRODUCT ];then
	usage
	exit 1
else
	echo "SELecTED_PRODUCT=${SELecTED_PRODUCT}"
fi

if [ "${CLEAN_OUTPUT_PRODUCT_DIR}" = "true" ]; then
    clean_output_product_dir
fi

enviroment ${SELecTED_PRODUCT}
if [ -z $Image_name ]; then
	Image_name=$(echo ${SELecTED_PRODUCT}|sed 's/_/-/g')
else
	Image_name=$(echo ${Image_name}|sed 's/_/-/g')
fi
echo "temp_out ===${Image_name}"
OUT_DIR="${ROOT_DIR}/openwrt/bin/${Image_name}-glibc/"
ota_defconfig=leo_gx8010_ota_1v
ota_dir=$(echo ${ota_defconfig}|sed 's/_/-/g')
OTA_OUT_DIR="${ROOT_DIR}/openwrt/bin/${ota_dir}-glibc/"


echo "IMAGES_DIR=${OUT_DIR}"

CMD="-j ${JOBS}"
if [ "${DEBUG}" = "true" ]; then
    CMD+=" V=s"
fi

if [ "${REMOVE_OUTPUT_PRODUCT_DIR}" = "true" ]; then
    remove_output_product_dir
fi


if [ "${APPS_FOR_FACTORY}" = "true" ]; then
    export APPS_FOR_FACTORY=true
fi

if [ -d ${ROOT_DIR}/check_by_jenkins ]; then
	rm -rf ${ROOT_DIR}/check_by_jenkins
fi
mkdir -p ${ROOT_DIR}/check_by_jenkins

check_exit

# clean packages which is specified
for package in $PACKAGES_TO_CLEAN
do
    # echo "clean ${TARGET_OUTPUT_DIR}/build/${package}..."
    rm -rf ${ROOT_DIR}/build_dir/target-arm_cortex-a7+neon-glibc-2.22_eabi/${package}*

done

build_fullimage
build_otaimage
rokid_package
build_ftpfiles
check_exit

pushd $ROOT_DIR
{
    ls check_by_jenkins/
} > check_by_jenkins/ftp_file_list
sed -i 's/^/check_by_jenkins\//' check_by_jenkins/ftp_file_list
popd

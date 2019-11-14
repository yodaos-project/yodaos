#!/bin/bash
echo "Start buildprop"
echo -----$1
echo -----$2

# openwrt buildprop.sh will action twice so we workround here
echo $1 |grep staging
result=$?
if [ $result = 0 ];then
    echo "this is staging exit"
    exit 0
fi

ROOT_DIR=$1
TOPDIR=$2
ALAB_DIR=$TOPDIR/../frameworks/aial/openvoice/alab-libs
VSP_DIR=$TOPDIR/../hardware/nationalchip/gx8010/vsp
BUILD_PROP_FILE=$ROOT_DIR/system/build.prop
BUILD_VERSION_FILE=$TOPDIR/version.txt
SUFFIX=$TOPDIR/suffix.txt
CONFIG_FILE=$TOPDIR/.config
# HAS_LUA="CONFIG_ROKIDOS_FEATURES_HAS_LUA=y"

#1.first we should delete old build.prop and creat new
if [ -f "$BUILD_PROP_FILE" ]; then
    echo "exist, rm $BUILD_PROP_FILE"
    rm -rf $BUILD_PROP_FILE
fi



#2.echo "create new file"
touch $BUILD_PROP_FILE
if [ -f "$BUILD_VERSION_FILE" ]; then
    rm -rf $BUILD_VERSION_FILE
fi
touch $BUILD_VERSION_FILE

if [ -f "$SUFFIX" ]; then
    rm -rf $SUFFIX
fi
touch $SUFFIX

##build version
# if [ `grep -c $HAS_LUA $CONFIG_FILE` -eq 0 ]; then
    # echo "nodejs"
    # ROKID_PLATFORM_VERSION="0.2.0_rc5"
# else
    # echo "lua"
    # ROKID_PLATFORM_VERSION="3.0.2"
# fi

ROKID_PLATFORM_VERSION="3.0.4"

pushd $ALAB_DIR
TUREN_COMMITID=$(git rev-parse --short HEAD)
echo $TUREN_COMMITID
popd

pushd $VSP_DIR
VSP_COMMITID=$(git rev-parse --short HEAD)
echo $VSP_COMMITID
popd


ROKID_SDK_VERSION="1.0.9"
PROP_DATE_NAME="ro.rokid.build.date"
PROP_TIME_NAME="ro.rokid.build.time"
BUILD_DATE="`TZ=Asia/Shanghai date "+%Y-%m-%d"`"
BUILD_TIME="`TZ=Asia/Shanghai date "+%H:%M:%S"`"
BUILD_ZONE="`date "+%Z"`"
ROKID_BUILD_TIMESTAMP="`TZ=Asia/Shanghai date +"%Y%m%d-%H%M%S"`"


echo "builddate=$BUILD_DATE"
echo "buildtime=$BUILD_TIME"
echo "buildzone=$BUILD_ZONE"
echo "build_timestamp=$ROKID_BUILD_TIMESTAMP"

PROP_DATE_NAME_VALUE="${PROP_DATE_NAME}=${BUILD_DATE}"
PROP_TIME_NAME_VALUE="${PROP_TIME_NAME}=${BUILD_TIME}"
echo "ro.build.version.release=$ROKID_SDK_VERSION-$ROKID_BUILD_TIMESTAMP" >> $BUILD_PROP_FILE
echo "ro.rokid.build.turen=$ROKID_SDK_VERSION-$ROKID_BUILD_TIMESTAMP-$TUREN_COMMITID" >> $BUILD_PROP_FILE
echo "ro.rokid.build.vsp=$ROKID_SDK_VERSION-$ROKID_BUILD_TIMESTAMP-$VSP_COMMITID" >> $BUILD_PROP_FILE

VERSION_NODE=$ROKID_PLATFORM_VERSION-$ROKID_BUILD_TIMESTAMP
echo "VERSION_NODE"=$VERSION_NODE > $SUFFIX

#ROKIDOS_BOARDCONFIG_CAPTURE_DEVICEID
CAPTURE_NAME=ROKIDOS_BOARDCONFIG_CAPTURE_DEVICEID
if [ `grep -c $CAPTURE_NAME $CONFIG_FILE` -eq 0 ]; then
    echo "not find $CAPTURE_NAME"
else
    #echo "find $CAPTURE_NAME"
    CAPTURENAMEID=`grep $CAPTURE_NAME $CONFIG_FILE`
    #echo $CAPTURENAMEID
    CAPID=${CAPTURENAMEID##*=}
    #echo $CAPID
    echo "ro.rokid.captureid=$CAPID" >> $BUILD_PROP_FILE
fi

#ROKIDOS_BOARDCONFIG_PLAYBACK_DEVICEID
PLAY_NAME=ROKIDOS_BOARDCONFIG_PLAYBACK_DEVICEID
if [ `grep -c $PLAY_NAME $CONFIG_FILE` -eq 0 ]; then
    echo "not find $PLAY_NAME"
else
    #echo "find $PALY_NAME"
    PLAYNAMEID=`grep $PLAY_NAME $CONFIG_FILE`
    #echo $PLAYNAMEID
    PLAYID=${PLAYNAMEID##*=}
    #echo $PLAYID
    echo "ro.rokid.playid=$PLAYID" >> $BUILD_PROP_FILE
fi
cat $BUILD_PROP_FILE > $BUILD_VERSION_FILE

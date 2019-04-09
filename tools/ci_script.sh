CWD=`pwd`
REPO=$CWD/tools/repo

# upgrade apt-get
sudo apt-get update -q
sudo apt-get install -q -y \
    pkg-config openssl \
    build-essential subversion \
    libncurses5-dev zlib1g-dev \
    gawk gcc-multilib flex git-core \
    gettext libssl-dev unzip texinfo \
    device-tree-compiler dosfstools \
    libusb-1.0-0-dev

# set ssh config
ssh -o StrictHostKeyChecking=no -p 29418 $ROKID_USERNAME@openai-corp.rokid.com ""
mkdir -p yodaos-source
cd yodaos-source

# set git config
git config --global color.ui false
git config --global user.email "circleci@rokid.com"
git config --global user.name "circle ci"

# initilize the repo
$REPO init \
  -u https://github.com/yodaos-project/yodaos.git \
  -m manifest.xml \
  --repo-url=http://openai-corp.rokid.com/tools/repo \
  --no-repo-verify

# sync
.repo/manifests/tools/repo-username -u $ROKID_USERNAME
$REPO sync

# compile
cp -r products/yodaos/rbpi-3b-plus/configs/broadcom_bcm2710_rpi3b_plus_defconfig openwrt/.config
cd openwrt
make defconfig && make -j1 V=s

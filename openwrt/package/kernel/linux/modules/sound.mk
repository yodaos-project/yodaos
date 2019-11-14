#
# Copyright (C) 2006-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

SOUND_MENU:=Sound Support

# allow targets to override the soundcore stuff
ifeq ($(CONFIG_TARGET_a113l), y)
SOUNDCORE_LOAD ?= \
	soundcore \
	snd \
	snd-hwdep \
	snd-rawmidi \
	snd-timer \
	snd-pcm \
	snd-mixer-oss \
	snd-pcm-oss \
	snd-compress

SOUNDCORE_FILES ?= \
	$(LINUX_DIR)/sound/soundcore.ko \
	$(LINUX_DIR)/sound/core/snd.ko \
	$(LINUX_DIR)/sound/core/snd-hwdep.ko \
	$(LINUX_DIR)/sound/core/snd-rawmidi.ko \
	$(LINUX_DIR)/sound/core/snd-timer.ko \
	$(LINUX_DIR)/sound/core/snd-pcm.ko \
	$(LINUX_DIR)/sound/core/oss/snd-mixer-oss.ko \
	$(LINUX_DIR)/sound/core/oss/snd-pcm-oss.ko \
	$(LINUX_DIR)/sound/core/snd-compress.ko \
    $(LINUX_DIR)/sound/soc/snd-soc-core.ko
else
SOUNDCORE_LOAD ?= \
	soundcore \
	snd \
	snd-hwdep \
	snd-seq-device \
	snd-rawmidi \
	snd-timer \
	snd-pcm \
	snd-mixer-oss \
	snd-pcm-oss \
	snd-compress

SOUNDCORE_FILES ?= \
	$(LINUX_DIR)/sound/soundcore.ko \
	$(LINUX_DIR)/sound/core/snd.ko \
	$(LINUX_DIR)/sound/core/snd-hwdep.ko \
	$(LINUX_DIR)/sound/core/seq/snd-seq-device.ko \
	$(LINUX_DIR)/sound/core/snd-rawmidi.ko \
	$(LINUX_DIR)/sound/core/snd-timer.ko \
	$(LINUX_DIR)/sound/core/snd-pcm.ko \
	$(LINUX_DIR)/sound/core/oss/snd-mixer-oss.ko \
	$(LINUX_DIR)/sound/core/oss/snd-pcm-oss.ko \
	$(LINUX_DIR)/sound/core/snd-compress.ko \
    $(LINUX_DIR)/sound/soc/snd-soc-core.ko  
endif
SOUNDCORE_LOAD += \
	$(if $(CONFIG_SND_DMAENGINE_PCM),snd-pcm-dmaengine)

SOUNDCORE_FILES += \
	$(if $(CONFIG_SND_DMAENGINE_PCM),$(LINUX_DIR)/sound/core/snd-pcm-dmaengine.ko) \
	$(if $(CONFIG_SND_SIMPLE_CARD_UTILS),$(LINUX_DIR)/sound/soc/generic/snd-soc-simple-card-utils.ko) \
	$(if $(CONFIG_AC97_BUS),$(LINUX_DIR)/sound/ac97_bus.ko) 

ifeq ($(CONFIG_TARGET_x86_64), y)
	SOUNDCORE_DEPENDS_EXTRA = +kmod-regmap
endif

define KernelPackage/sound-core
  SUBMENU:=$(SOUND_MENU)
  TITLE:=Sound support
  DEPENDS:=@AUDIO_SUPPORT +kmod-input-core $(SOUNDCORE_DEPENDS_EXTRA)
  KCONFIG:= \
	CONFIG_SOUND \
	CONFIG_SND \
	CONFIG_SND_HWDEP \
	CONFIG_SND_RAWMIDI \
	CONFIG_SND_TIMER \
	CONFIG_SND_PCM \
	CONFIG_SND_PCM_TIMER \
	CONFIG_SND_SEQUENCER \
	CONFIG_SND_VIRMIDI \
	CONFIG_SND_SEQ_DUMMY \
	CONFIG_SND_SEQUENCER_OSS=y \
	CONFIG_HOSTAUDIO \
	CONFIG_SND_PCM_OSS \
	CONFIG_SND_MIXER_OSS \
	CONFIG_SOUND_OSS_CORE_PRECLAIM=y \
	CONFIG_SND_COMPRESS_OFFLOAD
  FILES:=$(SOUNDCORE_FILES)
  AUTOLOAD:=$(call AutoLoad,30,$(SOUNDCORE_LOAD))
endef

define KernelPackage/sound-core/uml
  FILES:= \
	$(LINUX_DIR)/sound/soundcore.ko \
	$(LINUX_DIR)/arch/um/drivers/hostaudio.ko
  AUTOLOAD:=$(call AutoLoad,30,soundcore hostaudio)
endef

define KernelPackage/sound-core/description
 Kernel modules for sound support
endef

$(eval $(call KernelPackage,sound-core))


define AddDepends/sound
  SUBMENU:=$(SOUND_MENU)
  DEPENDS+=kmod-sound-core $(1) @!TARGET_uml
endef


define KernelPackage/ac97
  TITLE:=ac97 controller
  KCONFIG:=CONFIG_SND_AC97_CODEC=y \
	  CONFIG_AC97_BUS=y
  FILES:= \
	$(LINUX_DIR)/sound/pci/ac97/snd-ac97-codec.ko 
  FILES:= \
	$(LINUX_DIR)/sound/ac97_bus.ko \
	$(LINUX_DIR)/sound/pci/ac97/snd-ac97-codec.ko
  AUTOLOAD:=$(call AutoLoad,35,ac97_bus snd-ac97-codec)
  $(call AddDepends/sound)
endef

define KernelPackage/ac97/description
 The ac97 controller
endef

$(eval $(call KernelPackage,ac97))


define KernelPackage/sound-mpu401
  TITLE:=MPU-401 uart driver
  KCONFIG:=CONFIG_SND_MPU401_UART
  FILES:= \
	$(LINUX_DIR)/sound/drivers/mpu401/snd-mpu401-uart.ko
  AUTOLOAD:=$(call AutoLoad,35,snd-mpu401-uart)
  $(call AddDepends/sound)
endef

define KernelPackage/sound-mpu401/description
 support for MIDI ports compatible with the Roland MPU-401
 interface in UART mode.
endef

$(eval $(call KernelPackage,sound-mpu401))


define KernelPackage/sound-seq
  TITLE:=Sequencer support
  FILES:= \
	$(LINUX_DIR)/sound/core/seq/snd-seq.ko \
	$(LINUX_DIR)/sound/core/seq/snd-seq-midi-event.ko \
	$(LINUX_DIR)/sound/core/seq/snd-seq-midi.ko
  AUTOLOAD:=$(call AutoLoad,35,snd-seq snd-seq-midi-event snd-seq-midi)
  $(call AddDepends/sound)
endef

define KernelPackage/sound-seq/description
 Kernel modules for sequencer support
endef

$(eval $(call KernelPackage,sound-seq))


define KernelPackage/sound-i8x0
  TITLE:=Intel/SiS/nVidia/AMD/ALi AC97 Controller
  DEPENDS:=+kmod-ac97
  KCONFIG:=CONFIG_SND_INTEL8X0
  FILES:=$(LINUX_DIR)/sound/pci/snd-intel8x0.ko
  AUTOLOAD:=$(call AutoLoad,36,snd-intel8x0)
  $(call AddDepends/sound)
endef

define KernelPackage/sound-i8x0/description
 support for the integrated AC97 sound device on motherboards
 with Intel/SiS/nVidia/AMD chipsets, or ALi chipsets using
 the M5455 Audio Controller.
endef

$(eval $(call KernelPackage,sound-i8x0))


define KernelPackage/sound-via82xx
  TITLE:=VIA 82xx AC97 Controller
  DEPENDS:=+kmod-ac97 +kmod-sound-mpu401
  KCONFIG:=CONFIG_SND_VIA82XX
  FILES:=$(LINUX_DIR)/sound/pci/snd-via82xx.ko
  AUTOLOAD:=$(call AutoLoad,36,snd-via82xx)
  $(call AddDepends/sound)
endef

define KernelPackage/sound-via82xx/description
 support for the integrated AC97 sound device on motherboards
 with VIA chipsets.
endef

$(eval $(call KernelPackage,sound-via82xx))


define KernelPackage/sound-soc-core
  TITLE:=SoC sound support
  DEPENDS:=+kmod-regmap +kmod-ac97
  KCONFIG:= \
	CONFIG_SND_SOC \
	CONFIG_SND_SOC_DMAENGINE_PCM=y \
	CONFIG_SND_SOC_ALL_CODECS=n
  FILES:=$(LINUX_DIR)/sound/soc/snd-soc-core.ko
  AUTOLOAD:=$(call AutoLoad,55, snd-soc-core)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-core))


define KernelPackage/sound-soc-ac97
  TITLE:=AC97 Codec support
  KCONFIG:=CONFIG_SND_SOC_AC97_CODEC
  FILES:=$(LINUX_DIR)/sound/soc/codecs/snd-soc-ac97.ko
  AUTOLOAD:=$(call AutoLoad,57,snd-soc-ac97)
  DEPENDS:=+kmod-ac97 +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-ac97))


define KernelPackage/sound-soc-seeed
  TITLE:=ASoC support for seeed
  KCONFIG:= \
	  CONFIG_SND_SOC_WM8960	\
	  CONFIG_SND_SOC_AC108	\
	  CONFIG_SND_SOC_SEEED_VOICECARD
  FILES:= \
	  $(LINUX_DIR)/sound/soc/seeed/snd-soc-ac108.ko \
	  $(LINUX_DIR)/sound/soc/seeed/snd-soc-seeed-wm8960.ko \
	  $(LINUX_DIR)/sound/soc/seeed/snd-soc-seeed-voicecard.ko
  AUTOLOAD:=$(call AutoLoad,60,snd-soc-ac108 snd-soc-seeed-wm8960 snd-soc-seeed-voicecard)
  DEPENDS:=@TARGET_brcm2708 +kmod-sound-soc-core  +kmod-i2c-core  +kmod-sound-soc-core +kmod-sound-core
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-seeed))


define KernelPackage/sound-soc-imx
  TITLE:=IMX SoC support
  KCONFIG:=\
	CONFIG_SND_IMX_SOC \
	CONFIG_SND_SOC_IMX_AUDMUX \
	CONFIG_SND_SOC_FSL_SSI \
	CONFIG_SND_SOC_IMX_PCM_DMA
  FILES:= \
	$(LINUX_DIR)/sound/soc/fsl/snd-soc-imx-audmux.ko \
	$(LINUX_DIR)/sound/soc/fsl/snd-soc-fsl-ssi.ko \
	$(LINUX_DIR)/sound/soc/fsl/imx-pcm-dma.ko
  AUTOLOAD:=$(call AutoLoad,56,snd-soc-imx-audmux snd-soc-fsl-ssi snd-soc-imx-pcm)
  DEPENDS:=@TARGET_imx6 +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-imx/description
 Support for i.MX6 Platform sound (ssi/audmux/pcm)
endef

$(eval $(call KernelPackage,sound-soc-imx))


define KernelPackage/sound-soc-imx-sgtl5000
  TITLE:=IMX SoC support for SGTL5000
  KCONFIG:=CONFIG_SND_SOC_IMX_SGTL5000
  FILES:=\
	$(LINUX_DIR)/sound/soc/codecs/snd-soc-sgtl5000.ko \
	$(LINUX_DIR)/sound/soc/fsl/snd-soc-imx-sgtl5000.ko
  AUTOLOAD:=$(call AutoLoad,57,snd-soc-sgtl5000 snd-soc-imx-sgtl5000)
  DEPENDS:=@TARGET_imx6 +kmod-sound-soc-imx
  $(call AddDepends/sound)
endef

define KernelPackage/sound-soc-imx-sgtl5000/description
 Support for i.MX6 Platform sound SGTL5000 codec
endef

$(eval $(call KernelPackage,sound-soc-imx-sgtl5000))


define KernelPackage/sound-soc-gw_avila
  TITLE:=Gateworks Avila SoC sound support
  KCONFIG:= \
	CONFIG_SND_GW_AVILA_SOC \
	CONFIG_SND_GW_AVILA_SOC_PCM \
	CONFIG_SND_GW_AVILA_SOC_HSS
  FILES:= \
	$(LINUX_DIR)/sound/soc/codecs/snd-soc-tlv320aic3x.ko \
	$(LINUX_DIR)/sound/soc/gw-avila/snd-soc-gw-avila.ko \
	$(LINUX_DIR)/sound/soc/gw-avila/snd-soc-gw-avila-pcm.ko \
	$(LINUX_DIR)/sound/soc/gw-avila/snd-soc-gw-avila-hss.ko
  AUTOLOAD:=$(call AutoLoad,65,snd-soc-tlv320aic3x snd-soc-gw-avila snd-soc-gw-avila-pcm snd-soc-gw-avila-hss)
  DEPENDS:=@TARGET_ixp4xx +kmod-sound-soc-core
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-gw_avila))


define KernelPackage/pcspkr
  DEPENDS:=@TARGET_x86 +kmod-input-core
  TITLE:=PC speaker support
  KCONFIG:= \
	CONFIG_INPUT_PCSPKR \
	CONFIG_SND_PCSP
  FILES:= \
	$(LINUX_DIR)/drivers/input/misc/pcspkr.ko \
	$(LINUX_DIR)/sound/drivers/pcsp/snd-pcsp.ko
  AUTOLOAD:=$(call AutoLoad,50,pcspkr snd-pcsp)
  $(call AddDepends/sound)
endef

define KernelPackage/pcspkr/description
 This enables sounds (tones) through the pc speaker
endef

$(eval $(call KernelPackage,pcspkr))

define KernelPackage/sound-dummy
  $(call AddDepends/sound)
  TITLE:=Null sound output driver (sink)
  KCONFIG:= \
	CONFIG_SND_DUMMY
  FILES:= \
	$(LINUX_DIR)/sound/drivers/snd-dummy.ko
  AUTOLOAD:=$(call AutoLoad,32,snd-dummy)
endef

define KernelPackage/sound_dummy/description
 Dummy sound device for Alsa when no hardware present
endef

$(eval $(call KernelPackage,sound-dummy))

define KernelPackage/sound-soc-nationalchip
  TITLE:=ASoC support for Nationalchip
  KCONFIG:= \
	  CONFIG_SND_SOC_NATIONALCHIP
  FILES:= \
	  $(LINUX_DIR)/sound/soc/nationalchip/snd-soc-gx-machine.ko \
	  $(LINUX_DIR)/sound/soc/nationalchip/snd-soc-gx-platform-dai.ko \
	  $(LINUX_DIR)/sound/soc/nationalchip/snd-soc-gx-platform.ko
  AUTOLOAD:= $(call AutoLoad,60,snd-soc-gx-platform-dai snd-soc-gx-platform snd-soc-gx-machine)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-nationalchip))

define KernelPackage/sound-nationalchip
  TITLE:=ALSA support for Nationalchip
  KCONFIG:= \
	  CONFIG_SND_NATIONALCHIP_PCM
  FILES:= \
	  $(LINUX_DIR)/sound/arm/nationalchip/snd-gx-pcm.ko
  AUTOLOAD:=$(call AutoLoad,60,snd-gx-pcm)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-nationalchip))

define KernelPackage/sound-soc-gx-codec
  TITLE:=Nationalchip LODAC CODEC
  KCONFIG:= \
	  CONFIG_SND_SOC_GX_CODEC
  FILES:= \
	  $(LINUX_DIR)/sound/soc/codecs/snd-soc-gx-codec.ko
  DEPENDS:=kmod-sound-soc-nationalchip
  AUTOLOAD:=$(call AutoLoad,59,snd-soc-gx-codec)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-gx-codec))

define KernelPackage/sound-soc-nau8822
  TITLE:=Nuvoton NAU8822 CODEC
  KCONFIG:= \
	  CONFIG_SND_SOC_NAU8822
  FILES:= \
	  $(LINUX_DIR)/sound/soc/codecs/snd-soc-nau8822.ko
  DEPENDS:=kmod-sound-soc-nationalchip
  AUTOLOAD:=$(call AutoLoad,59,snd-soc-nau8822)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-nau8822))

define KernelPackage/sound-soc-npcp215x
  TITLE:=Nuvoton NPCP215X CODEC
  KCONFIG:= \
	  CONFIG_SND_SOC_NPCP215X
  FILES:= \
	  $(LINUX_DIR)/sound/soc/codecs/snd-soc-npcp215x.ko
  DEPENDS:=kmod-sound-soc-nationalchip
  AUTOLOAD:=$(call AutoLoad,59,snd-soc-npcp215x)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-npcp215x))

define KernelPackage/sound-soc-alc1311
  TITLE:=Realt ALC1311 CODEC
  KCONFIG:= \
	  CONFIG_SND_SOC_ALC1311
  FILES:= \
	  $(LINUX_DIR)/sound/soc/codecs/snd-soc-alc1311.ko
  DEPENDS:=kmod-sound-soc-nationalchip
  AUTOLOAD:=$(call AutoLoad,59,snd-soc-alc1311)
  $(call AddDepends/sound)
endef

$(eval $(call KernelPackage,sound-soc-alc1311))

define KernelPackage/sound-soc-sma1301
   TITLE:=Silicon mitus SMA1301 Audio Amplifier CODEC
   KCONFIG:= \
           CONFIG_SND_SOC_SMA1301
   FILES:= \
           $(LINUX_DIR)/sound/soc/codecs/snd-soc-sma1301.ko
   DEPENDS:=kmod-sound-soc-nationalchip
   AUTOLOAD:=$(call AutoLoad,59,snd-soc-sma1301)
   $(call AddDepends/sound)
 endef

$(eval $(call KernelPackage,sound-soc-sma1301))

define KernelPackage/sound-soc-tas5805m
   TITLE:=Ti_tas5805m Audio Amplifier CODEC
   KCONFIG:= \
           CONFIG_SND_SOC_TAS5805M
   FILES:= \
           $(LINUX_DIR)/sound/soc/codecs/snd-soc-tas5805m.ko
   DEPENDS:=kmod-sound-soc-nationalchip
   AUTOLOAD:=$(call AutoLoad,59,snd-soc-tas5805m)
   $(call AddDepends/sound)
 endef

$(eval $(call KernelPackage,sound-soc-tas5805m))


define KernelPackage/sound-soc-ad82584f
   TITLE:=AD82584F Audio Amplifier CODEC
   KCONFIG:= \
           CONFIG_SND_SOC_AD82584F
   FILES:= \
           $(LINUX_DIR)/sound/soc/codecs/snd-soc-ad82584f.ko
   DEPENDS:=kmod-sound-soc-nationalchip
   AUTOLOAD:=$(call AutoLoad,59,snd-soc-ad82584f)
   $(call AddDepends/sound)
 endef

$(eval $(call KernelPackage,sound-soc-ad82584f))

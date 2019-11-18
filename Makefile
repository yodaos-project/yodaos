TOP_DIR:=${CURDIR}
TARGET_DIR:=openwrt
DEBUG_BUILD:=
TARGET_BUILD:=
ifeq ("$(origin V)", "command line")
	DEBUG_BUILD+= V=$(V) 
endif
ifeq ("$(origin command)", "command line")
  ifeq ("$(origin target)", "command line")
    ifeq ($(strip $(target)),)
		TARGET_BUILD+= target/$(command)
    else
		TARGET_BUILD+= target/$(target)/$(command)
    endif
  else
    ifeq ("$(origin package)", "command line")
      ifeq ($(strip $(package)),)
		TARGET_BUILD+= package/$(command)
      else
		TARGET_BUILD+= package/$(package)/$(command)
      endif
    endif
  endif
endif
all:
	@$(MAKE) -C $(TARGET_DIR) $(DEBUG_BUILD) $(TARGET_BUILD) 
clean:
	@$(MAKE) -C $(TARGET_DIR) clean
distclean:
	@$(MAKE) -C $(TARGET_DIR) distclean

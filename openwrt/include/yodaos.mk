#
# configs for board or system can be defined here, then the configs can be used by all modules.
#
ifeq ($(CONFIG_DEBUG),y)
  EXTRA_CFLAGS += -DYODAOS_DEBUG
endif
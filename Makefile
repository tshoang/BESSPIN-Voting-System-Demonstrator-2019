# fpga/sim are unconditionally defined,
# so set the default target to freertos
TARGET ?= freertos
$(info TARGET=$(TARGET))

# List all substems directories
export SOURCE_DIR       = $(PWD)/source
export SBB_DIR          = $(SOURCE_DIR)/sbb
export LOG_DIR          = $(SOURCE_DIR)/logging
export CRYPTO_DIR       = $(SOURCE_DIR)/crypto
export CRYPTO_TEST_DIR  = $(SOURCE_DIR)/tests/crypto
export LOGGING_TEST_DIR = $(SOURCE_DIR)/tests/logging
export SBB_TEST_DIR     = $(SOURCE_DIR)/tests/sbb
export INCLUDE_DIR      = $(SOURCE_DIR)/include

# Set correct C header location for macOS 10.14+ hosts
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
    export CPATH = $(shell xcrun --show-sdk-path)/usr/include
endif

# Expected GFE repo location (for flash scripts)
CURRENT_PATH       = $(shell pwd)
GFE_DIR           ?= $(CURRENT_PATH)/../gfe
P1_BITSTREAM_PATH ?= $(GFE_DIR)/bitstreams/soc_chisel_p1.bit

# To enable the running lights task
export USE_LED_BLINK_TASK=1

.PHONY: all posix_all bottom_all freertos_all typecheck_all verify_all
.PHONY: sim fpga
.PHONY: clean clean_all clean_crypto clean_logging clean_sbb
.PHONY: hosttest_all crypto_hosttest_clean logging_hosttest_clean sbb_hosttest_clean
.PHONY: sim_all sim_crypto sim_logging sim_sbb
.PHONY: all_boxes upload_binary_box1 upload_binary_box2 upload_binary_box3 upload_binary_box4
.PHONY: upload_binary_sim upload_binary_and_bitstream_sim upload_binary_fpga upload_binary_and_bitstream_sim
.PHONY: posix_crypto posix_logging posix_sbb
.PHONY: freertos_crypto freertos_logging freertos_sbb
.PHONY: typecheck_crypto typecheck_logging typecheck_sbb
.PHONY: verify_crypto verify_logging verify_sbb
.PHONY: crypto_hosttest_all logging_hosttest_all sbb_hosttest_all

#####################################
#
#		SBB targets
#
#####################################
all: sim fpga

fpga:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos default
	cp $(SOURCE_DIR)/default_ballot_box.* .

sim:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim default
	cp $(SOURCE_DIR)/default_ballot_box_sim.* .

clean_all:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos clean
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim clean
	rm -f default_ballot_box.*
	rm -f default_ballot_box_sim.*

#####################################
#
#		BOTTOM targets
#
#####################################
ifeq ($(TARGET),bottom)
export OS_DIR  = $(SOURCE_DIR)/os/bottom
export CFLAGS += -I $(INCLUDE_DIR) \
                 -I $(INCLUDE_DIR)/crypto \
                 -I $(INCLUDE_DIR)/logging \
                 -I $(INCLUDE_DIR)/sbb \
                 -DVOTING_PLATFORM_BOTTOM

include $(SBB_DIR)/Makefile.bottom
include $(CRYPTO_DIR)/Makefile.bottom
include $(LOG_DIR)/Makefile.bottom

bottom_all: crypto_bottom logging_bottom sbb_bottom
clean: crypto_bottom_clean logging_bottom_clean sbb_bottom_clean

else
#####################################
#
#		POSIX targets
#
#####################################
ifeq ($(TARGET),posix)

posix_all: posix_crypto posix_logging posix_sbb

clean: clean_crypto clean_logging clean_sbb

posix_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix crypto

posix_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix log

posix_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix sbb

clean_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix clean_crypto

clean_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix clean_logging

clean_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.posix clean_sbb

else
#####################################
#
#		FREERTOS targets
#
#####################################
ifeq ($(TARGET),freertos)

freertos_all: freertos_crypto freertos_logging freertos_sbb

clean: clean_sbb clean_crypto clean_logging

freertos_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos crypto

freertos_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos logging

freertos_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos sbb

clean_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos clean

clean_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos clean

clean_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos clean_sbb

else
#####################################
#
#		VERIFICATION targets
#
#####################################
ifeq ($(TARGET),verification)
export OS_DIR = $(SOURCE_DIR)/os/posix

typecheck_all: typecheck_crypto typecheck_sbb typecheck_logging

verify_all: verify_crypto verify_sbb verify_logging

clean: clean_crypto clean_sbb clean_logging

typecheck_crypto:
	cd $(CRYPTO_DIR) ; \
	$(MAKE) -f Makefile.verification typecheck

verify_crypto:
	cd $(CRYPTO_DIR) ; \
	$(MAKE) -f Makefile.verification verify

clean_crypto:
	cd $(CRYPTO_DIR) ; \
	$(MAKE) -f Makefile.verification clean

typecheck_sbb:
	cd $(SBB_DIR) ; \
	$(MAKE) -f Makefile.verification typecheck

verify_sbb:
	cd $(SBB_DIR) ; \
	$(MAKE) -f Makefile.verification verify

clean_sbb:
	cd $(SBB_DIR) ; \
	$(MAKE) -f Makefile.verification clean

typecheck_logging:
	cd $(LOG_DIR) ; \
	$(MAKE) -f Makefile.verification typecheck

verify_logging:
	cd $(LOG_DIR) ; \
	$(MAKE) -f Makefile.verification verify

clean_logging:
	cd $(LOG_DIR) ; \
	$(MAKE) -f Makefile.verification clean

else
#####################################
#
#		HOSTTESTS targets
#
#####################################
ifeq ($(TARGET),hosttests)
export OS_DIR = $(SOURCE_DIR)/os/posix

# Assume clang is on PATH, but needs some special setup on Darwin to override
# Apple's clang and use the HomeBrew one instead...
export CC := clang

export HOSTTEST_CFLAGS = \
	-g -m64 -Werror -Wall -DVOTING_PLATFORM_POSIX -DNO_MEMSET_S \
	-DVOTING_SYSTEM_DEBUG -DLOG_SYSTEM_DEBUG \
	-Wno-macro-redefined -I$(INCLUDE_DIR) \
        -fsanitize=address,undefined

hosttest_all: crypto_hosttest_all logging_hosttest_all sbb_hosttest_all

clean: crypto_hosttest_clean logging_hosttest_clean sbb_hosttest_clean

crypto_hosttest_all:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests crypto_hosttest_all

crypto_hosttest_clean:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests crypto_hosttest_clean

logging_hosttest_all:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests logging_hosttest_all

logging_hosttest_clean:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests logging_hosttest_clean

sbb_hosttest_all:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests sbb_hosttest_all

sbb_hosttest_clean:
	cd $(SOURCE_DIR); \
	$(MAKE) -f Makefile.hosttests sbb_hosttest_clean

else
#####################################
#
#		SIMULATION targets
#
#####################################
ifeq ($(TARGET),sim)
export OS_DIR = $(SOURCE_DIR)/os/freertos
export CFLAGS := -DVOTING_PLATFORM_FREERTOS

sim_all: sim_crypto sim_logging sim_sbb

clean: clean_crypto clean_logging clean_sbb

sim_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim crypto

clean_crypto:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim clean_crypto

sim_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim logging

clean_logging:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim clean_logging

sim_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim sbb

clean_sbb:
	cd $(SOURCE_DIR) ; \
	$(MAKE) -f Makefile.freertos_sim clean_sbb

else
#####################################
#
#		DEPLOYMENT targets
#
#####################################
ifeq ($(TARGET),deploy)
all_boxes:
	cd $(SBB_DIR) ; \
	$(MAKE) -f Makefile.freertos all
	cp $(SBB_DIR)/default_ballot_box.* .
	cp $(SBB_DIR)/ballot_box_* .

upload_binary_box1: all_boxes
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/ballot_box_1.elf --no-bitfile

upload_binary_box2: all_boxes
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/ballot_box_2.elf --no-bitfile

upload_binary_box3: all_boxes
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/ballot_box_3.elf --no-bitfile

upload_binary_box4: all_boxes
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/ballot_box_4.elf --no-bitfile

upload_binary_sim: sim
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/default_ballot_box_sim.elf --no-bitfile

upload_binary_and_bitstream_sim: sim
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/default_ballot_box_sim.elf

upload_binary_fpga: fpga
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/default_ballot_box.elf --no-bitfile

upload_binary_and_bitstream_fpga: fpga
	@echo GFE_DIR=$(GFE_DIR)
	@echo CURRENT_PATH=$(CURRENT_PATH)
	@echo P1_BITSTREAM_PATH=$(P1_BITSTREAM_PATH)
	cd $(GFE_DIR);	\
	./upload_flash_simple.sh $(P1_BITSTREAM_PATH) $(CURRENT_PATH)/default_ballot_box.elf

endif # ($(TARGET),deploy)
endif # ($(TARGET),sim)
endif # ($(TARGET),hosttests)
endif # ($(TARGET),verification)
endif # ($(TARGET),posix)
endif # ($(TARGET),freertos)
endif # ($(TARGET),bottom)

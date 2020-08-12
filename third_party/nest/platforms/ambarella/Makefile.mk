# Copyright 2019 Google LLC. All Rights Reserved.

include top.mk

OPENTHREAD_NAME     := openthread
OPENTHREAD_SRCDIR   := $(TOP)/$(dir)
OPENTHREAD_REPO     := $(OPENTHREAD_SRCDIR)/repo
OPENTHREAD_POSIX    := src/posix
OPENTHREAD_WORKDIR  := $(OUT)/$(OPENTHREAD_NAME)
OPENTHREAD_TARBALL  := $(OUT)/$(OPENTHREAD_NAME).tar.bz2
OPENTHREAD_CPPFLAGS := \
		-I${OPENTHREAD_REPO}/third_party/nest/platforms/ambarella \
		-I${OPENTHREAD_REPO}/third_party/nest/extension \
		-DOPENTHREAD_CONFIG_BORDER_AGENT_ENABLE=1 \
		-DOPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE=1 \
		-DOPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE=1 \
		-DOPENTHREAD_CONFIG_DIAG_ENABLE=1 \
		-DOPENTHREAD_CONFIG_DTLS_ENABLE=1 \
		-DOPENTHREAD_CONFIG_JAM_DETECTION_ENABLE=1 \
		-DOPENTHREAD_CONFIG_JOINER_ENABLE=1 \
		-DOPENTHREAD_CONFIG_LEGACY_ENABLE=1 \
		-DOPENTHREAD_CONFIG_MAC_FILTER_ENABLE=1 \
		-DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1 \
		-DOPENTHREAD_CONFIG_TMF_NETDATA_SERVICE_ENABLE=1 \
		-DOPENTHREAD_CONFIG_UDP_FORWARD_ENABLE=1 \
		-DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\<openthread-core-ambarella-config.h\>
ALL += $(OPENTHREAD_TARBALL)

$(OPENTHREAD_REPO)/configure: $(OPENTHREAD_REPO)/configure.ac $(OPENTHREAD_REPO)/Makefile.am $(OPENTHREAD_REPO)/bootstrap
	(cd $(OPENTHREAD_REPO) && \
		./bootstrap && \
		touch configure \
	)

$(OPENTHREAD_WORKDIR)/config.status: $(OPENTHREAD_REPO)/configure
	@echo 1 prereqs that are newer than $@: $?
	mkdir -p $(OPENTHREAD_WORKDIR)
	(cd $(OPENTHREAD_WORKDIR) && \
	$(OPENTHREAD_REPO)/configure \
		CC=$(TOOL_CC) \
		CXX=$(TOOL_CXX) \
		CFLAGS="-mtune=$(TOOL_TUNE) -march=$(TOOL_ARCH)" \
		CXXFLAGS="-mtune=$(TOOL_TUNE) -march=$(TOOL_ARCH)" \
		CPPFLAGS="$(OPENTHREAD_CPPFLAGS)" \
		--host=$(TOOL_HOST) \
		--enable-cli \
		--enable-ftd \
		--enable-ncp \
		--with-platform=posix \
		--with-vendor-extension=$(OPENTHREAD_REPO)/third_party/nest/extension/ot-legacy-pairing-ext.cpp \
	)

ifeq ($(USE_PREBUILTS),1)
$(OPENTHREAD_TARBALL): $(PREBUILTS_MANIFEST)
	(cd $(PREBUILTS_OPENTHREAD) && tar cjf $@ fakeroot)
else
$(OPENTHREAD_TARBALL): $(OPENTHREAD_WORKDIR)/config.status
	@echo 2 prereqs that are newer than $@: $?
	$(MAKE) V=$(BUILD_VERBOSE) -C "$(OPENTHREAD_WORKDIR)"
	-rm -rf "$(OPENTHREAD_WORKDIR)/fakeroot"
	mkdir -p $(OPENTHREAD_WORKDIR)/fakeroot/usr/sbin
	cp $(OPENTHREAD_WORKDIR)/$(OPENTHREAD_POSIX)/ot-ncp $(OPENTHREAD_WORKDIR)/fakeroot/usr/sbin
	cp $(OPENTHREAD_WORKDIR)/$(OPENTHREAD_POSIX)/ot-cli $(OPENTHREAD_WORKDIR)/fakeroot/usr/sbin
	tar cjf "$@" -C "$(OPENTHREAD_WORKDIR)" fakeroot
endif # USE_PREBUILTS

.PHONY: openthread
openthread: $(OPENTHREAD_TARBALL)

include bottom.mk

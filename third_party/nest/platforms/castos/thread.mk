# Copyright 2019 Google LLC. All Rights Reserved.

OPENTHREAD_PROJECT_CFLAGS = -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-castos-config.h\"
OPENTHREAD_PROJECT_INCLUDES = external/openthread/third_party/nest/platforms/castos
OPENTHREAD_PROJECT_SRC_FILES = third_party/nest/extension/ot-legacy-pairing-ext.cpp

OTBR_PROJECT_CFLAGS = -DOTBR_ENABLE_UNSECURE_JOIN=1

USE_OT_RCP_BUS = spi
USE_OTBR_DAEMON := 1
OTBR_MDNS := mojo

PRODUCT_PACKAGES += \
  ncp-ctl \
  ncp_spi_loader \
  ot-ncp-app.bin \
  otbr-agent \
  otbr-agent.conf \
  thread-defines \
  $(NULL)

ifeq ($(USE_OTBR_DAEMON), 1)
PRODUCT_PACKAGES += \
  ot-ctl \
  otbr-agent-launch.sh \
  $(NULL)
else
PRODUCT_PACKAGES += \
  ot-ncp \
  ot-cli \
  spi-hdlc-adapter \
  wpanctl \
  wpantund \
  wpantund-launch.sh \
  $(NULL)
endif

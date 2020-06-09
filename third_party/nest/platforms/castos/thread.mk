# Copyright 2019 Google LLC. All Rights Reserved.

OPENTHREAD_PROJECT_CFLAGS = -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"openthread-core-castos-config.h\"
OPENTHREAD_PROJECT_INCLUDES = external/openthread/third_party/nest/platforms/castos
OPENTHREAD_PROJECT_SRC_FILES = third_party/nest/extension/ot-legacy-pairing-ext.cpp

USE_OTBR_DAEMON := 1

PRODUCT_PACKAGES += \
  ncp-ctl \
  ncp_spi_loader \
  ot-ncp \
  ot-ncp-app.bin \
  otbr-agent \
  otbr-agent.conf \
  spi-hdlc-adapter \
  thread-defines \
  $(NULL)

ifeq ($(USE_OTBR_DAEMON), 1)
PRODUCT_PACKAGES += \
  ot-ctl \
  otbr-agent-launch.sh \
  $(NULL)
else
PRODUCT_PACKAGES += \
  ot-cli \
  wpanctl \
  wpantund \
  wpantund-launch.sh \
  $(NULL)
endif

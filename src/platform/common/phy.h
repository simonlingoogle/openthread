/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    @author  Jonathan Hui <jonhui@nestlabs.com>
 *
 */

#ifndef PLATFORM_COMMON_PHY_H_
#define PLATFORM_COMMON_PHY_H_

#ifdef CPU_KW2X
#include "platform/kw2x/phy.h"
#elif CPU_KW4X
#include "platform/kw4x/phy.h"
#elif CPU_DA15100
#include "platform/da15100/phy.h"
#elif CPU_EM35X
#include "platform/em35x/phy.h"
#else
#include "platform/posix/phy.h"
#endif

#endif  // PLATFORM_COMMON_PHY_H_

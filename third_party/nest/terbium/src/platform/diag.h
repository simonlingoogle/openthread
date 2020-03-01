/*
 *  Copyright (c) 2020 Google LLC.
 *  All rights reserved.
 *
 *  This document is the property of Google LLC, Inc. It is
 *  considered proprietary and confidential information.
 *
 *  This document may not be reproduced or transmitted in any form,
 *  in whole or in part, without the express written permission of
 *  Google LLC.
 *
 */

#ifndef PLATFORM_DIAG_H__
#define PLATFORM_DIAG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function outputs characters to diag response buffer.
 *
 * @param[in]  aFormat  A pointer to the format string.
 * @param[in]  ...      Arguments for the format specification.
 *
 * @return The number of output bytes.
 *
 */
int tbDiagResponseOutput(const char *aFormat, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_DIAG_H__

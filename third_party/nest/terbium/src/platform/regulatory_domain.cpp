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

/*
 * @file
 *   This file implements an interface for regulatory domains.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <common/code_utils.hpp>

#include "platform/regulatory_domain.h"

otError tbRegulatoryDomainDecode(const uint16_t aCode, char *aCodeString, uint16_t aCodeStringSize)
{
    otError error = OT_ERROR_INVALID_ARGS;

    VerifyOrExit((aCodeString != NULL) && (aCodeStringSize >= TB_REGULATORY_DOMAIN_STRING_SIZE), OT_NOOP);

    aCodeString[0] = (aCode >> 8) & 0xFF;
    aCodeString[1] = (aCode >> 0) & 0xFF;
    aCodeString[2] = '\0';

    error = OT_ERROR_NONE;

exit:
    return error;
}

uint16_t tbRegulatoryDomainEncode(const char *aCodeString)
{
    uint16_t code = TB_REGULATORY_DOMAIN_UNKNOWN;

    if ((aCodeString != NULL) && (strlen(aCodeString) >= TB_REGULATORY_DOMAIN_STRING_LENGTH))
    {
        code = TB_MAKE_REGULATORY_DOMAIN(aCodeString[0], aCodeString[1]);
    }

    return code;
}

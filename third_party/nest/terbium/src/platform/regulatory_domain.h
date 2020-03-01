/*
 *  Copyright (c) 2020 Google LLC
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
 *   This file includes definitions for regulatory domains.
 *
 */

#ifndef PLATFORM_REGULATORY_DOMAIN_H__
#define PLATFORM_REGULATORY_DOMAIN_H__

#include <stdint.h>
#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_REGULATORY_DOMAIN_STRING_LENGTH 2 ///< The length of the regulatory domain code string.

#define TB_REGULATORY_DOMAIN_STRING_SIZE \
    (TB_REGULATORY_DOMAIN_STRING_LENGTH + 1) ///< The size of the regulatory domain code string.

/**
 * This macro combines two bytes together to make a regulatory domain code.
 *
 * @param[in] aUpper  The upper byte of the regulatory domain code.
 * @param[in] aLower  The lower byte of the regulatory domain code.
 *
 * @returns The regulatory domain code.
 *
 */
#define TB_MAKE_REGULATORY_DOMAIN(aUpper, aLower) ((((aUpper)&0xFF) << 8) | (((aLower)&0xFF) << 0))

/**
 * This enumeration defines commonly used regulatory domain codes.
 */
enum
{
    ///< Unknown Domain.
    TB_REGULATORY_DOMAIN_UNKNOWN = TB_MAKE_REGULATORY_DOMAIN('?', '?'),

    ///< Worldwide Domain.
    TB_REGULATORY_DOMAIN_WORLDWIDE = TB_MAKE_REGULATORY_DOMAIN('0', '0'),

    ///< Canada - Industry Canada.
    TB_REGULATORY_DOMAIN_AGENCY_IC = TB_MAKE_REGULATORY_DOMAIN('A', '0'),

    ///< Europe - European Telecommunications Standards Institute.
    TB_REGULATORY_DOMAIN_AGENCY_ETSI = TB_MAKE_REGULATORY_DOMAIN('A', '1'),

    ///< United States - Federal Communications Commission.
    TB_REGULATORY_DOMAIN_AGENCY_FCC = TB_MAKE_REGULATORY_DOMAIN('A', '2'),

    ///< Japan - Ministry of Internal Affairs and Communications.
    TB_REGULATORY_DOMAIN_AGENCY_ARIB = TB_MAKE_REGULATORY_DOMAIN('A', '3'),
};

/**
 * This function dencodes regulatory domain code into regulatory domain code string.
 *
 * @param[in]  aCode           The regulatory domain code.
 * @param[out] aCodeString     The regulatory domain code string.
 * @param[in]  aCodeStringSize The size of the @p aCodeString.
 *
 * @retval OT_ERROR_NONE           Successfully decoded regulatory domain code.
 * @retval OT_ERROR_INVALID_ARGS   The @p aCodeString is NULL or the aBufferSize is smaller than
 *                                 (TB_REGULATORY_DOMAIN_SIZE + 1).
 *
 */
otError tbRegulatoryDomainDecode(const uint16_t aCode, char *aCodeString, uint16_t aCodeStringSize);

/**
 * This function encodes regulatory domain code string into regulatory domain code.
 *
 * @param[in] aCodeString  The regulatory domain code string.
 *
 * @returns The regulatory domain code. If the @p aCodeString can't be encoded into regulatory domain code,
 *          return TB_REGULATORY_DOMAIN_UNKNOWN.
 *
 */
uint16_t tbRegulatoryDomainEncode(const char *aCodeString);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_REGULATORY_DOMAIN_H__

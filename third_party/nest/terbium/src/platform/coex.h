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

#ifndef PLATFORM_COEX_H__
#define PLATFORM_COEX_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration defines the coex radio errors.
 *
 */
typedef enum
{
    TB_COEX_RADIO_ERROR_NONE           = 0, ///< None.
    TB_COEX_RADIO_ERROR_GRANT_TIMEOUT  = 1, ///< Coex grant is not active due to timeout.
    TB_COEX_RADIO_ERROR_RADIO_DISABLED = 2, ///< Radio driver is disabled.
} tbCoexRadioError;

/**
 * This method initializes radio coex. It is called from radio driver when it
 * is being initialized.
 */
extern void tbCoexRadioInit(void);

/**
 * This methods starts radio coex. It is called from radio driver when it is
 * exiting sleep.
 */
extern void tbCoexRadioStart(void);

/**
 * This methods stops radio coex. It is called from radio driver when it is
 * going to enter sleep mode.
 */
extern void tbCoexRadioStop(void);

/*
 * This method is called from the radio driver during frame reception after
 * address filter match. If the request is not immediately granted then the
 * coex driver will start a timeout timer waiting for grant to become active.
 * This coex driver will always indicate that the RX frame can proceed by
 * returning true. It does nothing to stop or otherwise block the reception
 * of an RX frame when the request was not immediately granted.
 *
 * @returns TRUE if the request was immediately granted, FALSE otherwise.
 *
 */
extern bool tbCoexRadioRxRequest(void);

/*
 * This method is called from the radio driver during frame reception before
 * transmitting an ACK response. If grant is not already active the request
 * will be denied in which case the radio driver will not send the ACK but the
 * RX frame will be propogated up to the radio driver client. If grant is
 * currently active the request will be approved and the radio driver will
 * proceed with transmission of the ACK frame. The coex driver will set state
 * to indicate that an ACK frame transmission is in progress.
 *
 * @returns TRUE if the grant is currently active, FALSE otherwise.
 *
 */
extern bool tbCoexTxAckRequest(void);

/*
 * This method is called from the radio driver after CCA backoff but before
 * frame transmission. If the request is not immediately granted then the
 * return value will be false and the coex driver will hold the tx frame
 * pointed to by @p aFrame until either the grant line becomes active or until
 * a configurable timeout expires. If the request is immediately granteda then
 * the return value will be true indicating to the caller that it can proceed
 * with the transmission of the tx frame.
 *
 * @param[in]  aFrame  A pointer to PSDU of the frame to transmit.
 *
 * @returns TRUE if the request was immediately granted, FALSE otherwise.
 *
 */
extern bool tbCoexRadioTxRequest(const uint8_t *aFrame);

/*
 * This method checks whether the grant line is active or not. It is called
 * from the radio driver at various gatekeeping points.
 *
 * @returns TRUE if grant is active or if coex is not enabled, FALSE otherwise.
 *
 */
extern bool tbCoexRadioIsGrantActive(void);

/*
 * This method is called from the radio driver when it enters a critical section
 * to protect radio register access.  While in the critical section grant
 * activations and deactivations will be delayed until the exit from the
 * critical section.
 *
 */
extern void tbCoexRadioCriticalSectionEnter(void);

/*
 * This method is called from the radio driver when it exits a critical
 * section to protect radio register access.  While in the critical section
 * grant activations and deactivations will have been delayed until the exit
 * from the critical section so those coex events are handled here.
 */
extern void tbCoexRadioCriticalSectionExit(void);

/*
 * This method is called from the radio driver at all RX termination points.
 *
 * @param[in] aSuccess  TRUE if rx frame was successfully received, FALSE otherwise.
 *
 */
extern void tbCoexRadioRxEnded(bool aSuccess);

/*
 * This method is called from the radio driver at all TX termination points.
 *
 * @param[in] aSuccess  TRUE if rx frame was successfully transmitted, FALSE otherwise.
 *
 */
extern void tbCoexRadioTxEnded(bool aSuccess);

/*
 * This method is called for processing coex radio unit test.
 *
 * @note This method is used when feature TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE is enabled.
 *
 */
extern void tbCoexRadioUnitTestProcess(void);

/**
 * This method restarts the radio to abort anything it was doing and put it
 * into a known state.
 *
 */
void tbCoexRadioRestart(void);

/**
 * This method starts CSMA-CA procedure for transmission of given frame.
 * First attempt happens immediately without backoff.
 *
 * @param[in]  aFrame  A pointer to PSDU of frame that should be transmitted.
 *
 */
void tbCoexRadioCsmaCaTransmitStart(const uint8_t *aFrame);

/*
 * This method stops the CSMA/CA procedure.
 *
 */
void tbCoexRadioCsmaCaTransmitStop(void);

/**
 * This method notifies radio driver that the frame was not transmitted.
 *
 * @param[in]  aFrame  A pointer to PSDU of the frame that failed transmit
 *                     operation.
 * @param[in]  aError  An error code indicating reason of the failure.
 *
 */
void tbCoexRadioTransmitFailed(const uint8_t *aFrame, tbCoexRadioError aError);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_COEX_H__

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

#ifndef THREAD_ERROR_H_
#define THREAD_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

enum ThreadError
{
    kThreadError_None = 0,
    kThreadError_Error = 1,
    kThreadError_Drop = 2,
    kThreadError_NoBufs = 3,
    kThreadError_NoRoute = 4,
    kThreadError_Busy = 5,
    kThreadError_Parse = 6,
    kThreadError_InvalidArgs = 7,
    kThreadError_Security = 8,
    kThreadError_LeaseQuery = 9,
    kThreadError_NoAddress = 10,
    kThreadError_NotReceiving = 11,
    kThreadError_Abort = 12,
};

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // THREAD_ERROR_H_

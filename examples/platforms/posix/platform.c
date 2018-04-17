/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include "platform-posix.h"

#if OPENTHREAD_POSIX_VIRTUAL_TIME == 0

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <libgen.h>
#include <syslog.h>
#endif

#include <openthread/openthread.h>
#include <openthread/tasklet.h>
#include <openthread/platform/alarm-milli.h>

uint32_t NODE_ID           = 1;
uint32_t WELLKNOWN_NODE_ID = 34;

extern bool gPlatformPseudoResetWasRequested;

#ifndef _WIN32
int    gArgumentsCount = 0;
char **gArguments      = NULL;
#endif

void PlatformInit(int aArgCount, char *aArgVector[])
{
    char *   endptr;
    uint32_t speedUpFactor = 1;

    if (gPlatformPseudoResetWasRequested)
    {
        gPlatformPseudoResetWasRequested = false;
        return;
    }

    if (aArgCount < 2)
    {
        fprintf(stderr, "Syntax:\n    %s NodeId [TimeSpeedUpFactor]\n", aArgVector[0]);
        exit(EXIT_FAILURE);
    }

#ifndef _WIN32
    openlog(basename(aArgVector[0]), LOG_PID, LOG_USER);
    setlogmask(setlogmask(0) & LOG_UPTO(LOG_NOTICE));

    gArgumentsCount = aArgCount;
    gArguments      = aArgVector;
#endif

    NODE_ID = (uint32_t)strtol(aArgVector[1], &endptr, 0);

    if (*endptr != '\0' || NODE_ID < 1 || NODE_ID >= WELLKNOWN_NODE_ID)
    {
        fprintf(stderr, "Invalid NodeId: %s\n", aArgVector[1]);
        exit(EXIT_FAILURE);
    }

    if (aArgCount > 2)
    {
        speedUpFactor = (uint32_t)strtol(aArgVector[2], &endptr, 0);

        if (*endptr != '\0' || speedUpFactor == 0)
        {
            fprintf(stderr, "Invalid value for TimerSpeedUpFactor: %s\n", aArgVector[2]);
            exit(EXIT_FAILURE);
        }
    }

    platformAlarmInit(speedUpFactor);
    platformRadioInit();
    platformRandomInit();
}

bool PlatformPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void PlatformDeinit(void)
{
    platformRadioDeinit();
}

void PlatformProcessDrivers(otInstance *aInstance)
{
    fd_set         read_fds;
    fd_set         write_fds;
    fd_set         error_fds;
    int            max_fd = -1;
    struct timeval timeout;
    int            rval;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&error_fds);

    platformUartUpdateFdSet(&read_fds, &write_fds, &error_fds, &max_fd);
    platformRadioUpdateFdSet(&read_fds, &write_fds, &max_fd);
    platformAlarmUpdateTimeout(&timeout);

    if (!otTaskletsArePending(aInstance))
    {
        rval = select(max_fd + 1, &read_fds, &write_fds, &error_fds, &timeout);

        if ((rval < 0) && (errno != EINTR))
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
    }

    platformUartProcess();
    platformRadioProcess(aInstance);
    platformAlarmProcess(aInstance);
}

#endif // OPENTHREAD_POSIX_VIRTUAL_TIME == 0

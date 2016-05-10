/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <platform/alarm.h>

static void *alarm_thread(void *arg);

static bool s_is_running = false;
static uint32_t s_alarm = 0;
static struct timeval s_start;

static pthread_t s_thread;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;

void hwAlarmInit(void)
{
    gettimeofday(&s_start, NULL);
    pthread_create(&s_thread, NULL, alarm_thread, NULL);
}

uint32_t otPlatAlarmGetNow(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    timersub(&tv, &s_start, &tv);

    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void otPlatAlarmStartAt(uint32_t t0, uint32_t dt)
{
    pthread_mutex_lock(&s_mutex);
    s_alarm = t0 + dt;
    s_is_running = true;
    pthread_mutex_unlock(&s_mutex);
    pthread_cond_signal(&s_cond);
}

void otPlatAlarmStop(void)
{
    pthread_mutex_lock(&s_mutex);
    s_is_running = false;
    pthread_mutex_unlock(&s_mutex);
}

void *alarm_thread(void *arg)
{
    int32_t remaining;
    struct timeval tva;
    struct timeval tvb;
    struct timespec ts;

    while (1)
    {
        pthread_mutex_lock(&s_mutex);

        if (!s_is_running)
        {
            // alarm is not running, wait indefinitely
            pthread_cond_wait(&s_cond, &s_mutex);
            pthread_mutex_unlock(&s_mutex);
        }
        else
        {
            // alarm is running
            remaining = s_alarm - otPlatAlarmGetNow();

            if (remaining > 0)
            {
                // alarm has not passed, wait
                gettimeofday(&tva, NULL);
                tvb.tv_sec = remaining / 1000;
                tvb.tv_usec = (remaining % 1000) * 1000;
                timeradd(&tva, &tvb, &tva);

                ts.tv_sec = tva.tv_sec;
                ts.tv_nsec = tva.tv_usec * 1000;

                pthread_cond_timedwait(&s_cond, &s_mutex, &ts);
                pthread_mutex_unlock(&s_mutex);
            }
            else
            {
                // alarm has passed, signal
                s_is_running = false;
                pthread_mutex_unlock(&s_mutex);
                otPlatAlarmSignalFired();
            }
        }
    }

    return NULL;
}

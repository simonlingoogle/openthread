#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import ipaddress
import logging
import time
import unittest
from typing import Union, List

import config
import network_layer
import thread_cert

logging.basicConfig(level=logging.DEBUG)

_, BBR_1, BBR_2, ROUTER_1_2, ROUTER_1_1, SED_1, MED_1, MED_2, FED_1 = range(9)

WAIT_ATTACH = 5
WAIT_REDUNDANCE = 3
ROUTER_SELECTION_JITTER = 1
BBR_REGISTRATION_JITTER = 5
SED_POLL_PERIOD = 1000  # 1s

REREG_DELAY = 10
MLR_TIMEOUT = 300
PARENT_AGGREGATE_DELAY = 5

MA1 = 'ff04::1234:777a:1'
MA1g = 'ff0e::1234:777a:1'
MA2 = 'ff05::1234:777a:2'
MA3 = 'ff0e::1234:777a:3'
MA4 = 'ff05::1234:777a:4'

MA5 = 'ff03::1234:777a:5'
MA6 = 'ff02::10'
"""
 Initial topology:

   BBR_1---BBR_2
     |     /   \                       |
     |    /     \                      |
   ROUTER_1_2  ROUTER_1_1
    |     |  \____                     |
    |     |       \                    |
  SED_1  MED_1/2  FED_1
"""


class TestMulticastListenerRegistration(thread_cert.TestCase):
    TOPOLOGY = {
        BBR_1: {
            'version': '1.2',
            'allowlist': [BBR_2, ROUTER_1_2],
            'is_bbr': True,
            'router_selection_jitter': 1,
        },
        BBR_2: {
            'version': '1.2',
            'allowlist': [BBR_1, ROUTER_1_2, ROUTER_1_1],
            'is_bbr': True,
            'router_selection_jitter': 1,
        },
        ROUTER_1_2: {
            'version': '1.2',
            'allowlist': [BBR_1, BBR_2, SED_1, MED_1, MED_2, FED_1],
            'router_selection_jitter': 1,
        },
        ROUTER_1_1: {
            'version': '1.1',
            'allowlist': [BBR_2],
            'router_selection_jitter': 1,
        },
        MED_1: {
            'mode': 'rn',
            'version': '1.2',
            'allowlist': [ROUTER_1_2],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        MED_2: {
            'mode': 'rn',
            'version': '1.2',
            'allowlist': [ROUTER_1_2],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        SED_1: {
            'mode': 'n',
            'version': '1.2',
            'allowlist': [ROUTER_1_2],
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
        FED_1: {
            'mode': 'rdn',
            'version': '1.2',
            'allowlist': [ROUTER_1_2],
            'router_upgrade_threshold': 0,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
        },
    }
    """All nodes are created with default configurations"""

    def _bootstrap(self, bbr_1_enable_backbone_router=True, turn_on_bbr_2=True, turn_on_router_1_1=True):
        assert (turn_on_bbr_2 or not turn_on_router_1_1)  # ROUTER_1_1 needs BBR_2

        # starting context id
        t0 = time.time()
        context_id = 1

        # Bring up BBR_1, BBR_1 becomes Leader and Primary Backbone Router
        self.nodes[BBR_1].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
        self.nodes[BBR_1].set_bbr_registration_jitter(BBR_REGISTRATION_JITTER)

        self.nodes[BBR_1].set_backbone_router(seqno=1, reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[BBR_1].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[BBR_1].get_state(), 'leader')

        if bbr_1_enable_backbone_router:
            self.nodes[BBR_1].enable_backbone_router()
            WAIT_TIME = BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE
            self.simulator.go(WAIT_TIME)
            self.assertEqual(self.nodes[BBR_1].get_backbone_router_state(), 'Primary')
            self.nodes[BBR_1].set_domain_prefix(config.DOMAIN_PREFIX)
            self.nodes[BBR_1].register_netdata()

        self.pbbr_seq = 1
        self.pbbr_id = BBR_1

        if turn_on_bbr_2:
            # Bring up BBR_2, BBR_2 becomes Router and Secondary Backbone Router
            self.nodes[BBR_2].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
            self.nodes[BBR_2].set_bbr_registration_jitter(BBR_REGISTRATION_JITTER)
            self.nodes[BBR_2].set_backbone_router(seqno=2, reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
            self.nodes[BBR_2].start()
            WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
            self.simulator.go(WAIT_TIME)
            self.assertEqual(self.nodes[BBR_2].get_state(), 'router')
            self.nodes[BBR_2].enable_backbone_router()
            WAIT_TIME = BBR_REGISTRATION_JITTER + WAIT_REDUNDANCE
            self.simulator.go(WAIT_TIME)
            self.assertEqual(self.nodes[BBR_2].get_backbone_router_state(), 'Secondary')

        # Bring up ROUTER_1_2
        self.nodes[ROUTER_1_2].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
        self.nodes[ROUTER_1_2].start()
        WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
        self.simulator.go(WAIT_TIME)
        self.assertEqual(self.nodes[ROUTER_1_2].get_state(), 'router')

        if turn_on_router_1_1:
            # Bring up ROUTER_1_1
            self.nodes[ROUTER_1_1].set_router_selection_jitter(ROUTER_SELECTION_JITTER)
            self.nodes[ROUTER_1_1].start()
            WAIT_TIME = WAIT_ATTACH + ROUTER_SELECTION_JITTER
            self.simulator.go(WAIT_TIME)
            self.assertEqual(self.nodes[ROUTER_1_1].get_state(), 'router')

        # Bring up FED_1
        self.nodes[FED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[FED_1].get_state(), 'child')

        # Bring up MED_1
        self.nodes[MED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[MED_1].get_state(), 'child')

        # Bring up MED_2
        self.nodes[MED_2].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[MED_2].get_state(), 'child')

        # Bring up SED_1
        self.nodes[SED_1].set_pollperiod(SED_POLL_PERIOD)
        self.nodes[SED_1].start()
        self.simulator.go(WAIT_ATTACH)
        self.assertEqual(self.nodes[SED_1].get_state(), 'child')

        logging.info("bootstrap takes %f seconds", time.time() - t0)
        self.nodes[BBR_1].add_ipmaddr(MA1)
        self.nodes[BBR_1].ping('fd00:7d03:7d03:7d03::abcd')
        self.simulator.go(10)

    def test(self):
        self._bootstrap(turn_on_router_1_1=False)


if __name__ == '__main__':
    unittest.main()

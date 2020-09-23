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
import logging
import unittest

import config
import thread_cert
# Test description: verifies that the basic MLR feature works.
#
# Topology:
#   --------(eth)---------
#       |     |      |
#     PBBR---SBBR   HOST
#       \    /  \
#        \  /    \
#        ROUTER--LEADER
#       /  |  \
#      /   |   \
#    FED  MED  SED
#
from pktverify.packet_verifier import PacketVerifier

PBBR = 1
SBBR = 2
ROUTER = 3
LEADER = 4
FED = 5
MED = 6
SED = 7
HOST = 8


class TestMlr(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        PBBR: {
            'name': 'PBBR',
            'whitelist': [SBBR, ROUTER],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        SBBR: {
            'name': 'SBBR',
            'whitelist': [PBBR, ROUTER, LEADER],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        LEADER: {
            'name': 'ROUTER',
            'whitelist': [SBBR],
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        self.nodes[HOST].start()

        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[LEADER].get_state())

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()


if __name__ == '__main__':
    unittest.main()

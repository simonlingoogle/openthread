#!/usr/bin/python
#
#    Copyright 2016 Nest Labs Inc. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import time
import unittest

import node

LEADER = 1
ED = 2

class Cert_6_5_1_ChildResetSynchronize(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ED].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ED].set_panid(0xface)
        self.nodes[ED].set_mode('rsn')
        self.nodes[ED].set_timeout(3)
        self.nodes[ED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ED].enable_whitelist()

    def tearDown(self):
        for node in self.nodes.itervalues():
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ED].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[ED].stop()
        time.sleep(5)

        self.nodes[ED].set_timeout(100)
        self.nodes[ED].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[ED].stop()
        time.sleep(3)
        self.nodes[ED].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        addrs = self.nodes[ED].get_addrs()
        for addr in addrs:
            if addr[0:4] == 'fe80':
                self.nodes[LEADER].ping(addr)

if __name__ == '__main__':
    unittest.main()

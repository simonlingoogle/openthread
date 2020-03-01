#!/usr/bin/env python3
#
# Copyright (c) 2020 Google LLC.
# All rights reserved.
#
# This document is the property of Google LLC, Inc. It is
# considered proprietary and confidential information.
#
# This document may not be reproduced or transmitted in any form,
# in whole or in part, without the express written permission of
# Google LLC.

import os
import subprocess
import sys


def printHelp():
    print('Usage:')
    print(
        '  ./script/address2line.py <path-to-elf-file> <addrsss0> <address1> <address2> ...'
    )


def isAddr2lineInstalled():
    return subprocess.getstatusoutput('which arm-none-eabi-addr2line')[0] == 0


def address2Line(index, address, elfPath):
    command = 'arm-none-eabi-addr2line {} -e {} -f -C -s'.format(
        address, elfPath)
    output = subprocess.getoutput(command)
    rets = output.splitlines()

    print('{:<2} {:<30} {}'.format(index, rets[1], rets[0]))


def main(argv):
    argc = len(sys.argv)  # Number of parameters.

    if argc <= 2:
        printHelp()
        return

    # Check if the tool `arm-none-eabi-addr2line` exist.
    if isAddr2lineInstalled() != True:
        print('ERROR: arm-none-eabi-addr2line is not installed!')
        print(
            'Visit https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads to install it.'
        )
        return

    # Path to the ELF we want to parse.
    elfPath = sys.argv[1]

    # Check if the elf file exist.
    if os.path.isfile(elfPath) == False:
        print('File {} doesn\'t exist'.format(elfPath))
        return

    for i in range(2, len(sys.argv)):
        address2Line(i - 2, sys.argv[i], elfPath)


if __name__ == '__main__':
    main(sys.argv[1:])

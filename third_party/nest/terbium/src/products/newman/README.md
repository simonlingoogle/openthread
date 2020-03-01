# Terbium on Newman
---
OpenThread is enabled on [Newman][Newman] to provide home connectivity for other google products. OpenThread is working in [RCP mode][RCP Mode] on Newman. The core of OpenThread protocol lives on host processor, the OpenThread drivers and platform features are running on the 802.15.4 SOC. Terbium builds the RCP image for the 802.15.4 SOC.

The 802.15.2 SOC is [nrf52840][nRF52840] and FEM chip is SKY66403 on Newman.

[Newman]: https://goto.google.com/newman-dev
[RCP Mode]: https://openthread.io/platforms
[nRF52840]: https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52840

### Build Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

### Source Code
Donwload the source code from remote git repository.
```bash
$ git clone https://nest-internal.googlesource.com/tps/openthread
```

### Build
Run the following commands to build the firmware.
```bash
$ cd openthread/third_party/nest/terbium
$ git checkout terbium/master
$ ./scripts/cmake_build.sh newman developement
```
After a successful build, all images can be found in `<path-to-terbium>/output/newman/eng/images`.

### Format and Style

Reformat code and enforce code format and style.
```bash
$ scripts/make-pretty
```

Check the code style. This check must pass before a pull request is merged.
```bash
$ scripts/make-pretty check
```

`scripts/make-pretty` requires [clang-format v6.0.0](http://releases.llvm.org/download.html#6.0.0) for C/C++ and [yapf](https://github.com/google/yapf) for Python.

#### Build with cmake
```bash
$ cd openthread/third_party/nest/terbium
$ git checkout terbium/master
$ ./scripts/cmake-build newman developement
```
After a successful build, all images can be found in `<path-to-terbium>/output/newman/eng/images`.

Run the command `./scripts/cmake-build help` to see more compile options.

### Format and Style

Reformat code and enforce code format and style.
```bash
$ scripts/make-pretty
```

Check the code style. This check must pass before a pull request is merged.
```bash
$ scripts/make-pretty check
```

`scripts/make-pretty` requires [clang-format v6.0.0](http://releases.llvm.org/download.html#6.0.0) for C/C++ and [yapf](https://github.com/google/yapf) for Python.

### Update RCP Image
#### Copy RCP Image to Host Processor
##### If Newman was flashed with FCT Image
1. Connect the Newman to your machine with a USB cable.

2. Run the `adb push` command to copy the RCP image to the host processor.
```
$ adb push output/newman/eng/images/ot-rcp.bin /nestlabs/marble/ot-ncp-app.bin
```

3. Login to the host processor.
```
$ adb shell
/ #
```

##### If Newman was flashed with Non-FCT Image
1. Connect the Newman to your machine with [shortleash][shortleash-guide].

[shortleash-guide]: https://support.google.com/techstop/answer/2675487?hl=en

2. Get the IP address of the host processor.
```
$ arp -a
? (100.84.187.254) at 00:00:5e:00:01:65 [ether] on eno1
? (172.16.243.72) at 20:d8:0b:db:91:81 [ether] on enxc05627906998
```
The Newman's IP address is `172.16.243.72`.

3. Login to the host processor to enable the host processor to be able to write the received files to its file system.
```
$ ssh root@172.16.243.72
# mount -o rw,remount /
```

4. Run the `scp` command to copy the RCP image to the host processor.
```
$ scp output/newman/eng/images/ot-rcp.bin root@172.16.243.72:/nestlabs/marble/ot-ncp-app.bin
```

5. Login to the host processor.
```
$ ssh root@172.16.243.72
#
```

#### Update RCP Image of 802.15.4 SOC
1. After copy the RCP image to the host processor and login to the host processor, run the following commands to flash the RCP image into 802.15.4 SOC.
```
# stop wpantund
# ncp-ctl update
```

2. Check the RCP image versions.
```
# ncp-ctl check
```

3. Run `ncp-ctl cli` command to start OpenThread in cli mode for testing.
```
# ncp-ctl cli
> diag start
start diagnostics mode
status 0x00
Done
```

### Flashing & RTT Logging & GDB Debuging
The pins of Jtag are not exposed by default on Newman. If you want to use the Jlink to flash image, output RTT log or do GDB debuging on the 802.15.4 SOC, you should manually expose the Jtag pins and connect them to Jlink. Then you can refer [nrf52811dk page][nrf52811dk-page] to flash image, output RTT log or do GDB debugging.

[nrf52811dk-page]: ./../nrf52811dk/README.md

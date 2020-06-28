# Terbium on nRF52840dk
---
The nrf52840dk project is created to make it easier for us to develop and debug Terbium.

### Build Toolchain

Download and install the [GNU toolchain for ARM Cortex-M][gnu-toolchain].

[gnu-toolchain]: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

### Flashing and Debugging Tools

Install the [nRF Command Line Tools][nRF-Command-Line-Tools] to flash, debug, and make use of logging features on the nRF52840 DK with SEGGER J-Link.

[nRF-Command-Line-Tools]: https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools

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
$ ./scripts/cmake-build nrf52840dk eng
```
After a successful build, all images can be found in `<path-to-terbium>/output/nrf52840dk/eng/images`.

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

### Flash Image

1. Download bootloader binary `bootloader.hex` from [Marble Images Server][MarbleServerElaineImageFolder] or directly use the Terbium bootloader. The Terbium bootloader is under `<path-to-terbium>/output/nrf52840dk/eng/images`.
[MarbleServerElaineImageFolder]: https://images.nestlabs.com/images/Marble/Builds/1.0d998/Non-production/rcp_elaine1/

2. Combine the bootloader and application into one image.

For FTD
```
$ mergehex -m bootloader.hex ot-cli-ftd.hex -o combo.hex
```
For RCP
```
$ mergehex -m bootloader.hex ot-rcp.hex -o combo.hex
```

3. Run `nrfjprog` to get the SERIAL_NUMBER:
```
$ nrfjprog -i
```

4.Flash the combined binary onto nRF52840.
```
$ nrfjprog -f nrf52 -s <SERIAL_NUMBER> --chiperase --program combo.hex --reset
```

### SEGGER J-Link Tools
SEGGER J-Link tools allow to debug and flash generated firmware using on-board debugger or external one.

#### Working with RTT Logging
By default, the OpenThread's logging module provides functions to output logging
information over SEGGER's Real Time Transfer (RTT).

You can set the desired log level by using the `OPENTHREAD_CONFIG_LOG_LEVEL` define.

#### Enable RTT Logging

1. Connect the board to your machine with a USB cable.
2. Run `nrfjprog` to get the SERIAL_NUMBER:
```
$ nrfjprog -i
```

3. Run `JLinkExe` to connect to the target.
```
$JLinkExe -device NRF52810_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN <SERIAL_NUMBER> -RTTTelnetPort 19021
```

4. Run `JLinkRTTClient` to obtain the RTT logs from the connected device in a separate console.
```
$JLinkRTTClient -RTTTelnetPort 19021
```

### Debugging with GDB
SEGGER provides a GDB server tool which can be used to debug a running application on the nRF52840 DK board.

1. Connect the board to your machine with a USB cable.
2. Launch the GDB server.
```
$ JLinkGDBServer -device NRF52 -if swd -speed 1000 -Select USB=<SERIAL_NUMBER> -port 2331
```

3. Launch the GDB client in a separate console.

For FTD
```
$ arm-none-eabi-gdb  -tui  ./output/nrf52840dk/eng/images/ot-cli-ftd.elf
(gdb)
```
For RCP
```
$ arm-none-eabi-gdb  -tui  ./output/nrf52840dk/eng/images/ot-rcp.elf
(gdb)
```

4. Connect the GDB client to server.
```
(gdb) target remote localhost:2331
```

5. Enable the server for downloading image to the device.
```
(gdb) mon speed 10000
(gdb) mon flash download=1
```

6. Download the image to the device.
```
(gdb) load
```

7. At this point, user can run GDB commands to debug the application.

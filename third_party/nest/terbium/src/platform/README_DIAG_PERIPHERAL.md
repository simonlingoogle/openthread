# Peripheral Diagnostics Reference

Peripheral Diagnostics module is a tool for users to test peripherals.

## Command List

* [diag gpio get \<pinnum\>](#diag-gpio-get-pinnum)
* [diag gpio set \<pinnum\> \<value\>](#diag-gpio-set-pinnum-value)
* [diag gpio int \<pinnum\> \[rising | falling | both\]](#diag-gpio-int-pinnum-rising-falling-both)
* [diag gpio int \<pinnum\>](#diag-gpio-int-pinnum)
* [diag gpio int \<pinnum\> release](#diag-gpio-int-pinnum-release)
* [diag gpio loopback \<outputpinnum\> \<interruptpinnum\> \[rising | falling | both\]](#diag-gpio-loopback-outputpinnum-interruptpinnum-rising-falling-both)
* [diag vcc2 v\<vcc2voltagevalue\>](#diag-vcc2-vvcc2voltagevalue)
* [diag pwrIndex i\<fempowerindexvalue\> ](#diag-pwrindex-ifempowerindexvalue)
* [diag txPower \<regulatorycode\> \<targetpower\>](#diag-txpower-regulatorycode-targetpower)
* [diag cw start](#diag-cw-start)
* [diag cw stop](#diag-cw-stop)
* [diag transmit \<femmode\> \<fempower\> \<radiopower\> \<channel\> \<count\> \<delayms\> \<payload\>](#diag-transmit-femmode-fempower-radiopower-channel-count-delayms-payload)
* [diag transmit stop](#diag-transmit-stop)
* [diag watchdog](#diag-watchdog)
* [diag watchdog enable](#diag-watchdog-enable)
* [diag watchdog disable](#diag-watchdog-disable)
* [diag watchdog stop](#diag-watchdog-stop)
* [diag backtrace](#diag-backtrace)
* [diag backtrace memfault](#diag-backtrace-memfault)
* [diag backtrace usagefault](#diag-backtrace-usagefault)

### diag gpio get \<pinnum\>
Get current input value of the given pin.
```bash
> diag gpio get 16
0
Done
```

### diag gpio set \<pinnum\> \<value\>
Set gpio output value.
```bash
>  diag gpio set 16 0
Done
> diag gpio set 16 1
Done
```

### diag gpio int \<pinnum\> \[rising | falling | both\]
Set the given pin as an interrupt pin and record the number of interrupts generated at the specified edge on the interrupt pin.
```bash
> diag gpio int 19 falling
Done
```

### diag gpio int \<pinnum\>
Show the number of interrupts generated on the specified pin.
```bash
> diag gpio int 19
IntCounter: 2
Done
```

### diag gpio int \<pinnum\> release
Release the interrupt pin.
```bash
> diag gpio int 19 release
Done
```

### diag gpio loopback \<outputpinnum\> \<interruptpinnum\> \[rising | falling | both\]
Loopback testing. This command generates 1000 pulses on the output pin and records the number of interrupts generated
at the specified edge on the interrupt pin.

Note: User should connect the output pin and interrupt pin together before running this commnand.
```bash
> diag gpio loopback 16 19 rising
IntCounter: 1000
Done
>
```

### diag vcc2 v\<vcc2voltagevalue\>
Set FEM VCC2 value.

Note: This command is valid only when FEM supports setting VCC2 voltage.

```bash
> diag vcc2 v200
Set v200 voltage value succeeded: 200, 200
```

### diag pwrIndex i\<fempowerindexvalue\>
Set FEM power index value.

Note: This command is valid only when FEM supports setting power index.
```bash
> diag pwrIndex i2
Set i2 power index value succeeded: 2, 2
```

### diag txPower \<regulatorycode\> \<targetpower\>
Set regulatory code and target power for the radio driver to lookup power settings from wireless calibration table.
```bash
> diag txPower  16688 2000
Setting PowerCal override reg_code(16688) target_power(2000)
```

### diag cw start
Start to emit continuous carrier wave.
```bash
> diag cw start
Started transmitting CW on channel 20
```

### diag cw stop
Stop emiting continuous carrier wave.
```bash
> diag cw stop
Stopped transmitting CW on channel 20
```

### diag transmit \<femmode\> \<fempower\> \<radiopower\> \<channel\> \<count\> \<delayms\> \<payload\>
Transmit specified number of packets with the specified power on the specified channel.

- femmode : FEM mode.
- fempower : The FEM power setting, which represents VCC2 voltage value or FEM power index register value. If it represents VCC2 voltage value, the format is "vxxx" where 'v' is a character and 'xxx' is the FEM power setting value. If it represents power index value, the format is "ixxx" where 'i' is a character and 'xxx' is the FEM power setting.
- radiopower : The power coming out of the radio chip in dBm.
- channel : The channel to be used for packets transmission.
- count : The number of packets to transmit (-1 for infinite).
- delayms : The time in milliseconds between packets.
- payload : Payload is a string of hex digits, two characters per octet. The number of octets in the payload string is the length of the payload. A negative payload number means send a generated number of bytes as the payload instead of using the user specified payload.

```bash
> diag transmit 1 i10 -4 11 10 16 -64
sending 10 packets of length 64, of auto generated payload, channel 11, with delay of 16 ms
>
> diag transmit 1 i10 -8 11 10 16 12345678901234567890
sending 10 packets of length 12, of user provided payload, channel 11, with delay of 16 ms
```

### diag transmit stop
Stop transimtting packets.
```
> diag transmit stop
Stopped previous transmit operation
```

### diag watchdog
Show the state of the watchdog.
```
> diag watchdog
enabled
Done
```

### diag watchdog enable
Enable watchdog.
```
> diag watchdog enable
Done
```

### diag watchdog disable
Disable watchdog.
```
> diag watchdog disable
Done
```

### diag watchdog stop
Enter an infinite loop to wait for watchdog to reset device.
```
> diag watchdog stop
```

### diag backtrace
Output the backtrace of the diag command.
```
> diag backtrace
0x1f906 0x1fc8b 0x9c2b 0x60b3 0x6e81 0x5437 0x54b3 0x19f2b 0x19be1 0x5379 0x530f
Done
```

### diag backtrace memfault
Generate a memory management fault. The device will output the backtrace to the logging system and then reset the device.
```
> diag backtrace memfault
```

### diag backtrace usagefault
Generate a usage fault. The device will output the backtrace to the logging system and then reset the device.

```
> diag backtrace usagefault
```

# Factory Wireless Calibration Diagnostics Reference
Factory Wireless Calibration Diagnostics module is a tool for factory to get or set Wireless Calibration table.

## Command List

* [diag factory wirelesscal set \<power settings formats\>](#diag-factory-wirelesscal-set-power-settings-formats)
* [diag factory wirelesscal get](#diag-factory-wirelesscal-get)
* [diag factory wirelesscal get powersettings](#diag-factory-wirelesscal-get-powersettings)
* [diag factory wirelesscal apply -r \<regulatorydomaincode\> -c \<channel\> -a \<adjustmentpower\>](#diag-factory-wirelesscal-apply-r-regulatorycode-c-channel-a-adjustmentpower)

### diag factory wirelesscal set \<power settings formats\>

Set calibration data for discrete power calibrations.

Power Settings Formats:
```bash
  [-r <regulatorydomaincode> -t [<channelstart>,<channelcount>,<targetpower>]/...] ...
  [-b <channelstart>,<channelcount>,<settingcount> -s [<calibratedpower>,<radiopower>,<femmode>,<fempower>]/...] ...
```

- -r \<regulatorydomaincode\> : Defines the regulatory domain that will use this table. Wireless Calibration needs this because the IC, US and EU have very different power limitations due to country regulations.
    - regulatorydomaincode:
        - A0: Canada (DOC/IC)
        - A1: Europe (ETSI)
        - A2: USA    (FCC)

For example, "-r A2" means: the next option `-t` sets the sub-band target powers used by USA.

- -t \[\<channelstart\>,\<channelcount\>,\<targetpower\>\]\/... : Defines the sub-bands target power within the set of all channels.
    - channelstart: Sub-band start channel. Valid channel range : <11, 26>.
    - channelcount: The count of the channel from start channel.
    - targetpower : The target power in 0.01dBm.

For example, "-t 11,3,1100" means: the sub-band starts at channel 11, continues for 3 channels, and the target power for these channels is 11.00 dBm.

- -b \<channelstart\>,\<channelcount\>,\<settingcount\> : Defines the start of a new sub-band.
    - channelstart: Sub-band start channel. Valid channel range : <11, 26>.
    - channelcount: The count of the channel from start channel.
    - settingcount: The count of the power mappings.

For example, "-b 11,14,3" means: this sub-band starts at channel 11, continues for 14 channels total, and will have 3 power mappings associated with it.

- -s \[\<calibratedpower\>,\<radiopower\>,\<femmode\>,\<fempower>\]\/... : Defines the power mappings for the sub-band.
    - calibratedpower: The actual output power in 0.01dBm.
    - radiopower     : The power coming out of the radio chip in dBm.
    - femmode        : The FEM mode.
    - fempower       : The FEM power setting, which represents VCC2 voltage value or FEM power index register value.

For example, "-s 1000,-8,1,10" means: the calibrated power will be 10.00 dBm when RF radio power is set to -8dBm, FEM mode is set to 1 and FEM power is set to 10.

Note: The “-b” sub-band group entries' start channels must be monotonically increasing, and entries in the “-s” area must be monotonically decreasing.

```bash
> diag factory wirelesscal set -r A0 -t 11,16,1000 -r A1 -t 11,14,2000/25,2,1500 -b 11,14,3 -s 2000,0,1,20/1500,-4,1,15/1000,-8,1,10 -b 25,2,3 -s 2000,0,1,20/1500,-4,1,15/1000,-8,1,10
Done
>
```

### diag factory wirelesscal get
Show calibration data for discrete power calibrations.

```bash
> diag factory wirelesscal get
Calibration(6LoWPAN): Type 3
TargetPower:
  A0 FCC: [(11,16,1000)],
  A1 ETSI: [(11,14,2000), (25,2,1500)]
SubbandSettings:
  (11,14,3) [(2000,0dBm,1,i20), (1500,-4dBm,1,i15), (1000,-8dBm,1,i10)],
  (25,2,3) [(2000,0dBm,1,i20), (1500,-4dBm,1,i15), (1000,-8dBm,1,i10)]
In Set Syntax:
  -r A0 -t (11,16,1000)
  -r A1 -t (11,14,2000)/(25,2,1500)
  -b 11,14,3 -s (2000,0dBm,1,i20)/(1500,-4dBm,1,i15)/(1000,-8dBm,1,i10)
  -b 25,2,3 -s (2000,0dBm,1,i20)/(1500,-4dBm,1,i15)/(1000,-8dBm,1,i10)

Done
```

### diag factory wirelesscal get powersettings
Show power settings table.

```bash
> diag factory wirelesscal get powersettings

Power settings table in sysenv:
 Id=0 RadioPower=0, FemMode=1, FemPower=20
 Id=1 RadioPower=-4, FemMode=1, FemPower=15
 Id=2 RadioPower=-8, FemMode=1, FemPower=10
End
Done
```

### diag factory wirelesscal apply -r \<regulatorydomaincode\> -c \<channel\> -a \<adjustmentpower\>
Apply calibration parameters to given channel.
- regulatorydomaincode:
    - A0: Canada (DOC/IC)
    - A1: Europe (ETSI)
    - A2: USA    (FCC)
- channel : The channel to adjust the power.
- adjustmentpower : Adjustment power from the target power in 0.01 dBm.

For example, "-r A1 -c 11 -a -100" means: use regulatory domain code "A0" and Channel "11" to lookup the target power, then add adjustment power "-100" to target power as a new target power. The device uses the new target power to lookup the power mapping table to get power settings.

```bash
> diag factory wirelesscal apply -r A0 -c 11 -a 0
    Applied adjusted Target Power(1000) on channel(11)
    Please make sure to run openthread <diag channel (11)> cmd to set channel before transmitting
Done
```

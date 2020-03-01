# Factory Sysenv Diagnostics Reference

Factory Sysenv Diagnostics module is a tool for factory to get or set Sysenv.

## Command List

* [diag factory sysenv](#diag-factory-sysenv)
* [diag factory sysenv check](#diag-factory-sysenv-check)
* [diag factory sysenv set \<key\>](#diag-factory-sysenv-set-key)
* [diag factory sysenv set \<key\> \<string\>](#diag-factory-sysenv-set-key-string)
* [diag factory sysenv set --data \<key\> \<hex\>](#diag-factory-sysenv-set-data-key-hex)
* [diag factory sysenv set --int \<key\> \<integer\>](#diag-factory-sysenv-set-int-key-integer)
* [diag factory sysenv get \<key\>](#diag-factory-sysenv-get-key)
* [diag factory sysenv get --len \<key\>](#diag-factory-sysenv-get-len-key)
* [diag factory sysenv get --int \<key\>](#diag-factory-sysenv-get-int-key)
* [diag factory sysenv get --data \<key\>](#diag-factory-sysenv-get-data-key)
* [diag factory sysenv get --data=\[x|o|d\]\[1|2|4\] key](#diag-factory-sysenv-get-dataxod124-key)

### diag factory sysenv

Show all entries in sysenv.

```bash
> diag factory sysenv
begin sysenv dump
nlwirelessregdom=A0
hwaddr0=11:22:33:44:55:66:77:88
end sysenv dump
Done
```

### diag factory sysenv check

Show sysenv statistics.

```bash
> diag factory sysenv check
    bytes: 68
    max: 4092
    header: 4
    entries: 2
    key: 23
    value: 25
    meta: 16
Done
```

### diag factory sysenv set \<key\>

Delete the key-value entry for the given key from sysenv.

```bash
> diag factory sysenv set hwaddr0
successfully deleted key hwaddr0
Done
```

### diag factory sysenv set \<key\> \<string\>

Save the character string to sysenv.

```bash
> diag factory sysenv set hwaddr0 11:22:33:44:55:66:77:88
successfully set key hwaddr0: 11:22:33:44:55:66:77:88
Done
```

### diag factory sysenv set --data \<key\> \<hex\>

Save the hex data to sysenv.

```bash
> diag factory sysenv set --data extaddr dead00beef00cafe
successfully set key extaddr: dead00beef00cafe
Done
```

### diag factory sysenv set --int \<key\> \<integer\>

Save the integer to sysenv.

```bash
> diag factory sysenv set --int channel 11
successfully set key channel: 11
Done
```

### diag factory sysenv get \<key\>

Show the value of the key.

```bash
> diag factory sysenv get hwaddr0
11:22:33:44:55:66:77:88
Done
```

### diag factory sysenv get --len \<key\>

Show the value of the key and the length of the value.

```bash
> diag factory sysenv get --len hwaddr0
11:22:33:44:55:66:77:88
[length:23]
Done
```

### diag factory sysenv get --int \<key\>

Show the value of the key in integer format.

```bash
> diag factory sysenv get --int channel
11
Done
```

### diag factory sysenv get --data \<key\>

Show the value of the key in hex format.

```bash
> diag factory sysenv get --data extaddr

0000: de ad 00 be ef 00 ca fe
Done
```

### diag factory sysenv get --data=\[x|o|d\]\[1|2|4\] \<key\>

Show the value of the key in given format.

Display format:

- x : Hexadecimal format.
- o : Octal format.
- d : Integer format.

Format length:

- 1 : Show 1 byte as a group.
- 2 : Show 2 byte as a group.
- 4 : Show 4 bytes as a group.

```bash
> diag factory sysenv get --data=x1 extaddr

0000: de ad 00 be ef 00 ca fe
Done
> diag factory sysenv get --data=o2 extaddr

0000: 126736 137000 0357 177312
Done
>
> diag factory sysenv get --data=d4 extaddr

0000: -1107251746 -20315921
Done
```

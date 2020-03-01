# Backtrace Reference

A backtrace is the series of currently active function calls for the program.

## Usage Examples

1. Use the diag command to show the backtrace of the diag command.
```
> diag backtrace
0x1f906 0x1fc8b 0x9c2b 0x60b3 0x6e81 0x5437 0x54b3 0x19f2b 0x19be1 0x5379 0x530f
Done
```

Translate addresses into file names, line numbers and functions.
```
$ ./scripts/address2line.py ./output/nrf52811dk/eng/images/ot-cli-mtd.elf 0x1f906 0x1fc8b 0x9c2b 0x60b3 0x6e81 0x5437 0x54b3 0x19f2b 0x19be1 0x5379 0x530f
0  backtrace.cpp:511              tbBacktrace
1  diag_peripheral.cpp:647        processBacktrace(otInstance*, int, char**)
2  factory_diags.cpp:573          ot::FactoryDiags::Diags::ProcessCmd(int, char**, char*, unsigned int)
3  cli.cpp:3629                   ot::Cli::Interpreter::ProcessDiag(int, char**)
4  cli.cpp:3666                   ot::Cli::Interpreter::ProcessLine(char*, unsigned short, ot::Cli::Server&)
5  cli_uart.cpp:245               ot::Cli::Uart::ProcessCommand()
6  cli_uart.cpp:156               ot::Cli::Uart::ReceiveTask(unsigned char const*, unsigned short)
7  uart.c:115                     processReceive
8  system.c:219                   otSysProcessDrivers
9  main.c:114                     main
10 :?                             _start
```

2. When a hard fault occurs, the hard fault handler will output the backtrace of the fault.
```
[0000013615] -PLAT----: dumpBacktrace: OPENTHREAD/1.0d38-04399-gd973194d0-dirty; NRF52811dk-eng; Apr 21 2020 10:59:31
[0000013615] -PLAT----: dumpBacktrace: 0x1f906 0x1a5f9 0x1a6b9 0x1a9ad 0x1fcf5 0x9c2b 0x60b3 0x6e81 0x5437 0x54b3 0x19f2b 0x19be1 0x5379 0x530f
```

Translate addresses into file names, line numbers and functions.
```
$ ./scripts/address2line.py ./output/nrf52811dk/eng/images/ot-cli-mtd.elf 0x1f906 0x1a5f9 0x1a6b9 0x1a9ad 0x1fcf5 0x9c2b 0x60b3 0x6e81 0x5437 0x54b3 0x19f2b 0x19be1 0x5379 0x530f
0  backtrace.cpp:511              tbBacktrace
1  fault.c:358                    dumpBacktrace
2  fault.c:399                    crashDump
3  fault.c:457                    UsageFault_Handler
4  diag_peripheral.cpp:670        processBacktrace
5  factory_diags.cpp:573          ot::FactoryDiags::Diags::ProcessCmd(int, char**, char*, unsigned int)
6  cli.cpp:3629                   ot::Cli::Interpreter::ProcessDiag(int, char**)
7  cli.cpp:3666                   ot::Cli::Interpreter::ProcessLine(char*, unsigned short, ot::Cli::Server&)
8  cli_uart.cpp:245               ot::Cli::Uart::ProcessCommand()
9  cli_uart.cpp:156               ot::Cli::Uart::ReceiveTask(unsigned char const*, unsigned short)
10 uart.c:115                     processReceive
11 system.c:219                   otSysProcessDrivers
12 main.c:114                     main
13 :?                             _start
```

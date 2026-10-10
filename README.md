# PicoRV32 fault injection campaign

This project builds a Verilator simulation of the PicoRV32 core and runs a
RISC-V program through it, injecting soft errors

## Requirements

- GNU Make
- Verilator
- `riscv64-unknown-elf-gcc` and `riscv64-unknown-elf-objcopy`

## Build

```sh
make run
```

```sh
make PROGRAM=programs/my_program run
```
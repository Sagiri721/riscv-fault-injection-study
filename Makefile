PROGRAM ?= programs/sum
RISCV_GCC ?= riscv64-unknown-elf-gcc
RISCV_OBJCOPY ?= riscv64-unknown-elf-objcopy
VERILATOR ?= verilator
BUILD_DIR ?= build

RTL := vendor/picorv32/picorv32.v
HARNESS_SRC := src/harness.cpp
HARNESS := $(BUILD_DIR)/obj_dir/Vpicorv32
WAVE := debug/wave.vcd

.PHONY: all run build_program build_harness clean

all: build_program build_harness

run: build_program build_harness
	$(HARNESS) $(PROGRAM).bin

build_harness: $(HARNESS)

$(BUILD_DIR):
	mkdir -p $@

$(HARNESS): $(RTL) $(HARNESS_SRC) src/fault.h src/runtime.h Makefile | $(BUILD_DIR)
	$(VERILATOR) --cc $(RTL) --top-module picorv32 --Mdir $(BUILD_DIR)/obj_dir \
		--exe $(abspath $(HARNESS_SRC)) --build -O3 -Wno-fatal \
		-GSTACKADDR=65536 -GREGS_INIT_ZERO=1 --public-flat-rw --trace

build_program: $(PROGRAM).bin

$(PROGRAM).elf: $(PROGRAM).s
	$(RISCV_GCC) -march=rv32i -mabi=ilp32 -nostdlib -nostartfiles \
		-Wl,-Ttext=0 -o $@ $<

$(PROGRAM).bin: $(PROGRAM).elf
	$(RISCV_OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR) obj_dir
	rm -f $(PROGRAM).elf $(PROGRAM).bin $(WAVE)

CC     = riscv64-elf-gcc
CFLAGS = -march=rv64gc -mabi=lp64d -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -Wall -Wextra -Wpedantic -Tkernel.ld -Iinclude/
CFLAGS += -g -O1
CODE   = src/

GDB    = riscv64-elf-gdb

EMU    = qemu-system-riscv64
CORES  = 1
EDEVICES =
# EDEVICES += -device virtio-blk-device,scsi=off,drive=root
EFLAGS = -machine virt -cpu rv64 -bios opensbi-riscv64-generic-fw_dynamic.bin -m 256m -global virtio-mmio.force-legacy=false $(EDEVICES) -smp $(CORES) -s

ifdef WAIT_GDB
	EFLAGS += -S
endif
ifdef GRAPHIC
	EFLAGS += -serial stdio -device vga
else
	EFLAGS += -nographic
endif

.PHONY: all clean run gdb

all: $(CODE)asm/*.s $(CODE)*.c $(CODE)drivers/*.c
	$(CC) $(CFLAGS) $? -o kernel

run: all
	$(EMU) $(EFLAGS) -kernel kernel

gdb:
	$(GDB) -q -x kernel.gdb

clean:
	-@rm kernel 2>/dev/null || true

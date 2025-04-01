# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Project files
SRCS = bus.c dev.c guest.c main.c pci.c serial.c virtio_pci.c vm.c virtq.c virtio-blk.c diskimg.c
HDRS = bus.h dev.h guest.h pci.h serial.h serial_dev.h serial_dev_priv.h utils.h virtio_pci.h vm.h virtq.h virtio-blk.h diskimg.h
OBJS = $(SRCS:%.c=build/%.o)


# Output binary
TARGET = build/main

build/myfs.ext4:
	$(Q)dd if=/dev/zero of=$@ bs=4k count=600
	$(Q)mkfs.ext4 -F $@

# Default target
all: $(TARGET) build/myfs.ext4

# Create build directory
build:
	mkdir -p build

# Compile object files
build/%.o: %.c $(HDRS) | build
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into the final executable
$(TARGET): $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Clean up generated files
clean:
	rm -rf build

# Run the project
run: $(TARGET)
	./$(TARGET) bzImages/bzImage build/myfs.ext4
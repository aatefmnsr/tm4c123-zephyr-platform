BOARD ?= tm4c123gxl
BUILD_DIR ?= build

.PHONY: all build clean pristine flash flash-only menuconfig guiconfig

all: build

build:
	./build.sh -b $(BOARD)

pristine:
	./build.sh -p -b $(BOARD)

flash:
	./build.sh -f -b $(BOARD)

flash-only:
	./build.sh --flash-only -b $(BOARD)

menuconfig:
	./build.sh -m -b $(BOARD)

guiconfig:
	./build.sh -g -b $(BOARD)

clean:
	./build.sh -c


PROJECT := Roberto
DEBUG_PORT ?= 4242
BUILD_DIR ?= cmake-build-debug

default_target: build
.PHONY: _flash flash debug-server debug build

${BUILD_DIR}:
	cmake -S . -B ${BUILD_DIR}

build: cmake-build-debug
	cmake --build ${BUILD_DIR} --target ${PROJECT} -- -j 12
	arm-none-eabi-objcopy -O binary cmake-build-debug/${PROJECT} ${PROJECT}.bin

_flash: build
	st-flash write ${PROJECT}.bin 0x8000000

flash:
	# flash sometimes fails the first time.
	${MAKE} _flash || ${MAKE} _flash

debug-server:
	st-util -p ${DEBUG_PORT}

debug:
	gdb-multiarch -ex "target remote localhost:${DEBUG_PORT}" -ex "symbol-file ${BUILD_DIR}/${PROJECT}"


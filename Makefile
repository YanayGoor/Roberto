PROJECT := Roberto
DEBUG_PORT ?= 4242
BUILD_DIR ?= cmake-build-debug

SOURCES += $(shell find . -name '*.c' -o -name '*.h')
EXCLUDE_DIRS += ./stm32f4/%
EXCLUDE_DIRS += ./include/stm32f4/%
EXCLUDE_DIRS += ./$(BUILD_DIR)/%

default_target: all
.PHONY: _flash flash debug-server debug build format lint all debug-exit

all: format lint build

format:
	clang-format -i $(filter-out $(EXCLUDE_DIRS), $(SOURCES))

lint:
	clang-tidy --extra-arg="-Iinclude"  $(filter-out $(EXCLUDE_DIRS), $(SOURCES))

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

debug: flash
	st-util -p ${DEBUG_PORT} &
	gdb-multiarch -ex "set confirm off" -ex "target remote localhost:${DEBUG_PORT}" -ex "symbol-file ${BUILD_DIR}/${PROJECT}"

debug-exit: flash
	st-util -p ${DEBUG_PORT} &
	gdb-multiarch -ex "set confirm off" -ex "target remote localhost:${DEBUG_PORT}" -ex "symbol-file ${BUILD_DIR}/${PROJECT}" -ex "b done" -ex "c" -ex "frame 1" -ex "info locals" -ex "q"
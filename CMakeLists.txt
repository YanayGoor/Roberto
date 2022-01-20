cmake_minimum_required(VERSION 3.10)

project(Roberto VERSION 0.1.0)

configure_file(config.h.in config.h)

add_executable(Roberto
    roberto.c
    stm32f4/startup_stm32f4xx.s
    stm32f4/system_stm32f4xx.c
    drivers/spi/spi.c
    drivers/enc28j60/internal.c
    drivers/enc28j60/enc28j60.c
    drivers/gpio/gpio.c
    drivers/dma/dma.c
    kernel/sched/context_switch.s
    kernel/sched/sched.c
    kernel/sched/future.c
    kernel/time.c
)

include_directories(include)

set_property(SOURCE stm32f4/startup_stm32f4xx.s PROPERTY LANGUAGE C)
set_property(SOURCE kernel/sched/context_switch.s PROPERTY LANGUAGE C)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -T${PROJECT_SOURCE_DIR}/stm32f4/stm32_flash.ld")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nosys.specs")

add_compile_definitions(HSE_VALUE=8000000 PLL_M=8)

# TODO: make this work
#set(LINKER_SCRIPT stm32f4/stm32_flash.ld)
#set_target_properties(Roberto PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

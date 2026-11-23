import os

Import("env")

kernel = os.path.join(
    env.subst("$PROJECT_DIR"), "Middlewares", "Third_Party", "FreeRTOS", "Source"
)

env.Append(CPPPATH=[
    os.path.join(kernel, "include"),
    os.path.join(kernel, "CMSIS_RTOS_V2"),
    os.path.join(kernel, "portable", "GCC", "ARM_CM4F"),
])

env.BuildSources("$BUILD_DIR/FreeRTOS", kernel, src_filter=[
    "+<croutine.c>",
    "+<event_groups.c>",
    "+<list.c>",
    "+<queue.c>",
    "+<stream_buffer.c>",
    "+<tasks.c>",
    "+<timers.c>",
    "+<CMSIS_RTOS_V2/cmsis_os2.c>",
    "+<portable/MemMang/heap_4.c>",
    "+<portable/GCC/ARM_CM4F/port.c>",
])
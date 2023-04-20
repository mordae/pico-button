# pico-switch: Switch Driver

To use these drivers with `pico-sdk`, modify your `CMakeLists.txt`:

```cmake
add_subdirectory(vendor/pico-switch)
target_link_libraries(your_target pico_switch ...)
```

If you do not have your board header file, create `include/boards/myboard.h` and use it like this:

```cmake
set(PICO_BOARD myboard)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/include/boards)
```

Your header file may include the original board file, but it must define following:

```c
/* Number of switchs on the board. */
#define NUM_SWITCHES 1

/* How many microseconds to wait in order to resolve possible bounce. */
#define SWITCH_DEBOUNCE_US 1000

/* How many events can the queue hold. */
#define SWITCH_QUEUE_SIZE 16

/* Enable pull up switch pins. */
#define SWITCH_PULL_UP 1
```

See `include/switch.h` for interface.

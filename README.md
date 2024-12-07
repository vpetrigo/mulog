[![codecov](https://codecov.io/gh/vpetrigo/mulog/graph/badge.svg?token=V29ApsX9FN)](https://codecov.io/gh/vpetrigo/mulog)
[![tests](https://github.com/vpetrigo/mulog/actions/workflows/tests.yml/badge.svg)](https://github.com/vpetrigo/mulog/actions/workflows/tests.yml)
[![GitHub Release](https://img.shields.io/github/v/release/vpetrigo/mulog)](https://github.com/vpetrigo/mulog/releases)

# Mulog - simple logger for embedded systems

This repository is an attempt to design a simple yet powerful logger that can be used for embedded systems.

## Dependencies

- [`printf`](https://github.com/eyalroz/printf.git): lightweight `printf`-family implementation suitable for embedded
  systems
- [`lwrb`](https://github.com/MaJerle/lwrb): ring buffer implementation suitable for embedded
  systems (_required only in deferred mode_)

## Usage

### CMake FetchContent

```cmake
include(FetchContent)
# Optional - configure mulog library options
set(MULOG_SINGLE_LOG_LINE_SIZE 256)
set(MULOG_ENABLE_LOCKING OFF)
# Fetch library content
FetchContent_Declare(mulog_library
        GIT_REPOSITORY https://github.com/vpetrigo/mulog.git
        GIT_TAG v12.34.45 # Replace this with a real available version
)
FetchContent_MakeAvailable(mulog_library)

target_link_libraries(<your_target> PRIVATE mulog::mulog)
```

## Optimization

By default, the project does not specify additional optimization flags for the library. To achieve smaller size of an
executable, consider adding the following:

- **GCC**: compile options `-ffunction-sections -fdata-sections` and linker option `-Wl,--gc-sections` to garbage
  collect unused functions/data. This also prevents necessity to define `putchar_()` function implementation which is
  a dependency from the `printf` library

## Options

The following options available for library configuration:

| Option                        | Default value | Description                                                                            |
|-------------------------------|---------------|----------------------------------------------------------------------------------------|
| MULOG_ENABLE_TESTING          | `OFF`         | Enable tests for mulog library                                                         |
| MULOG_ENABLE_COLOR_OUTPUT     | `ON`          | Enable color output                                                                    |
| MULOG_ENABLE_TIMESTAMP_OUTPUT | `ON`          | Enable timestamp output for log entries                                                |
| MULOG_ENABLE_LOCKING          | `ON`          | Enable locking mechanism for multithreading/multitasking environment                   |
| MULOG_SINGLE_LOG_LINE_SIZE    | `128`         | **Deferred mode only**: Maximum size of a single log line passed to an output callback |
| MULOG_OUTPUT_HANDLERS         | `2`           | Maximum number of output handlers that can be registered                               |
| MULOG_CUSTOM_CONFIG           | `""`          | Optional path to an external config file                                               |
| MULOG_ENABLE_DEFERRED_LOGGING | `OFF`         | Enable deferred logging support                                                        |
| MULOG_BUILD_EXAMPLES          | `OFF`         | Build examples                                                                         |

[`config.h`](src/internal/config.h) can be updated and used along with the `MULOG_CUSTOM_CONFIG` to provide a path
to modified configuration to be used for library build.

# Usage example

```c++
#include <stdbool.h>
#include <stdio.h>

#include <mulog.h>

static char buffer[256];

unsigned long mulog_config_mulog_timestamp_get(void)
{
    return 0;
}

static void output(const char *buf, const size_t buf_size)
{
    printf("%.*s", (int)buf_size, buf);
}

// required here to facilitate libprintf dependency requirements
void putchar_(char c)
{
    (void)c;
}

int main(void)
{
    mulog_set_log_buffer(buffer, sizeof(buffer));
    mulog_set_log_level(MULOG_LOG_LVL_INFO);
    mulog_add_output(output);

    MULOG_LOG_INFO("Hello");
    MULOG_LOG_DBG("World!");

    return 0;
}
```

# Contribution

--------------

Contributions are always welcome! If you have an idea, it's best to float it by me before working on it to ensure no
effort is wasted. If there's already an open issue for it, knock yourself out.

# License

---------

<sup>
This project is licensed under <a href="LICENSE.md">Apache License, Version 2.0</a>
</sup>

<br/>
<br/>

<sup>
Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in time by you, as
defined in the Apache-2.0 license, shall be licensed as above, without any additional terms or conditions.
</sup>

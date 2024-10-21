[![codecov](https://codecov.io/gh/vpetrigo/mulog/graph/badge.svg?token=V29ApsX9FN)](https://codecov.io/gh/vpetrigo/mulog)
[![tests](https://github.com/vpetrigo/mulog/actions/workflows/tests.yml/badge.svg)](https://github.com/vpetrigo/mulog/actions/workflows/tests.yml)

# Mulog - simple logger for embedded systems

This repository is an attempt to design a simple yet powerful logger that can be used for embedded systems.

## Dependencies

- [`printf`](https://github.com/eyalroz/printf.git): lightweight `printf`-family implementation suitable for embedded
  systems
- [`lwrb`](https://github.com/MaJerle/lwrb): ring buffer implementation suitable for embedded
  systems (_required only in deferred mode_)

## Optimization

By default, the project does not specify additional optimization flags for the library. To achieve smaller size of an
executable, consider adding the following:
- **GCC**: compile options `-ffunction-sections -fdata-sections` and linker option `-Wl,--gc-sections` to garbage
  collect unused functions/data. This also prevents necessity to define `putchar_()` function implementation which is
  a dependency from the `printf` library

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

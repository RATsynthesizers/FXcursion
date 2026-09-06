# FXcursion_Audio
Logic of FXcursion Audio Controller

# Clone project with all submodules
```git clone --recurse-submodules --remote-submodules https://github.com/RATsynthesizers/FXcursion_Audio.git```

# Layout
| Folder | Contents |
| --- | --- |
| `ApplicationSW` | Audio engine, audio IO, control link |
| `Config` | Per-project `*_cfg.h` for the shared services |
| `STM32CubeIDE` | The CubeIDE project: `.cproject`, `.ioc`, `Core`, `Drivers`, linker scripts |
| `SystemSW` | Shared submodule - drivers, services, `Lib/InterComProtocol` |
| `Tests` | Host test suite, builds with CMake and plain gcc |
| `UtilitySW` | Reserved, matches the interface project |

`ApplicationSW`, `Config`, `SystemSW` and `UtilitySW` sit beside the CubeIDE
project rather than inside it, and reach it through Eclipse linked resources
declared in `STM32CubeIDE/.project`. Include paths therefore use
`${workspace_loc:/${ProjName}/...}` and not `../`, because only the workspace
form resolves through a link.

# Host tests
```
cmake -S Tests -B Tests/build && cmake --build Tests/build && ./Tests/build/fxc_tests
```
These build for the PC and deliberately exclude anything that includes the HAL
or FreeRTOS, so a green run says nothing about whether the firmware compiles.
Build in STM32CubeIDE for that.

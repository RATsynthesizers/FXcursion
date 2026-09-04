# FXcursion

A guitar processor based on STM32H743VITX. Currently in prototype phase.

## Features

<img src="https://user-images.githubusercontent.com/1298948/232513645-18dd9126-a2e6-42c4-89a1-5d45171cd0a9.png" alt="FXcursion render" width="50%" align="right" />

- **A powerful mixer with 3 stereo inputs and outputs.** \
  Easily integrate your guitar with other instruments and equipment.

- **Built-in looper and recording capabilities.** \
	Capture your musical ideas on the spot and save them to an SD-card.

- **Dynamic effects chain.** \
	Arrange reverb, chorus, distortion, flanger, and more effects in any order
  you choose.

- **Intuitive interface that's out of your way.** \
	Easily adjust parameters and fine-tune your sound while staying focused on
  your music.

- **Separate visualization display** that shows your current waveform and
  preset names.


## Getting started

**Clone with submodules.** A plain `git clone` leaves the `SystemSW` folders
empty and nothing will link:

```
git clone --recurse-submodules https://github.com/RATsynthesizers/FXcursion.git
```

Already cloned without them? Fill them in:

```
git submodule update --init --recursive
```

### About the submodules

`SystemSW` is one repository — [RATsynthesizers/SystemSW][systemsw] — checked
out at **two** paths, one per controller:

```
TestBenchmarks/FXcursion_Audio_SHIELD/SystemSW
TestBenchmarks/FXcursion_Interface_SHIELD/SystemSW
```

Both boards share it because both compile the same inter-controller protocol
out of `SystemSW/Lib/InterComProtocol` — the frame layout, the loop session
state machine, the CRC. **The two pointers must always be on the same commit.**
A split pointer is how the sender and the receiver end up built against
different definitions of the same frame, which on this link means silently
misrouted audio rather than a build error. Check them with:

```
git submodule status
```

Both lines must show the same SHA. When you bump one, bump the other in the
same commit.

If a submodule fails to fetch a commit, it usually means someone committed a
pointer without pushing `SystemSW` itself. Push there first, then here.

[systemsw]: https://github.com/RATsynthesizers/SystemSW

### Building

We use [STM32CubeIDE][] for development. Each controller's IDE project lives in
its own `STM32CubeIDE/` folder, with `ApplicationSW`, `Config`, `SystemSW`,
`UtilitySW` and `Tests` beside it and reached through Eclipse linked resources.
Include paths therefore use `${workspace_loc:/${ProjName}/...}` rather than
`../`, because only the workspace form resolves through a link.

There are also host test suites that build for the PC and exclude anything
touching the HAL, so a green run there says nothing about whether the firmware
compiles — build in the IDE for that:

```
cmake -S TestBenchmarks/FXcursion_Audio_SHIELD/Tests -B /tmp/at && cmake --build /tmp/at
```

Building the firmware from the command line (or other IDEs) might be possible,
but we haven't tested this yet.

[STM32CubeIDE]: https://www.st.com/en/development-tools/stm32cubeide.html

Hardware designs are availible at https://github.com/RATsynthesizers/FXcursion-HW

♡ Stay tuned for updates! ♡


## Docs

Documentation currently lives in the
[wiki](https://github.com/Predtech4/ProtoStack_H743VI_V0.2/wiki).


## Sponsors

[PCBWay][pcbway] sponsored our project with free PCB prototyping services.
You can read more about our experience working with them [on our Hackaday][pcbway-post].

[pcbway]: https://pcbway.com/g/i4SuPL
[pcbway-post]: https://hackaday.io/project/192448-fxcursion/log/225631-we-have-been-sponsored-by-pcbway

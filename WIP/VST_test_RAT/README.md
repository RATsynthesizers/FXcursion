# FXcursion FX Bench

A VST3 for auditioning the pedal's effects in FL Studio, with your own material.

It compiles **the firmware's own effect code** — the same `.c` files that go on
the STM32, from `../FXcursion_Audio_SHIELD`, unmodified and not copied. Nothing
is reimplemented for the PC. What you hear here is what the hardware will do.

---

## Build and install

```bat
build.bat
```
then, **right click → Run as administrator**:
```bat
install.bat
```

`build.bat` needs no admin. `install.bat` does, because
`C:\Program Files\Common Files\VST3` is not user-writable.

In FL Studio: **Options → Manage plugins → Find more plugins**. It appears as
*FXcursion FX Bench* by *RAT Synthesizers*.

---

## Set FL Studio to 48 kHz

The effect code has `AUDIO_SAMPLE_RATE_HZ = 48000` compiled into it. Delay
lengths, filter coefficients and LFO rates are all derived from that constant,
so at 44.1 kHz everything is out by 8.8% — a 500 ms delay becomes 544 ms.

The plugin does **not** resample to hide this, because a resampler would colour
exactly the thing you are trying to judge. Instead the header shows the host
rate and turns amber when it is not 48 kHz.

---

## What the panel does

```
  [ effect selector .................................. ]
  [x] Follow host tempo    BPM 120.0    Bar 4 / 4

     knob      knob      knob      knob        <- parameters 1-4
     knob      knob      knob      knob        <- parameters 5-8
```

**Knob centre** shows the live value, `0.000`–`1.000`. That is the number the
engine actually uses: `FX_PARAM.fValue` is normalised, and each effect maps it
to seconds, hertz or decibels internally. Turning that into a physical readout
needs a per-effect range table, which does not exist yet — ask if you want one.

**Under each knob** is the parameter name for the selected effect, from
`g_aFxDesc[].pParam[].pName`. A trailing `*` means the effect reads the value as
a small integer rather than a continuum (`FX_PF_STEPPED`) — phaser stages, for
example.

**sync** switches the parameter from its knob to a note division. It is only
enabled on parameters the effect declares as `FX_PF_SYNCABLE`. When sync is on
the effect ignores `fValue` **completely**, so the knob centre switches to
showing the division instead — that is also the clearest way of saying "turning
me does nothing right now".

**Tempo** feeds `TEMPO`, which is what synced parameters resolve against.
*Follow host* takes BPM and time signature from FL Studio; untick it to dial
them manually. The bar-length convention is the firmware's: BPM defines the
quarter note whatever the signature is, so 6/8 at 120 BPM is a 1.5 s bar.

Effects with fewer than eight parameters leave the rest as inert placeholders
rather than rearranging the panel, so the layout does not move as you switch.

---

## Only 8 of the 22 entries do anything yet

Seven of the eleven concepts are still `PassThrough` stubs in `fx_stubs.c` —
declared, sized, registered and selectable, but with the DSP written out as a
TODO rather than as code. The selector labels them:

| Working | Stub (passes audio through) |
|---|---|
| Amp | Chorus |
| Delay | Compressor |
| Overdrive | Distortion |
| Tremolo | Flanger |
| | Phaser |
| | Reverb |
| | Vibrato |

Both the mono and stereo variant of each. As you implement one, delete its case
from `FxHost::isImplemented` in `Source/FxHost.cpp` and the `[ stub ]` tag
disappears.

---

## Mono effects are auditioned as two instances

A mono-only effect is one plane wide. Rather than folding your stereo input down
to mono, the bench runs **two independent instances** — plane 0 on the left,
plane 1 on the right, each with its own filters, LFO phase and delay line.

That is exactly what the pedal does in its four-mono topology, so a mono
algorithm behaves here the way it will on the hardware. Stereo effects get one
instance across both planes, as they do on the pedal.

---

## Verifying a build

`build.bat` finishes by running `FxBenchLoadTest`, a console host that opens the
built `.vst3` the way a DAW does, selects every effect and pushes noise through
it. It reports **changed samples** as well as level, because an effect that
silently passes audio through measures as a perfectly healthy `out/in` of 1.000.

Expected output: `changed 1024/1024` for the eight implemented entries and
`0/1024` for the fourteen stubs. The ratios are checkable by hand — Amp at 0.7
maps linearly onto 0..2, so it reads 1.401; Delay at 0.7 mix has a dry gain of
0.3 and reads 0.300 until the first repeat returns.

---

## Layout

```
VST_test_RAT/
├── CMakeLists.txt        plugin + load-test targets
├── build.bat             configure, build, verify
├── install.bat           copy to the shared VST3 folder (admin)
├── Source/
│   ├── FxHost.h/.cpp     the bridge: block chunking, plane routing, tempo
│   ├── PluginProcessor.*  parameters and the audio callback
│   ├── PluginEditor.*     the panel
│   └── LoadTest.cpp      console host that proves the bundle loads
└── _deps/JUCE            JUCE 8.0.4, shallow clone
```

JUCE is GPLv3 or commercial. Private use for testing carries no obligation;
those start on distribution.

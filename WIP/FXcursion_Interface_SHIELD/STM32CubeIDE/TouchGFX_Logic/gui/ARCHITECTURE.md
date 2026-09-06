# FXcursion interface controller — GUI architecture

State as of 2026-09-03. Covers the TouchGFX layer, how input reaches it, how it
talks to the audio controller, and what is still missing.

Read this before changing anything in `gui/`. The non-obvious parts are marked
**WHY** — those are the ones that cost real debugging time.

---

## 1. Big picture

Five FreeRTOS tasks. Only the GUI task touches TouchGFX; only the CtrlLink task
touches USART2; only the flush touches the FMC.

```
  UISurvey task (BelowNormal, 1 ms poll)
    encMenu/encParam  ─┐
    btnYes/No/Up/Down  ├─► PUBSUB_TOPIC_UI ──┐
    btnFunc            │                     │
    btnRec ───────────►│ sem_RecStart        │
                       │  (straight to       │
                       │   Recorder, bypasses│
                       │   the GUI)          │
                                             │
  CtrlLink task (5 ms poll, USART2 + DMA)    │
    RX ─► ACK / TELEMETRY / DIAG / PONG ─────┤
    TX ◄─ SendConfig / SetParam / SetTempo    │
                                             ▼
  GUI task ──► FrontendApplication::handleTickEvent()
                      │
                      └─► Model::tick()      ~60 Hz, once per frame
                             │  drains TELEMETRY (latest only)
                             │  drains UI (up to 8 events)
                             ▼
                          ModelListener  (Presenter implements it)
                             ▼
                          View  ──► Containers ──► widgets
                             │
                             └─► Model::pushConfig / pushParam ──► CtrlLink TX

  Recorder task        SD card, MDMA ch1
  Display flush        inside the GUI task, MDMA ch0 ──► FMC ──► ILI9341
```

There is **no touch input**. TouchGFX's event system is bypassed entirely;
everything arrives through `ModelListener`.

---

## 2. Layers

| Layer | Files | Rule |
|---|---|---|
| Flush / HAL | `target/TouchGFXHAL.cpp`, `SystemSW/HW_Drivers/DISP_ILI9341` | Framebuffer → panel. Knows nothing about the app. |
| Model | `gui/model/Model.{hpp,cpp}` | **Single source of truth.** Also owns the wire translation. |
| ModelListener | `gui/model/ModelListener.hpp` | The input contract. Every screen implements it. |
| Presenter | `gui/*_screen/*Presenter.hpp` | Thin passthrough, both directions. No logic. |
| View | `gui/*_screen/*View.{hpp,cpp}` | Screen behaviour. Derived from Designer's `*ViewBase`. |
| Container | `gui/containers/*.{hpp,cpp}` | Reusable widget groups. Derived from `*Base`. |
| Navigation | `gui/system_screen/SystemNav.hpp` | Pure logic, no framework. Host-testable. |

### Why the Model is a big bag of save/get

`FrontendHeap` stores **one view at a time in a union**. Every view is
constructed on screen entry and destroyed on exit.

```
FrontendHeapBase:  union { LogoStartupView, SystemView, FXChainView, ... }
                          ^ largest one wins, chosen at compile time
```

So **no view state survives a screen change.** Anything that must persist lives
in the Model, and `setupScreen()` rebuilds the containers from it. That is why
`SystemView::setupScreen` replays `addModule()` for every occupied slot.

Corollary: adding a view means adding it to the Designer type list, or the union
is too small and you get silent memory corruption. Designer does this for you.

---

## 3. Input path

```
Encoder::update()          poll A/B, book ±1 into an accumulator
   │                       ONE STEP PER EDGE OF A  = one per detent on the
   │                       fitted encoder (it moves A once per click)
   ▼
PublishEncoderSteps()      publish FIRST, consume only on success
   │                       ► full topic costs latency, never a lost click
   │                       ► one message per step, never an accumulated total
   ▼
PUBSUB_TOPIC_UI            mail queue, TOPIC_QUEUE_DEPTH = 8
   ▼
Model::tick()              while(), up to UI_EVENTS_PER_TICK = 8 per frame
   ▼
modelListener->encMenuUpdate(±1)  /  btnYesUpdate(state, bIsFuncPressed) / ...
```

**WHY the accumulator.** The old driver raised a "changed" flag on the
transition back to `ENC_STILL` too, so every detent published *two* messages:
the direction, then a zero. Consumers are written `if (1 == nValue) {…} else
{…}`, so a published zero moved the cursor **backwards**. It was invisible only
because the queue was 1 deep and the poll loop far outran the 60 Hz drain, so
the phantom was almost always the message that got dropped. Deepening the queue
or adding a poll delay — both now done — would have made it permanent.

**WHY one step per message.** Consumers compare against `1` and `-1` exactly. A
value of 2 fails `1 == nValue` and is taken for a reverse turn — the same bug in
a new place. A fast turn produces several consecutive ±1 messages instead.

**WHY 1 ms poll.** The loop had no delay at all, so the idle task never ran (a
permanent current draw on a battery unit) and the 50 ms button debounce was
sampled at a rate that varied with system load. 1 ms is far faster than a hand
can move a 20-detent knob (~60 Hz on channel A).

**Still missing:** encoders have no debounce. Buttons get 50 ms + long-press.
A 4-state transition table that rejects illegal state pairs is the proper fix.

**Gotcha:** `nID` is only meaningful for `ENC_PARAM`, and `UISurvey` currently
hardcodes it to 0 — only one parameter encoder is wired. `EffectSettingsView`
bounds-checks it anyway.

---

## 4. Navigation — `SystemNav.hpp`

The System screen is a cursor over a grid. The logic is **framework-free plain
integers** so it can be exercised on the host across every topology.

```
        col 3     col 2     col 1     col 0
      (nearest OUT)                (nearest IN)
  ┌───────┬─────────┬─────────┬─────────┬─────────┬───────┐
  │       │  slot3  │  slot2  │  slot1  │  slot0  │       │  row 0
  │  OUT  ├─────────┼─────────┼─────────┼─────────┤  IN   │  row 1
  │       │         │         │         │         │       │  row 2 …
  └───────┴─────────┴─────────┴─────────┴─────────┴───────┘
          │      STOMP 1  │  STOMP 2  │  STOMP 3         │
          └───────────────┴───────────┴──────────────────┘
                fs 0          fs 1         fs 2

  encoder  +1 ──► towards IN  (slot index DECREASES)
           -1 ──► towards OUT (slot index INCREASES)
  btnUp/Down ──► between rows
```

Coordinates: `NAV_POS { S8 nRow; U8 nSlot; }`

- `nRow` 0..`nRowQty-1` = chain rows top→bottom
- `NAV_ROW_INPUT` = -1, `NAV_ROW_OUTPUT` = -2, `NAV_ROW_STOMP` = -3
- `nSlot` = grid column 0..3, **0 nearest IN** (matches `ChainModuleNumber`
  *and* the protocol's grid slot index). On the stomp row it is a foot-switch
  index 0..2; on IN/OUT it is unused.

### The row table

How many rows exist, and which container each one is, depends on the two stereo
flags. **A stereo input pair is ONE row; a mono pair is TWO.**

```
  bIsStereo1  bIsStereo2   rows   row 0        row 1        row 2      row 3
      0           0          4    monoChain1   monoChain2   monoChain3 monoChain4
      1           0          3    stereoChain1 monoChain3   monoChain4    -
      0           1          3    monoChain1   monoChain2   stereoChain2  -
      1           1          2    stereoChain1 stereoChain2    -          -
```

Built once per screen entry by `SystemView::buildRowTable()`. Group 1 is the
first input pair, group 2 the second.

**WHY this exists.** The old `encMenuUpdate` was 1090 lines — the same walk
written once per chain, six times. Normalising the chain index collapsed its 881
non-blank lines to 72 unique ones, and two bugs lived in that duplication:

- `btnUpUpdate` picked the mono-vs-stereo form of the **bottom** row by testing
  `bIsStereo1`, the flag for the **top** pair. With input 1 mono and input 2
  stereo, leaving the stomp board upwards selected a container that is not on
  screen — the highlight vanished and the cursor stuck in a phantom row.
- `btnDownUpdate` mapped stereo chain 2's slots onto the foot switches
  backwards, where every other site maps slot 0 → switch 3.

Neither is expressible now. "The bottom row" is `nRowQty - 1`; there is exactly
one slot↔switch mapping (`Nav_FootForSlot` / `Nav_SlotForFoot`).

### Invariants (all host-checked)

- No move ever lands on a non-existent row.
- A vertical move changes the row by at most 1 and never changes the slot.
- Horizontal is reversible everywhere it moves.
- Leaving the stomp board upwards always lands on the bottom row.
- Walking down from anywhere reaches the stomp board without looping.

### Deliberate quirks, preserved from the original

- **The mixer is one tall cell** spanning a whole column across every row.
  Standing on it, **up does nothing** (nothing above it) but **down leaves for
  the stomp board**. Vertical movement is otherwise blocked on the mixer column.
- **The middle switch is lossy.** Columns 1 and 2 both map to switch 2; coming
  back up picks column 2. Leave column 1 downwards and return, and you land one
  column over.

### Cursor state

`eCurrentSelect` (a `ModuleSelectType`) is the persisted notion, mirrored into
the Model. `ePrevSelect` records the last thing the cursor was **on**, so
leaving the chain at IN/OUT and coming back lands on the row you left. Moving
*within* a row does not change `eCurrentSelect`, so it must not change
`ePrevSelect` either — `moveCursor()` enforces that.

`SystemView::moveCursor()` is the only place highlights change. It also keeps
`mixModule`'s highlight in sync, and only touches it when the answer actually
changed (`MixModule::select/deselect` invalidate unconditionally).

---

## 5. Talking to the audio controller

**Everything is fire-and-forget and safe with no audio board attached.** Every
sender in `ctrl_link_if.c` is a push into a byte ring plus a DMA kick — no
handshake, no ACK wait, no spin. With nothing on USART2 the bytes clock out and
vanish. Nothing in the GUI reads a reply, so nothing can hang or time out. The
only observable difference is `isAudioAlive()` staying FALSE.

### Send state, not edits

There is deliberately **no "add effect" message**. Any change to the graph
re-sends the whole 96-byte `PROTO_CFG` and the audio side rebuilds, so a dropped
frame cannot leave the two boards disagreeing. Parameters are the one exception
— addressed by (chain, effect type, index) and sent individually, so a knob
moving does not cost a full rebuild.

### What is wired

| Trigger | Call | Where |
|---|---|---|
| Gauge turned | `pushParam` | `EffectSettingsView::encParamUpdate` |
| Module added / deleted | `pushConfig` | `SystemView::btnYesUpdate` (3 sites) |
| Mixer placed / removed | `pushConfig` | `SystemView::btnYesUpdate` |
| Effect added / deleted / reordered | `pushConfig` | `FXChainView::btnYesUpdate` |
| Audio board appears or reboots | `pushConfig` + `pushTempo` | `Model::tick`, dead→alive edge |

**WHY the dead→alive edge.** Whoever powers up second used to lose: an audio
board booting later, plugged in later, or reset would sit with an empty graph
until the user happened to touch the grid. Edge-triggered, so a healthy link
carries no repeats.

### The chain-index mapping

The audio side numbers chains in **plane order**, and chain widths always sum to
`AUDIO_CH_QTY` (4). So which GUI channel is chain 0 depends on topology:

```
  TOPO_4_MONO      m1 | m2 | m3 | m4       chains 0,1,2,3
  TOPO_ST1_2MONO   (m1+m2) | m3 | m4       chains 0,1,2
  TOPO_2MONO_ST2   m1 | m2 | (m3+m4)       chains 0,1,2
  TOPO_2_STEREO    (m1+m2) | (m3+m4)       chains 0,1
```

`Model::protoChainForChannel()` returns the index, or **-1 when that channel is
not part of the active topology** — `pushConfig` skips those so stale contents
cannot leak into the configuration.

### Effect id mapping

GUI `TEXTS` ids `T_CHORUSEFFECT..T_VIBRATOEFFECT` (10) map onto the protocol
pool (11 effects × mono/stereo = 22 ids, mono at even ids). Step is 2; the
protocol also has `FX_AMP`, which the GUI does not offer.

```
  FX_TYPE mono = FX_CHORUS_M + 2 * (eTexts - T_CHORUSEFFECT)
  wire id       = FX_VARIANT_FOR_WIDTH(mono, chainWidth)   // +1 if stereo
```

A `FXC_STATIC_ASSERT` in `Model.cpp` pins both ends of that range, so
renumbering either enum is a **build error**, not the wrong effect quietly
appearing on the audio board.

Value scaling: gauge `0..255` → wire `0..65535`, both endpoints exact.

### Not wired, on purpose

- `CtrlLinkIf_Stream` — the recorder stream must be armed to the layout from the
  last ACK **before** enabling it, or every channel records into the wrong file
  with plausible audio in it. Needs hardware to watch.
- `CtrlLinkIf_Transport` — needs the Looper screen.
- `bSync` / `eDivision` in `PROTO_SET_PARAM` — always free-running. The
  tempo-sync behaviour exists in the VST prototype but the GUI has nowhere to
  express it yet (no sync toggle, and `CustomGauge` has no discrete mode).
  `g_aFxDesc`'s `FX_PF_SYNCABLE` flag says which parameters may offer it.

---

## 6. Display flush — two non-obvious fixes

Single framebuffer at **0xC0000000** (SDRAM), `MPU_ACCESS_NOT_CACHEABLE`, so no
cache maintenance is needed on it. MDMA channel 0 pushes it to the FMC at
0x60800000. Channel 1 belongs to the recorder.

```
Application::draw(Rect&)
   currentScreen->draw(rect)      ← enqueues DMA2D blits, does NOT wait
   HAL::flushFrameBuffer(rect)    ← tail call, two byte stores, no barrier

TouchGFXHAL::flushFrameBuffer(rect)
   alignRectForWordDma(rect)      ← grow x/width to even
   flushDMA()                     ← ***the barrier***
   lcdSetWindow(...)              ← 11 FMC writes + DSB
   wait TE, MDMA chunk, wait mdmaSemaphore   (×1 or ×2)
```

**`flushDMA()` is load-bearing.** `STM32DMA` is installed, so widgets are drawn
by DMA2D asynchronously. The framework's own drain sits only on the
`REFRESH_STRATEGY_OPTIM_SINGLE_BUFFER_TFT_CTRL` path; this project uses
`REFRESH_STRATEGY_DEFAULT`, which skips it. That is correct for an LTDC that
rescans continuously, but we **push** the frame — without the barrier the MDMA
reads lines DMA2D has not written yet and sends whatever SDRAM holds. Symptom:
the first chunk of a heavy frame arriving half-drawn.

**The mdma semaphore is drained in `initialize()`.** `osSemaphoreDef` expands to
`{0, NULL}`, so `osSemaphoreCreate` takes the `vSemaphoreCreateBinary` path,
which creates a binary semaphore **already given**. Without the drain, every
flush acknowledged the *previous* transfer and returned early, and the next
`lcdSetWindow` retargeted the window under a live MDMA. `vSyncAllowedSem` is
deliberately **not** drained — it is a gate, and must start available.

**`alignRectForWordDma`** grows x/width to even because channel 0 uses WORD data
size against a 16bpp framebuffer. An odd `rect.x` hands the MDMA a source two
bytes off a word boundary. Full-screen rects (x=0, w=320) are always safe, which
is why screen changes repainted correctly while a widget update did not.

Tearing is a separate, open matter — governed by FMC throughput vs. the blanking
budget, and by the chunk height in `flushFrameBuffer`.

---

## 7. Working with TouchGFX Designer 4.25

**Never edit these — regenerated on every Designer save:**

```
  generated/**                    all of it
  gui/**/*Base.hpp, *Base.cpp     no such files exist in gui/, but
  gui_generated/**                *Base classes live here
  target/generated/**             TouchGFXConfiguration, OSWrappers, GeneratedHAL
```

**Safe to edit:** everything in `gui/` (derived classes), `target/TouchGFXHAL.cpp`
(generated once, then yours).

Rules that have already cost time:

- **Renaming a widget in Designer silently breaks every reference by name.**
  It is a regeneration, not a merge. Keep the derived class's coupling to
  `*ViewBase` members narrow — the `customGauges[]` / `effectPictograms[]` /
  `scrollMenu[]` / `aRow[]` tables exist for exactly this reason: one place to
  fix per rename.
- **Prefer Designer's Custom Actions/Triggers over poking child widgets.**
  `EffectListContainer::scrollEffectsUpdateItem` is the pattern — a virtual hook
  in the Base that survives regeneration. `container.someBox.setColor()` from a
  view does not.
- **`CANVAS_BUFFER_SIZE` is Designer-controlled** (4800 bytes in
  `SystemViewBase` and `EffectSettingsViewBase`). `CustomGauge` arcs are canvas
  widgets, and when the buffer is too small TouchGFX **silently drops
  rendering** rather than erroring. If arcs half-draw after adding widgets,
  raise it in Designer first — it is not a bug in your code.
- **New source files** need adding to `Debug/*/subdir.mk` and `objects.list` for
  a command-line `make`; CubeIDE regenerates those from `.cproject` itself.
  Include paths and source entries are two independent lists. `SystemNav.hpp` is
  header-only specifically to avoid this.

---

## 8. Host test harnesses

Currently in the scratchpad — **move these into the repo**, they are the only
automated coverage the GUI layer has.

| Harness | Checks | What it pins |
|---|---|---|
| `test_nav.c` | 4769 | Compiles the **real** `SystemNav.hpp`. All 4 topologies × every mixer placement × every cursor position. |
| `test_cfgmap.c` | 690 | Compiles the **real** shared headers *and* `fx_defs.c`. Topology→chain mapping, width sum invariant, TEXTS↔FX_TYPE, value scaling, and every effect descriptor: names present, param counts within `FX_PARAM_QTY`, even=mono/odd=stereo layout, and parameter names short enough for `PARAMETERNAME_SIZE`. |
| `test_encoder.c` | 67 | Replicates the driver. One step per A edge, sign, saturation, boot priming. |

```
gcc -std=gnu11 -Wall -I<gui/include> -I<SystemSW/Include> -I<Config> -I<Shared> \
    -o test_cfgmap test_cfgmap.c <Shared>/fx_defs.c
```

`test_cfgmap` links `fx_defs.c` because it reads `g_aFxDesc[]`; the other two are
self-contained.

**Lesson worth keeping.** The first `test_encoder.c` passed while the firmware
was wrong, because it modelled a detent as a full quadrature cycle and the
fitted encoder moves A once per detent. A replicated model can be wrong in the
same direction as your reasoning. `test_nav.c` and `test_cfgmap.c` compile the
real headers for that reason — prefer that whenever the types allow it.

---

## 9. What needs to be done

### Blocking other work

1. **Input / Output screens.** Both are stubs, so `bIsStereo1/2` cannot be
   changed at runtime — which means three of the four topologies are
   unreachable, and the navigation paths that used to hold both bugs are still
   untested on hardware. Highest value next.
2. **Shared parameter descriptor table.** `EffectSettingsView` gives every
   effect the same eight placeholder names (`T_RESOURCEID1 + n`); the lone
   `default:` in `refreshParamPage()` marks the spot. `g_aFxDesc[]` in
   `Shared/fx_defs.c` already carries real names, counts and `FX_PF_SYNCABLE`
   flags, and is already compiled into this project. Wire the screen to it and
   the effect screen stays one generic page forever.

### Screens still to write

`RecorderView`, `LooperView`, `MixerView`, `StompView`, `InputView`,
`OutputView` — all `setupScreen`/`tearDownScreen` stubs with `btnNoUpdate`
returning to the System screen.

- Recorder is the first real consumer of `telemetryUpdate()` and of
  `CtrlLinkIf_Stream` (mind the arm-before-enable ordering).
- Looper is the first consumer of `CtrlLinkIf_Transport`.
- Mixer needs `aMixGain` / `aMixPan`, currently defaulted to unity/centre.

### Known defects and gaps

- **Mixer column is not defended on add.** `btnYesUpdate` refuses to place a
  *mixer* on a column that holds a module, but does not refuse to place a
  *module* on a column that holds the mixer. That produces a grid the audio side
  rejects with `PROTO_RES_BAD_GRID`. Noted in `Model::pushConfig`.
- **Status bar is static.** `saveProjectName` / `saveBPM` / `saveBatteryState`
  and all three `StatusBar::update*` have **no callers**, so it shows
  constructor defaults ("Untitled", 120, 100%) forever. Needs a battery ADC, a
  tempo control, and project naming.
- **No persistence.** `Model.cpp` still has
  `// TODO: if loading project, read values from config file or EEPROM`. The SD
  card works and the Model is already the single source of truth, so this is
  mostly serialisation — `PROTO_CFG` is documented as the core of the preset
  format.
- **`btnFootUpdate` is never published.** `UISurvey` has a
  `// Repeat for each foot switch` marker; only `btnFunc` and `btnRec` exist.
- **`btnYesUpdate` / `btnNoUpdate` still switch on chain type** and use the
  older `monoChain[]` / `stereoChain[]` arrays. They are correct — they already
  index rather than repeat — but folding them onto `aRow[]` would remove the
  last of the mono/stereo branching. Do it after the navigation is confirmed on
  hardware.
- **`led_ws2812.c` and `Services/pixel_drv` are excluded from the build** and
  `led_ws2812.c` does not compile.
- **Interface Release configuration has never been built.**
- **One open warning:** sign-compare in `TouchGFXHAL.cpp` `flushFrameBuffer`
  (`lines_done + MAX_CHUNK_HEIGHT <= r.height`, `uint32_t` vs `int16_t`).
- **`framesWithoutChunking = 178`** in `flushFrameBuffer` changes flush
  behaviour a few seconds after boot. It is a bring-up hack that expires by
  counting *rects*, not frames.

### Deferred by design

Tempo-sync parameters (needs a GUI affordance), encoder debounce (needs bench
measurement of the fitted part), `CtrlLinkIf_Stream` (needs the audio board).

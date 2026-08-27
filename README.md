# Firmware fuzzing in CI with Renode

Fuzz your firmware's serial parser natively, then **prove the crash reproduces
on the real firmware** running on an emulated STM32F4 board — and gate CI on it.
All software, no hardware, runs in Docker and GitHub Actions.

> **As shipped, CI is red on purpose.** The parser contains one real,
> exploitable bug. The native fuzzer finds it in seconds, and Renode reproduces
> the same crash on the emulated microcontroller. Flip one commented line in
> [`common/parser.c`](common/parser.c) and CI goes green. That is the whole
> "I found a crash in my own firmware" story, on rails.

---

## How it fits together

```
                     common/parser.c  ── the SAME code, two ways ──┐
                     (serial packet decoder,                       │
                      one planted overflow)                        │
                            │                                      │
         ┌──────────────────┴───────────────────┐                 │
         ▼                                        ▼                │
  fuzz/harness.c                          firmware/ (main.c +      │
  built native with                       startup.c) cross-built  │
  clang -fsanitize=                       for ARM Cortex-M4        │
  fuzzer,address                                 │                 │
         │                                        ▼                │
         ▼                               Renode emulates an        │
  libFuzzer drives the                   STM32F4 Discovery, feeds  │
  parser millions of times.              packets over USART2,      │
  ASan flags the stack                   watches the UART for the  │
  overflow instantly.                    crash marker.             │
         │                                        │                │
         └──────────► GATE 1: crash ◄─────────────┘                │
                      found = CI red        GATE 2: firmware       │
                                            crashes on target      │
                                            = CI red               │
                                                                   │
   Apply the one-line fix ──────────────────────────────────────► both green
```

The key idea: **the firmware and the fuzz target share `common/parser.c`
verbatim.** What the fuzzer breaks is the exact translation unit that runs on
the board — not a re-implementation of it.

## The bug

[`common/parser.c`](common/parser.c) decodes a length-prefixed serial frame:

```
+------+------+---------------------+
| SYNC | LEN  | PAYLOAD (LEN bytes) |
+------+------+---------------------+
  0xAA  0..255
```

`decode_packet()` copies `LEN` payload bytes into a 16-byte stack buffer with
**no bounds check** — a textbook CWE-121 stack buffer overflow. Any frame with
`LEN > 16` corrupts the stack.

- **Natively**, AddressSanitizer catches the overflowing `memcpy` and prints a
  precise report pointing at `parser.c`.
- **On the board**, the firmware is built with `-fstack-protector-all`, so the
  overflow clobbers the stack canary; the function epilogue detects it and calls
  `__stack_chk_fail()`, which emits `!!FAULT:STACK-SMASH!!` over the UART. Renode
  sees that marker and fails the test — a deterministic on-target crash.

## Project layout

| Path | What it is |
|------|-----------|
| [`common/parser.c`](common/parser.c) `/.h` | Shared serial packet decoder. **The bug lives here.** |
| [`firmware/`](firmware/) | Bare-metal STM32F4 firmware: vector table, USART2 command loop, fault/canary traps. Cross-built with `arm-none-eabi-gcc`. |
| [`fuzz/`](fuzz/) | libFuzzer + ASan harness, seed corpus, saved crash. |
| [`renode/`](renode/) | Board bring-up script + Robot test that drives USART2 and asserts on the output. |
| [`docker/Dockerfile`](docker/Dockerfile) | Reproducible toolchain + Renode image (same locally and in CI). |
| [`.github/workflows/ci.yml`](.github/workflows/ci.yml) | The two gates. |

## Run it

### With Docker (matches CI exactly)

```bash
docker build -t fw-fuzz -f docker/Dockerfile .

# Full gate (firmware + fuzz + on-target). Red as shipped.
docker run --rm -v "$PWD:/work" -w /work fw-fuzz make ci
```

Individual pieces:

```bash
docker run --rm -v "$PWD:/work" -w /work fw-fuzz make firmware    # cross-compile
docker run --rm -v "$PWD:/work" -w /work fw-fuzz make fuzz-run    # fuzz gate
docker run --rm -v "$PWD:/work" -w /work fw-fuzz make repro       # replay saved crash
docker run --rm -v "$PWD:/work" -w /work fw-fuzz make renode      # on-target gate
```

### Without Docker

You need `arm-none-eabi-gcc`, `clang` (with libFuzzer/ASan), and
[Renode](https://renode.io) with its test deps
(`pip install -r <renode>/tests/requirements.txt`). Then:

```bash
make firmware      # build the ARM firmware
make fuzz-run      # fuzz the parser (fails on crash)
make repro         # deterministically replay the saved crash
make renode        # run the Renode Robot suite against the firmware
```

> **Path note:** Renode's `renode-test` wrapper does not handle spaces in the
> working-directory path. If your checkout path contains a space, run the
> Renode step from a space-free path (e.g. a symlink), or pass the ELF
> explicitly: `renode-test renode/fuzz_target.robot --variable
> ELF:/abs/no-space/firmware/build/firmware.elf`. GitHub Actions checkout paths
> never contain spaces, so CI is unaffected.

## Watch it go green

Open [`common/parser.c`](common/parser.c) and un-comment the one-line guard in
`decode_packet()`:

```c
if (len > sizeof(scratch)) len = sizeof(scratch);
```

Re-run `make ci`:

- the fuzzer explores and exits 0 (no crash),
- Renode's oversized-packet test now gets a clean `OK:` back from the firmware.

Both gates pass. That is the fix landing and CI turning green.

## Plugging in your own firmware

1. Replace `common/parser.c` with your real serial-command handler or packet
   decoder (keep a thin `parser_feed()` byte entry point).
2. Point `fuzz/harness.c` at your entry function.
3. Adjust the Renode board in `renode/firmware.resc` /
   `renode/fuzz_target.robot` (Renode ships platforms for many Cortex-M parts),
   and update the UART the firmware talks on.
4. Feed the fuzzer's saved crash inputs into the Robot test to prove each
   finding reproduces on-target.

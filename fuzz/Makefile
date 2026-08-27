# Native libFuzzer + AddressSanitizer build of the parser.
#
# The firmware and the fuzzer share ../common/parser.c verbatim.

# Force clang: libFuzzer is clang-only. (make predefines CC=cc, so `?=` here
# would not stick -- use `:=` and allow an explicit override via `make CC=...`.)
CC      := clang
BUILD   := build
CORPUS  := corpus

CFLAGS  := -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
           -I../common
SRCS    := harness.c ../common/parser.c

.PHONY: all fuzz repro clean

all: $(BUILD)/fuzz_parser

$(BUILD)/fuzz_parser: $(SRCS) ../common/parser.h | $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) -o $@

$(BUILD):
	mkdir -p $(BUILD)

# A crash must fail the build deterministically on every platform, so pin the
# exit codes explicitly (libFuzzer -error_exitcode + ASan exitcode) rather than
# relying on defaults.
ASAN_OPTIONS := abort_on_error=0:exitcode=1
FUZZ_FLAGS   := -error_exitcode=1

# Time-boxed fuzzing run suitable for CI (overridable). Exits non-zero -- and
# writes a crash-<hash> reproducer -- the moment a crash is found.
FUZZ_SECONDS ?= 60
fuzz: all | $(CORPUS)
	ASAN_OPTIONS=$(ASAN_OPTIONS) $(BUILD)/fuzz_parser $(FUZZ_FLAGS) \
	    -max_total_time=$(FUZZ_SECONDS) -print_final_stats=1 $(CORPUS)

# Deterministically reproduce the planted crash from a saved input.
repro: all
	ASAN_OPTIONS=$(ASAN_OPTIONS) $(BUILD)/fuzz_parser $(FUZZ_FLAGS) crash/crash-known

$(CORPUS):
	mkdir -p $(CORPUS)

clean:
	rm -rf $(BUILD)

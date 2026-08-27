# Top-level orchestration for the firmware-fuzzing-in-CI demo.
#
#   make firmware   - cross-compile the STM32F4 firmware (ARM GCC)
#   make fuzz       - build the native libFuzzer+ASan harness
#   make fuzz-run   - time-boxed fuzzing gate (fails if a crash is found)
#   make repro      - deterministically replay the saved crash under ASan
#   make renode     - on-target gate: run the Renode Robot suite
#   make ci         - the full CI gate: firmware + fuzz-run + renode
#   make clean
#
# FUZZ_SECONDS overrides the fuzzing budget (default 60).
# RENODE_TEST  overrides the Renode test runner (default: renode-test on PATH).

FUZZ_SECONDS ?= 60
RENODE_TEST  ?= renode-test
ROBOT        := renode/fuzz_target.robot

.PHONY: all ci firmware fuzz fuzz-run repro renode clean

all: firmware fuzz

ci: firmware fuzz-run renode

firmware:
	$(MAKE) -C firmware

fuzz:
	$(MAKE) -C fuzz

fuzz-run:
	$(MAKE) -C fuzz fuzz FUZZ_SECONDS=$(FUZZ_SECONDS)

repro:
	$(MAKE) -C fuzz repro

# The firmware ELF must exist before Renode can load it.
renode: firmware
	$(RENODE_TEST) $(ROBOT)

clean:
	$(MAKE) -C firmware clean
	$(MAKE) -C fuzz clean
	rm -rf snapshots logs robot_output.xml log.html report.html

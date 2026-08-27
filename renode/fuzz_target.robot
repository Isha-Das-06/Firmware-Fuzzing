*** Settings ***
Documentation     On-target crash reproduction for the STM32F4 parser firmware.
...               Proves that a crash the native fuzzer finds also faults the
...               real firmware running on the emulated board.
Suite Setup       Setup
Suite Teardown    Teardown
Test Setup        Reset Emulation
Test Teardown     Test Teardown
Resource          ${RENODEKEYWORDS}

*** Variables ***
${ELF}            ${CURDIR}/../firmware/build/firmware.elf
${UART}           sysbus.usart2

*** Keywords ***
Create Machine
    Execute Command           mach create "stm32f4"
    Execute Command           machine LoadPlatformDescription @platforms/boards/stm32f4_discovery.repl
    Execute Command           sysbus LoadELF @${ELF}

Send Byte
    [Arguments]               ${value}
    # Send a single raw byte (0-255) to the firmware over USART2.
    Send Key To Uart          ${value}

Send Bytes
    [Arguments]               @{values}
    FOR    ${b}    IN    @{values}
        Send Byte             ${b}
    END

*** Test Cases ***
Benign Packet Is Acknowledged
    [Documentation]           A well-formed packet (LEN=2) decodes and is ACKed.
    Create Machine
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     READY
    # Frame: SYNC=0xAA LEN=0x02 payload=0x41 0x42
    Send Bytes                170    2    65    66
    Wait For Line On Uart     OK:

Oversized Packet Is Handled Without Crashing
    [Documentation]           THE GATE. Sends an oversized frame (LEN=64) --
    ...                       the exact class of input the native fuzzer saves.
    ...                       A correct firmware bounds-checks and still replies
    ...                       OK:. The shipped, vulnerable firmware instead
    ...                       overruns its 16-byte buffer, trips the stack-smash
    ...                       trap, and this test FAILS -- the failing UART log
    ...                       shows the !!FAULT:STACK-SMASH!! marker, i.e. the
    ...                       crash reproduced on the emulated board.
    ...                       Apply the one-line fix in common/parser.c to turn
    ...                       this green.
    Create Machine
    Create Terminal Tester    ${UART}
    Start Emulation
    Wait For Line On Uart     READY
    # Frame: SYNC=0xAA LEN=0x40(64) then 64 payload bytes.
    Send Bytes                170    64
    FOR    ${i}    IN RANGE    64
        Send Byte             65
    END
    Wait For Line On Uart     OK:    timeout=5

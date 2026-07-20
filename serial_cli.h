#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

// ============================================================
//  Interactive Serial Command-Line Interface
// ============================================================
//
//  Type commands into the Serial Monitor to inspect and
//  control the crowd counter at runtime.
//
//  Commands:
//    help                  Show available commands
//    count                 Show current crowd count
//    status                Show full status (count, RSSI,
//                          channel, uptime, memory)
//    peak                  Show peak count since boot
//    avg                   Show rolling average
//    channels              Show per-channel hit distribution
//    vendors               Show device vendor breakdown
//    rssi [value]          Get or set RSSI threshold
//    expiry [seconds]      Get or set MAC expiry timeout
//    dump                  Dump all tracked MACs with RSSI
//    reset                 Clear all tracked devices
//    csv                   Toggle CSV output mode
//    pause                 Pause scanning
//    resume                Resume scanning
//    led                   Toggle LED heartbeat
//
// ============================================================

// Initialise the CLI.
void cliInit();

// Call from loop() to process serial input.
// Returns true if a command was executed.
bool cliPoll();

#endif // SERIAL_CLI_H

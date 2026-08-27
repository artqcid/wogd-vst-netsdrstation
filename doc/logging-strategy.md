# Logging Strategy

## Overview

The project uses a two-level file logger that writes to `%TEMP%/netsdrstation.log`.

- **Location:** `C:\Users\marku\AppData\Local\Temp\netsdrstation.log`
- **Implementation:** `source/util/file_logger.h`
- **Thread-safe & real-time-safe:** Yes. Producers (including the audio thread)
  push a formatted line into a **lock-free SPSC ring buffer** (no `mutex`, no
  allocation, no file I/O, no clock call on the hot path). A dedicated
  background thread drains the ring, stamps the wall-clock time and writes the
  file.
- **Auto-flush:** Yes (INFO always flushed, DEBUG throttled to 500 ms)

## Log Levels

### INFO Level

**When:** Always enabled (Debug + Release)

**What:**
- Connection events (connect/disconnect/error)
- Server configuration (sample rate, handshake success/failure)
- Critical errors and exceptions
- First occurrence of issues (first underrun, first overflow, first queue full)
- Performance-neutral events only

**Goal:** Production-safe logging with minimal overhead

**Usage:**
```cpp
NETSDR_LOG_INFO("KiwiSDR connected");
NETSDR_LOG_INFO("Server sample rate: %.1f Hz", rate);
NETSDR_LOG_INFO("CRITICAL - no adpcmDecoder");
```

### DEBUG Level

**When:** Debug builds only (disabled in Release via `#ifdef NDEBUG`)

**What:**
- Frame-by-frame decoding statistics
- Pipeline telemetry (every N frames)
- Clock-drift ratio adjustments
- Buffer levels, queue depth
- Detailed diagnostic data

**Goal:** Verbose logging for debugging without impacting Release performance

**Usage:**
```cpp
NETSDR_LOG_DEBUG("decodeAndQueue: frame %d, numBytes=%zu", frame, size);
NETSDR_LOG_DEBUG("renderPipeline: blocksPopped=%d bufferMs=%.1f", blocks, ms);
NETSDR_LOG_DEBUG("Clock-drift: ratio=%.6f->%.6f", old, new);
```

## Usage Guidelines

### For Developers

1. **Critical events → INFO:**
   - Connection state changes
   - Configuration errors
   - First occurrence of any problem
   - Exceptions and error conditions

2. **Diagnostic data → DEBUG:**
   - Frame/block counters
   - Buffer statistics
   - Periodic telemetry
   - Detailed pipeline state

3. **Performance:**
   - INFO must be performance-neutral (no hot-path logging)
   - DEBUG can be verbose (disabled in Release)

4. **Throttling:**
   - Log first occurrence as INFO
   - Log subsequent occurrences as DEBUG
   - Example: First 5 underruns → INFO, rest → DEBUG

### Example Pattern

```cpp
static std::atomic<int> errorCount{0};
const int count = errorCount.fetch_add(1) + 1;

if (count == 1) {
    NETSDR_LOG_INFO("Queue overflow detected (first occurrence)");
} else if (count % 100 == 0) {
    NETSDR_LOG_DEBUG("Queue overflow (count=%d)", count);
}
```

## Debug Workflow

**All bugs and errors MUST be analyzed in Debug builds:**

1. Reproduce issue in Debug build
2. Check log file: `C:\Users\marku\AppData\Local\Temp\netsdrstation.log`
3. Analyze DEBUG-level logs for detailed pipeline state
4. Fix issue
5. Verify in Release build (INFO logs only)

**Never debug audio issues in Release builds** - use Debug for full visibility.

## Log Format

```
[HH:MM:SS.mmm] [LEVEL] Message
```

Example:
```
[06:07:34.436] [INFO ] Connecting to station: kphsdr.com:8072
[06:07:34.631] [INFO ] KiwiSDR connected
[06:07:34.958] [INFO ] Server sample rate: 12000.0 Hz
[06:07:35.334] [DEBUG] renderPipeline: blocksPopped=0 inputSamples=0 queueDepth=0 bufferMs=0.0
```

## Implementation Notes

- Log file is opened in **append mode** (logs accumulate across sessions)
- Each plugin instance logs to the same file (timestamps distinguish instances)
- The file handle is owned by the process-wide `FileLogger` singleton and closed
  on destruction (program exit); the background logger thread is joined there.
- Producers never block: `log()` drops the message when the ring buffer is full
  (never on the audio thread in practice — the ring holds 2048 slots).
- Flush is throttled: INFO lines flush immediately (critical, never lose),
  DEBUG lines flush at most every 500 ms (avoids disk stalls on the logger thread).
- Timestamp (`localtime_s`) is computed on the logger thread, not in `log()`.

## Macros

```cpp
NETSDR_LOG_INFO(...)   // INFO level (always logged)
NETSDR_LOG_DEBUG(...)  // DEBUG level (Debug builds only)
NETSDR_LOG(...)        // Legacy (maps to INFO)
```

## Build Configuration

The logger automatically detects build type via `NDEBUG`:

- **Debug build:** `NDEBUG` not defined → INFO + DEBUG logs
- **Release build:** `NDEBUG` defined → INFO logs only

No manual configuration needed.

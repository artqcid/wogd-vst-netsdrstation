#pragma once
#include <atomic>
#include <cstdint>

namespace netsdr {

struct PipelineTelemetry {
    std::atomic<std::uint64_t> underruns{0};
    std::atomic<std::uint64_t> overflows{0};
    std::atomic<std::uint64_t> sequenceGaps{0};
    std::atomic<std::uint64_t> droppedBlocks{0};
    std::atomic<std::int32_t> queueDepth{0};
    std::atomic<std::int32_t> jitterBufferMs{0};

    void reset() {
        underruns.store(0);
        overflows.store(0);
        sequenceGaps.store(0);
        droppedBlocks.store(0);
        queueDepth.store(0);
        jitterBufferMs.store(0);
    }
};

} // namespace netsdr

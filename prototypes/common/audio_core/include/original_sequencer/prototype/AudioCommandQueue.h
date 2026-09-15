#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace original_sequencer::prototype {

enum class AudioCommandType : std::uint8_t {
    trigger = 0,
};

struct AudioCommand {
    AudioCommandType type = AudioCommandType::trigger;
    std::uint64_t targetFrame = 0;
    std::uint32_t voice = 0;
    float value = 1.0F;
};

template <std::size_t Capacity>
class AudioCommandQueue {
    static_assert(Capacity > 0);
    static_assert(std::is_trivially_copyable_v<AudioCommand>);

public:
    [[nodiscard]] bool tryPush(const AudioCommand& command) noexcept {
        const auto write = writeIndex_.load(std::memory_order_relaxed);
        const auto read = readIndex_.load(std::memory_order_acquire);
        if (write - read >= Capacity) {
            overflowCount_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        storage_[write % Capacity] = command;
        writeIndex_.store(write + 1, std::memory_order_release);
        updateHighWaterMark(write + 1 - read);
        return true;
    }

    [[nodiscard]] bool tryPop(AudioCommand& command) noexcept {
        const auto read = readIndex_.load(std::memory_order_relaxed);
        const auto write = writeIndex_.load(std::memory_order_acquire);
        if (read == write) return false;

        command = storage_[read % Capacity];
        readIndex_.store(read + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::size_t depth() const noexcept {
        const auto write = writeIndex_.load(std::memory_order_acquire);
        const auto read = readIndex_.load(std::memory_order_acquire);
        return static_cast<std::size_t>(write - read);
    }

    [[nodiscard]] std::size_t highWaterMark() const noexcept {
        return highWaterMark_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t overflowCount() const noexcept {
        return overflowCount_.load(std::memory_order_relaxed);
    }

    void reset() noexcept {
        readIndex_.store(0, std::memory_order_relaxed);
        writeIndex_.store(0, std::memory_order_relaxed);
        highWaterMark_.store(0, std::memory_order_relaxed);
        overflowCount_.store(0, std::memory_order_relaxed);
    }

private:
    void updateHighWaterMark(std::size_t depth) noexcept {
        auto high = highWaterMark_.load(std::memory_order_relaxed);
        while (depth > high && !highWaterMark_.compare_exchange_weak(high, depth, std::memory_order_relaxed)) {}
    }

    std::array<AudioCommand, Capacity> storage_{};
    alignas(64) std::atomic<std::uint64_t> readIndex_{0};
    alignas(64) std::atomic<std::uint64_t> writeIndex_{0};
    std::atomic<std::size_t> highWaterMark_{0};
    std::atomic<std::uint64_t> overflowCount_{0};
};

}  // namespace original_sequencer::prototype

#include "original_sequencer/prototype/AudioCommandQueue.h"

#include <cassert>
#include <atomic>
#include <thread>

int main() {
    using original_sequencer::prototype::AudioCommand;
    using original_sequencer::prototype::AudioCommandQueue;
    using original_sequencer::prototype::AudioCommandType;

    AudioCommandQueue<4> queue;
    assert(queue.depth() == 0);
    assert(queue.highWaterMark() == 0);
    assert(queue.overflowCount() == 0);

    assert(queue.tryPush(AudioCommand{AudioCommandType::trigger, 100, 1, 0.25F}));
    assert(queue.tryPush(AudioCommand{AudioCommandType::trigger, 137, 2, 0.50F}));
    assert(queue.tryPush(AudioCommand{AudioCommandType::trigger, 196, 3, 0.75F}));
    assert(queue.tryPush(AudioCommand{AudioCommandType::trigger, 255, 4, 1.00F}));
    assert(queue.depth() == 4);
    assert(queue.highWaterMark() == 4);

    assert(!queue.tryPush(AudioCommand{AudioCommandType::trigger, 300, 5, 1.00F}));
    assert(queue.depth() == 4);
    assert(queue.overflowCount() == 1);

    AudioCommand command;
    assert(queue.tryPop(command));
    assert(command.targetFrame == 100);
    assert(command.voice == 1);
    assert(command.value == 0.25F);

    assert(queue.tryPop(command));
    assert(command.targetFrame == 137);
    assert(command.voice == 2);

    assert(queue.tryPush(AudioCommand{AudioCommandType::trigger, 400, 6, 0.5F}));
    assert(queue.depth() == 3);

    assert(queue.tryPop(command));
    assert(command.targetFrame == 196);
    assert(queue.tryPop(command));
    assert(command.targetFrame == 255);
    assert(queue.tryPop(command));
    assert(command.targetFrame == 400);
    assert(!queue.tryPop(command));

    assert(queue.highWaterMark() == 4);
    assert(queue.overflowCount() == 1);

    queue.reset();
    assert(queue.depth() == 0);
    assert(queue.highWaterMark() == 0);
    assert(queue.overflowCount() == 0);

    // Exercise repeated ring reuse with a producer, consumer and UI observer.
    constexpr std::uint64_t count = 100000;
    std::atomic<bool> done{false};
    std::thread producer([&] {
        for (std::uint64_t i = 0; i < count; ++i) {
            while (!queue.tryPush(AudioCommand{AudioCommandType::trigger, i, 0, 1.0F}))
                std::this_thread::yield();
        }
    });
    std::thread consumer([&] {
        AudioCommand received;
        for (std::uint64_t i = 0; i < count; ++i) {
            while (!queue.tryPop(received)) std::this_thread::yield();
            assert(received.targetFrame == i);
        }
        done.store(true, std::memory_order_release);
    });
    while (!done.load(std::memory_order_acquire)) {
        assert(queue.depth() <= 4);
        assert(queue.highWaterMark() <= 4);
    }
    producer.join();
    consumer.join();
    assert(queue.depth() == 0);
}

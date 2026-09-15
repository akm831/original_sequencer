#include "original_sequencer/prototype/AudioCommandQueue.h"

#include <cassert>

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
}

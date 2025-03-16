#include <sltbench/BenchCore.h>
#include "../../src/core/signal.h"


struct SomeSignalEmitter {
    Signal<> signal;
};

int x = 0;

void BenchSignal()
{
    // Simple benchmark that sets up signal and runs handler to compare build flag performance
    // beware: the run to run variation of this one is horrible
    SomeSignalEmitter emitter;
    emitter.signal += { nullptr, [](void*) {
        x += 1;
    }};
    for (int i = 0; i < 1000; ++i)
        emitter.signal.emit(&emitter);
}
SLTBENCH_FUNCTION(BenchSignal);

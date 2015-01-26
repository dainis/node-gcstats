# GCStats

Exposes stats about V8 GC after it has been executed.

# Usage

Create a new instance of the module and subscribe to `stats`-events from that:

    var gc = new (require('gc-stats'))();

    gc.on('stats', function (stats) {
        console.log('GC happened', stats);
    });

This will print blobs like this whenever a GC happened:

    GC happened {
        pause: 433034,
        pauseMS: 0,
        gctype: 1,
        before: {
            totalHeapSize: 18635008,
            totalHeapExecutableSize: 4194304,
            usedHeapSize: 12222496,
            heapSizeLimit: 1535115264
        }, after: {
            totalHeapSize: 18635008,
            totalHeapExecutableSize: 4194304,
            usedHeapSize: 8116600,
            heapSizeLimit: 1535115264
        }, diff: {
            totalHeapSize: 0,
            totalHeapExecutableSize: 0,
             usedHeapSize: -4105896,
            heapSizeLimit: 0
        }
    }

## Property insights
* totalHeapSize: Number of bytes V8 has allocated for the heap. This can grow if usedHeap needs more.
* usedHeapSize: Number of bytes in use by application data
* total HeapExecutableSize: Number of bytes for compiled bytecode and JITed code
* heapSizeLimit: The absolute limit the heap cannot exceed
* pause: Nanoseconds from start to end of GC using hrtime()
* pauseMS: pause expressed in milliseconds

gctype can have the following values:
* 1: Scavenge (minor GC)
* 2: Mark/Sweep/Compact (major GC)
* 3: Both 1 and 2

# Installation

    npm install gc-stats

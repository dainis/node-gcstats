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

# Installation

    npm install gc-stats

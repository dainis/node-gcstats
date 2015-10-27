var gcEmitter,
	util = require('util'),
	gcstats = require('./build/Release/gcstats'),
	EventEmitter = require('events').EventEmitter;

function GCStats() {
	if(!gcEmitter) {
		gcEmitter = new EventEmitter();
		gcstats.afterGC(function(stats) {
			gcEmitter.emit('data', stats);
			gcEmitter.emit('stats', stats);
		});
	}

	EventEmitter.call(this);
}

util.inherits(GCStats, EventEmitter);

module.exports = GCStats;

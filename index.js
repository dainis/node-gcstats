var gcEmitter,
	util = require('util'),
	gcstats = require('./build/Release/gcstats'),
	EventEmitter = require('events').EventEmitter;

function GCStats() {
	if(!gcEmitter) {
		gcEmitter = new EventEmitter();
		gcstats.afterGC(function(stats) {
			gcEmitter.emit('data', stats);
		});
	}

	EventEmitter.call(this);

	gcEmitter.on('data', this.emit.bind(this, 'stats'));
}

util.inherits(GCStats, EventEmitter);

module.exports = GCStats;
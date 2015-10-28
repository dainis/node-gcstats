var gcEmitter,
	gcstats = require('bindings')('gcstats.node'),
	EventEmitter = require('events').EventEmitter;

function gcStats() {
	if (this instanceof gcStats){
        	throw Error('gc-stats no longer exports a constructor. Call without the `new` keyword');
   	}
	if(!gcEmitter) {
		gcEmitter = new EventEmitter();
		gcstats.afterGC(function(stats) {
			gcEmitter.emit('data', stats);
			gcEmitter.emit('stats', stats);
		});
	}
	return gcEmitter;
}

module.exports = gcStats;

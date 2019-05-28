var GCStats = require('../'),
	should = require('should'),
	semver = require('semver'),
	topLevelKeys = ['startTime', 'endTime', 'pause', 'pauseMS', 'gctype', 'before', 'after', 'diff'],
	entryKeys = ['totalHeapSize', 'usedHeapSize', 'totalHeapExecutableSize', 'heapSizeLimit'];

if(semver.gte(semver.clean(process.version), '0.11.0')) {
	entryKeys.push('totalPhysicalSize'); //this v8 property became available with node 0.11+
}

if(semver.gte(semver.clean(process.version), '4.0.0')) {
	entryKeys.push('totalAvailableSize'); //this v8 property became available with node 4+
}

//this v8 properties became available with node 7+
if(semver.gte(semver.clean(process.version), '7.0.0')) {
	entryKeys.push('mallocedMemory'); 
	entryKeys.push('peakMallocedMemory'); 
}

//this v8 properties became available with node 10+
if(semver.gte(semver.clean(process.version), '10.0.0')) {
	entryKeys.push('numberOfNativeContexts'); 
	entryKeys.push('numberOfDetachedContexts');
}

describe('gc-stats', function() {
	var gcStats;

	beforeEach(function() {
		gcStats = GCStats();
	});

	it('should emit stats event with object containing gc stats', function(done) {
		gcStats.on('stats', function(stats) {
			topLevelKeys.forEach(function(key) {
				should.exist(stats[key]);
			});

			['before', 'after', 'diff'].forEach(function(topLevel) {
				entryKeys.forEach(function(entry) {
					should.exist(stats[topLevel][entry]);
				});
			});

			gcStats.removeAllListeners();
			done();
		});

		global.gc();
	});
});

var stats = (require('./index'))();

stats.on('stats', function(stats) {
	console.log(stats);
});

var t = [];

setInterval(function(){
	for (let i = 0; i < 100; i++) {
		t.push(new Date());
	}
}, 10);

setInterval(function() {
	while (t.length > 0) {
		t.pop();
	}
});

console.log("Generate some garbage");

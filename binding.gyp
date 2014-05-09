{
	"targets": [{
		"target_name" : "gcstats",
		"sources"     : [ "src/gcstats.cc" ],
		"include_dirs" : [
			"src",
			"<!(node -e \"require('nan')\")"
		]
	}]
}

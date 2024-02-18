{
    "targets": [{
        "target_name": "gcstats",
                "sources": ["src/gcstats.cc"],
                "include_dirs": [
                    "src",
                    "<!(node -e \"require('nan')\")"
                ]
                }, {
        "target_name": "action_after_build",
        "type": "none",
        "dependencies": ["gcstats"]
    }]
}

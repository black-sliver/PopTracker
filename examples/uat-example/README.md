# PopTracker UAT Example Pack

This example shows how to use [UAT](https://github.com/black-sliver/UAT) in
PopTracker to auto-track.

* UAT is enabled in `flags` in [manifest.json](manifest.json)
* Variables are received in [scripts/autotracking.lua](scripts/autotracking.lua)
* Sample server is in [util/uat-example-server.py](util/uat-example-server.py)

Load the pack and run the example server for a demonstation. The example server
requires python3 and websockets module (`pip install -r util/requirements.txt`).

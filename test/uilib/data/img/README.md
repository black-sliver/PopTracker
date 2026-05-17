# Test Image Files

## good

All files in here should be loadable with proper file format support.

NOTE: the good/gif87.gif is maybe not a valid gif87 file, but it has the gif87 header.

## bad

All files in here should fail to load without crashing.

* wrong-sig.png: wrong signature, should not be identified as PNG
* corrupt1.png: wrong CRC in IDAT
* corrupt2.png: wrong length in IDAT
* corrupt3.png: wrong CRC in IHDR
* corrupt4.png: wrong length in IHDR

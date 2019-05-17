# Sand-ular-automata

Ingredients: ESP32 breakout, SSD1105 OLED (128x64), MPU-6050 IUM, Double AA battery pack
Tricks: Simulates sand using cellular automata (6 simple rules)

## Converting images

Images for this OLED library map to the actual memory of the display. This makes for faster writing but it's more difficult to format the images.

* Make your 64x64 image
* Rotate it 90 degrees clockwise
* Mirror it horizontally
* Save as an XBM file (Gimp can do this, or https://convertio.co/image-converter/)

## Project Details:

https://hackaday.io/project/165620-digital-hourglass

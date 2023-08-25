# Project sketch: whispering forest fireflies

### Motivation: 
Foretsts are amazing to illuminate but unfortunately most lighting art i have seen in the past years are immediately recognizable as LED strips. Also i’m looking for a technical challenge.

### Short description: 
build autonomous little waterproof devices that can be hung up in trees. They blink and have a speaker and can triangulate their relative position to each other by using sound. This way we can simply hang them up randomly and know where they are, running light and sound animations through a larger space. 

### How does this work? 
One base station knows its exact location and sends out a timestamp every second or so. During calibration we walk around the forest and produce loud noises, each individual device picks up those noises as peaks and calculates the time difference between the timestamp and the peak, thus collecting a table of runtimes. These runtimes are then sent to the base station and this way we can calculate each position with an accuracy of at least 30cm (which would be 1ms granularity for sound in air) or even quicker. We use ESP-NOW as wifi broadcast for the timestamp and additional commands. 

### How does the hardware look like? 
It’s going to be a custom pcb with 3 leds on each side, a set of pin headers, a cheap SMD microphone, a switch and some charging infrastructure, plus a loud speaker  You can plug in a lithium polymer battery and an esp8266 dev board to the pin headers. The whole thing needs to be encased in something waterproof but the idea is to smartly use the ESPs sleep functions to keep the whole thing running for the entire week and only work at night. 

### What challenges are there? 
Many. Getting the math of the audio triangulation right, actually doing some audio triangulation, getting an energy saving way of doing the wifi command stuff, possibly doing our own PWM on the LEDs instead of using WS2812 (e.g. to use brighter LEDs), getting some nice anmations running, also getting the sound animation right, getting all of this done in a cheap fashion etc etc. 

### Costs? 
The cheaper the unit the more we can make. I hope to get a unit including battery for under 10 euros and i hope to get a dream funded that allows us to get 200 units. The cool thing is that every year we could increase the amount of units to make the thing even more sparkly…

### Why did you decide on the hardware you did? 
I had quite a few conversations at CCC Camp this year and right now this setup feels like the most sensible one, happy to explain individual questions any time. 

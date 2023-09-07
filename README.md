# Project sketch: whispering forest fireflies

###Motivation: 
Forests are amazing to illuminate but unfortunately most lighting art i have seen in the past years are immediately recognizable as LED strips. Also i’m looking for a technical challenge.

###Short description: 
build autonomous little waterproof devices that can be hung up in trees. They blink and have a speaker and can triangulate their relative position to each other by using sound. This way we can simply hang them up randomly and know where they are, running light and sound animations through a larger space. 

###How does this work? 
One base station knows its exact location and sends out a timestamp every second or so. During calibration we walk around the forest and produce loud noises, each individual device picks up those noises as peaks and calculates the time difference between the timestamp and the peak, thus collecting a table of runtimes. These runtimes are then sent to the base station and this way we can calculate each position with an accuracy of at least 30cm (which would be 1ms granularity for sound in air) or even quicker. We use ESP-NOW as wifi broadcast for the timestamp and additional commands. 

###How does the hardware look like? 
It’s going to be a custom pcb with a high power led on each side, an esp32, a cheap SMD microphone, a switch and some charging infrastructure, plus a loud speaker  You can plug in a lithium polymer battery and an esp8266 dev board to the pin headers. The whole thing needs to be encased in something waterproof but the idea is to smartly use the ESPs sleep functions to keep the whole thing running for the entire week and only work at night. 

###How do you want to charge 200 devices? 
Actually, we hope to get the battery and power consumption set up in a way that the devices can run for a whole week (power down during daylight etc), but the idea is also to have two notches in the width of a model railway track on the PCBs, so we can stick the boards on a long piece of track and charge all of them more or less at once. We might also do a sound installation during daytime and a light installation at night which creates even bigger need for charging infrastructure

###What challenges are there? 
Many. Getting the math of the audio triangulation right, actually doing some audio triangulation, getting an energy saving way of doing the wifi command stuff, possibly doing our own PWM on the LEDs instead of using WS2812 (e.g. to use brighter LEDs), getting some nice anmations running, also getting the sound animation right, getting all of this done in a cheap fashion etc etc. 

###Costs? 
The cheaper the unit the more we can make. I hope to get a unit including battery for under 10 euros and i hope to get a dream funded that allows us to get 200 units. The cool thing is that every year we could increase the amount of units to make the thing even more sparkly…

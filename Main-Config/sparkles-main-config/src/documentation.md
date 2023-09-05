#h1 Overall documentation
Here goes everything documentation for the coding process. nothing final, just thoughts

#h2 Messaging flow chart

- Client boots: 
    - boot client in vicinity of master
    - send address to master
    - wait for master to send timer
    - sync
    - hang up client
    - master continues to send timer
    - if timeout happens, client beeps
- client reboots
    - if calibration message received: delete eeprom and start from scratch
    - else load location data and ID from EEPROM
    - client sends message to master and asks for current time. 


- master upon boot: 
    - send timers including time. 
    - receive addresses
    - if no new address has been received for x minutes or button has been pressed (or upon serial command?): 
        - save all addresses onto EEPROM
        - go into calibration mode
        - announce calibration mode to everyone
    - after calibration mode is done (timeout or  button pressed): 
        - collect times from each address
        - do math
        - save all times onto EEPROM
        - send locations / IDs to clients
    - during animation mode
        - regularly send timers to update

- master upon reboot
    - if active time: 
        - check in with all clients to see if they are still there
    - if sleep mode: 
        - wait until dusk



#h2 differences master - client
- Master has no LED
- Master might have bigger antenna(?)
- master needs clock
- master needs much bigger battery






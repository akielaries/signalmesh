# signalmesh
*signalmesh* is a hybrid analog/digital synthesizer. For now it is a mess of reworked PCBs, perf/breadboards, and wires on my desk.

CAD work going on here right now: https://github.com/akielaries/signalmesh_CAD/

# Details
Don't forget to initialize submodules. ChibiOS is the RTOS used in this project on an STM32H7. I'm mostly building around an H755, 
occasionally the H753 but the H745/743 will probably work as well. AFAIK the only difference is the additional ARM M4 core and some
crypo acceleration stuff.


```
git submodule init
git submodule update
```



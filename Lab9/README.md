# Roller Coaster Simulation
There are P passengers and C cars, c is the capacity of each car and n is the rides count (each car will ride n times). Main thread creates all passenger and car threads. Cars are riding simultaneously, preserving their initial order. After car threads have finished their job, the passenger threads exit.

### More details on satisfied conditions
In each roller coaster iteration (all cars did one ride), only ONE car arrives at the platform and opens the door. Then, the passengers leave the car (if any). Afterwards, new passengers enter the car. When the car capacity is hit, one passenger (randomly picked) presses the start button, door closes and car leaves the platform. Then, next car arrives and so on. Each car's ride lasts for arbitrary number of miliseconds (I assumed it lasts from 0 to 9 ms).

## Getting started
First, clone the repo. Then type:
```
cd Lab9
make
```
In Makefile there are default arguments passed to main. You can change them by modifying Makefile.
Enjoy!

##Author
Monika Dziedzic, Student of 2nd Year of Computer Science at AGH UST.

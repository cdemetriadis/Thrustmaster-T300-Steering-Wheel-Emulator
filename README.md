## Thrustmaster T300 Wheel Arduino Nano Emulator

I'm trying to put together an Arduino Nano steering wheel emulator. I found plenty of scripts that do some of the things I need, but none that do everything *I need*. 

#### What I want to achieve

I want to build a custom steering wheel for my Thrustmaster T300 base that has a more ergonomic design and also features ALL the buttons available on the PS4 Pro Controller (incl. L3 & R3). 

##### What is the problem with the current projects?

The best project I've found so far only supports the standard wheel which is missing the L3 & R3 buttons. These buttons are located on the base of the T300. Another script includes these buttons but is way too complex and lacks any documentaion, so using it is impractical. Another problem is the fact that the Nano only has 16 GPIO (Digital & Analog) but with the addition of the L3 & R3, we need 17.

#### Solution?

I'm going to build a button grid matrix which only requires a fragmnet of the available GPIO's.

I also would like to include a screen for displaying feedback from the switches.

#### Target build

The idea is to get a custom built steering wheel from 3drap and add:

* Buttons
* Encoders
* 5way Joystick
* LCD Screen

###### Design

![GT3FF](images/GT3FF.png)

###### PS4 Button Mapping

![GT3FF](images/GT3FF-PS4.png)

#### Credits to those who have helped in one way or another

* Taras Ivaniukovich, https://rr-m.org/blog/
* Danny van den Brande, Arduinosensors.nl
* Bram Harmsen, https://www.thingiverse.com/thing:2813599

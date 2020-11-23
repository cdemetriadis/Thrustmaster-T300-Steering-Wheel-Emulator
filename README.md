# Thrustmaster T300 Steering Wheel Emulator

This project started when I decided to get a steering wheel that would suit my personal sim driving needs, something that represented the driving experience with realism and no frills. **This build is intended for use with a Thrustmaster T300 (direct plugin), Playstation 4 and Assetto Corsa Competizione,** but it should work on other Thrustmaster bases and simulators. 

## The Build

I set my goals high and made a list of things I wanted the steering wheel to feature.

### Features

1. The steerig wheel should feature all of the buttons available on the Playstation DualShock 4 controller
2. I wanted to include a display for settings related to the steering wheel (and possibly future proofing)
3. Rotary encoders for quick access to necessary driving functions (Brake Balance, ABS & Traction Control)
4. I wanted to also include a custom Combined Action Button (CAB) that would enable me to increase or decrease - with a single click - any of the above functions an X amount of times.

### Design

Since I do not have immediate access to a 3D printer or a CNC machine, I decided to go with one of [3DRap.it](https://www.3drap.it/product/f1-steering-wheel-kit-by-3drap-thrustmaster-logitech-osw/)'s designs. With the help of Antonio, we designed a much simpler version of the current wheel they have on stock, and one that matches my design below.

![F1 Steering Wheel](assets/f1-wheel-emulator.jpg)

### Stickers & Labels

The stickers were printed using a standard inkjet on an A4 sticker sheet, and then covered with transparent packing tape. You can find and download the [**sticker sheet here** (pdf)](assets/f1-wheel-labels.pdf) :link: .


## The Hardware
The most important thing to rememebr with the hardware is that the Thrustmaster base powers your steering wheel with ~3,5V. Everything you use should be able to run on that voltage.

### The Brains
From the very beginning, the idea was to build this around an **Arduino Nano** board. I've had previous experience with this platform and the fact that I found some starting points on the internet made this decision easy.

### The Display
Next up was the display. I just wanted the display as a means for me to view and edit settings on the wheel itslef, not to connect or view telemetry. So, a simple 16x2 backlit LCD would suffice. To keep the GPIO's limited, I also added an I<sup>2</sup>C backpack.

### The Rotary Encoders
The Rotary Encoders were a bit of trouble as the Arduono Nano does not support many GPIO's. At the end of the day, I went with three DuppaNet I<sup>2</sup>C Rotary Encoder Mini v2.1 boards. They work great, never miss a hit and require only 3 GPIO's total.

### The Rotary Switches
Finally, for the Rotary Switches I used two Mini 8-way Rotary Switches, each one driving a single analog GPIO, reducing the required GPIO's even more. The idea is pretty simple, and you can [find out more here](https://www.instructables.com/Arduino-Rotary-Switch-One-Analogue-Input/).

### The Buttons
I needed 22 buttons to perform all of the funcitons I planned. I sourced them from multiple online vendors with no particular one to mention. Most buttons are simple temporary (ON)-OFF switches with the exception of the (ON)-OFF-(ON) toggles. For the paddle shifters 3Drap.it supplies heavy duty microswitches that fit snuggly into the housing. 

---

:link: Here's the complete [**list of the hardware**](hardware-list.md) I used.

---

## The Software

This is where I received a lot of help from the ***community*** (pls see the Credits below). I did my research and found that many people were doing similar things, but none the way I wanted my steering wheel to work. So, I contacted a few of them, got some insights and started building my own version.

My starting point was [this post by Taras](https://rr-m.org/blog/thrustmaster-t300rs-steering-wheel-arduino-emulator/), where I got the basic hardware & software connection guides to the T300. This was also the first post I read that led me on this journey. Taras has been an immense help as he's been responding to all my technical questions.

Following that, I found the reverse engineered Thrustmaster F1 Wheel bits & bytes in a [project by Bram Harmsen](https://www.thingiverse.com/thing:2813599), which includes the L3 & R3 buttons, which in turn are generally not available on any other steering wheel. Having all of the information required to replicate all the actions, I started putting it all together.

I won't get into too much detail on that code here, but you are welcome to view it [in the repo](t300_steering_wheel_emulator/t300_steering_wheel_emulator.ino). I've added as many comments as possible. Here's what it does:


### Functions
The wheel behaves very much like any other wheel on the market. It emulates all of the Playstation DualShock 4 controller buttons. The main differences are **Button Matrix**, **Display Menu**, **Rotary Encoders** and the **Rotary Switches** (CAB Functions).

### The Button Matrix
In the name of saving PDIO's on the Nano, I created a 5x5 Button Matrix to support the 22 buttons I required for my design (yes, I have 3 to spare!).

---

:link: Here's the [**Button Matrix**](button-matrix.md).

---

### The Display Menu
There were a few reasons I wanted a display. **First,** I needed a way to change the wheel settings without the need to connect it to a computer. **Second,** I also thought it be nice to have a clock to display the real-world time & date. **Third,** the display could be used in future updates to display telemetry... (but nothing planned yet). **And finally,** it just looks cool.

The display is controlled by the three display buttons (MENU, NEXT & SELECT) and has the following options:

1. **Display Mode:** Set the mode of the display when Display Keypress is on. Every time you press a button it displays its' function:
	* **Mode: PS** - Pressing any button displays the Playstation button function (Cross, Triangle, L2, R2, ... etc)
	* **Mode: Wheel** - Pressing any button displays the simulator button mapping according to my personal preferences (Pit Limiter, HUD, Wipers, ... etc)
2. **Display Keypress:** Show the button keypress on the display
3. **Display Off** - Turn the display off (any display button turns it back on a again)
4. **Buzzer** - Turn the buzzer on or off
5. **Hour Chirp** - Yes, it could also beep every hour to keep track of the time you spend on the simuator
6. **Display Runtime** - Display the time since the steering wheel booted, so you know how long it's been running


### The Rotary Encoders
The Rotary Encoders allow for quick access the **Brake Balance**, **ABS** and **Traction Control** adjustments while driving. Rotary Encoders are cool because you can quickly tweak settings without needing to distract yourself with on screen menus etc.


### The CAB (Combined Action Buttons)
This is something I thought about while watching a video where **Nico Rosberg** was explaining how you need to shift the Brake Balance from the front to the rear and back multiple times on a single lap to optimise your driving. You can of course use the Rotary Encoders, but you can't be precise, especially with the force feedback kicking on your wheel.

The idea is to create a set buttons that can replicate mutliple clicks depending on your preferences. Using the Rotary Switches, you can quickly set the parameters:

* **CAB Mode:** Selects the function you want to replicate (BB, ABS, TC)
* **CAB Steps:** Select the number of times you want to press the button (1x-8x)
* **CAB -/+:** When selecting the CAB -/+ button, it will execute on your selected preferences

## Putting it all together
I wanted to keep things as compact as possible so a custom board was required. I also wanted things to be removeable and interchangeable in case there is failure in the future. My intention was to create a custom pritned PCB but it really did not make sense at this point in time (maybe in an update?). The final board is made up of a 5x7cm perfboard and I used solid core wire to make the connections.

Although my first attempt (v1) functioned perfectly it was not as compact as I'd like. In fact, it did not fit into the steering enclosure at all. I redesigned the tracks and rebuilt a second version (v2) using angled DuPont male connectors, saving a lot of space due to a limited height.

---

:link: Take a look at the [**Final Diagram Layout & Board**](board.md).

---

## Debugging and performance
The first software version had quite a few issues, with latency being the largest. Every now and then a button click would be missed which during a race this meant an misfire on an upshift or a downshift. This made up for some bad performance.

After running tests I discovered that a single loop ran at around 1700ms with an eventual hiccup every second at around 106000ms. It was abvious that something was producing some type of delay. I eventually optimised the code for the Rotary Encoders and the Clock (RTC). 

**The final version of the code running on the Arduino Nano with all the hardware connected has a loop latency of about ~190ms**. When the display is on, there's a hiccup every 750ms with a latency of ~2.000ms - this is due to the display refreshing every 750ms.

Since the display does not add much to the driving experience, I opt to turn it off while driving. That's why I added a shortcut to the ABS Rotary Switch, when it's long-pressed it turns the display off.

The current version works seamlessly and I have not noticed any misfires during racing.


## Credits
A small space to acknowledge the work done previously by others.

* **Taras Ivaniukovich** [https://rr-m.org/blog/](https://rr-m.org/blog/) - For the work he's shared on his blog and the help he provided all along the way
* **Bram Harmsen** [https://www.thingiverse.com/thing:2813599](https://www.thingiverse.com/thing:2813599) - For reverse engineeirng the F1 steering wheel
* **mrfid72** - For the awesome [Youtube Video](https://www.youtube.com/watch?v=84y5oYVXSMM) explaining the Analog Rotary Switches
* **Danny van den Brande** - For the code I found online that helped me setup the button matrix
* **Antonio De Stefano** [https://3drap.it](https://3drap.it) - For the awesome Steering Wheel housing

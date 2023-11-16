# disable_touchpad_on_typing_linux
Disables the touchpad and touchpad buttons during typing.

## Aren't there already working solutions for that?
Not in my case. The Gnome setting was not even available no me in the GUI. Changes in some config files didn't work. <br>
There is an application called "touchpad-indicator", which was triggered with a huge delay and also didn't disable the touchpad buttons. <br>
This was causing me lots of headaches. BUT NO MORE! <br><br>
This code is minimal, optimized and should work on any Xorg supported Linux distribution. 

## Building
`gcc main.c -o disable_touchpad_while_typing`<br><br>
With optimization:<br>
`gcc main -c -O3 -o disable_touchpad_while_typing`

## Usage
By using the standard parameters and autodetection of your keyboard and touchpad, use <br>
`./disable_touchpad_while_typing`<br>

There are 3 additional flags:
<ul>
  <li>-e: ID of the keystroke event file which can be found in /dev/input/. By not providing this argument, the program tries to find the ID automatically. However, if it doesn't succeed, then you have to provide it by yourself with this flag.<br>E.g. <b>-e 12</b></li>
  <li>-h: Helper message</li>
  <li>-t: Timeout: Timeout in milliseconds between the last keystroke and re-enabling the touchpad. Default value is 1500. E.g. <b>-t 1500</b></li>
</ul>

## Start application after booting
Place the `block-touchpad.desktop` file into `/home/<user>/.config/autostart` and adapt the directory in this file to the one pointing to the binary file. This directory only exists if you're using Gnome as your desktop environment. 

## Memory Leaks
Valgrind shows no memory leaks:
```
valgrind -s --leak-check=full --show-leak-kinds=all ./block_touchpad

==17595== 
==17595== HEAP SUMMARY:
==17595==     in use at exit: 0 bytes in 0 blocks
==17595==   total heap usage: 16 allocs, 16 frees, 15,315 bytes allocated
==17595== 
==17595== All heap blocks were freed -- no leaks are possible
==17595== 
==17595== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

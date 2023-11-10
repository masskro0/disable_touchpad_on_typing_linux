# disable_touchpad_on_typing_linux
Disables the touchpad and touchpad buttons during typing.

## Aren't there already working solutions for that?
Not in my case. The Gnome setting was not even available no me in the GUI. Changes in some config files didn't work. <br>
There is an application called "touchpad-indicator", which was triggered with a huge delay and also didn't disable the touchpad buttons. <br>
This was causing me lots of headaches. BUT NO MORE! <br><br>
This code is minimal and should work on any Xorg supported Linux distribution. 

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

# disable_touchpad_on_typing_linux
Disables the touchpad inclusive the touchpad buttons during typing.

## Aren't there already working solutions for that?
Not in my case. The Gnome setting was not even available no me in the GUI. Changes in some config files didn't work. <br>
There is an application called "touchpad-indicator", which was triggered with a huge delay and also didn't disable the touchpad buttons. <br>
This was causing me lots of headaches. BUT NO MORE! <br><br>
This code is minimal, not optimized, performs ressource-wise poorly, <b>BUT IT WORKS FLAWLESSLY!</b>.

## Building
`gcc main.c -o block_touchpad && ./block_touchpad`

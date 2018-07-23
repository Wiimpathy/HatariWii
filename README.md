
## Introduction 

HatariWii is a port of [Hatari](https://hatari.tuxfamily.org/ "Hatari") an Atari ST(E), TT and falcon emulator. The Atari ST was a 16/32 bits home computer. The Wii version has specific features :

- A virtual keyboard to send Atari ST keys to softwares.
- A mapper to bind ST keys to Wii controllers.
- Mouse emulation with Wiimote pointer, classic and GameCube controller stick/d-pad.
- 10 memory snapshots per game.
- A simple image viewer.

## Installing and running the Emulator 

Note, inside the archive read 'Manual_HatariWii.pdf' and 'manual.html' for in depth information!

Install the Emulator by unpacking the archive to a USB drive or SD card. The folder structure will be created and Hatari can then be launched directly with the Homebrew channel, or any other homebrew launcher. It also can be used as a plugin, and launched with command arguments through WiiFlow or Postloader (see Wiiflow plugin section). The emulator and folders can be copied on SD card, or USB drive. When the emulator starts, it looks for a configuration file (hatari.cfg) and a TOS file in the current directory. Here is the default directory structure :

/apps/hatari

    the default executable (boot.dol), configuration file (hatari.cfg) and TOS (tos.img) directory. 

/hatari/fd

    the floppy disk directory. Copy the games and softwares images here. The supported floppy images formats are : .st, .stx, .msa, or .dim. 

/hatari/saves : the memory snapshots are saved in this directory.

/hatari/hd : hard disk directory.

/hatari/doc : document directory. Manuals in image format can be stored here. The image viewer can read .png, .jpg or .bmp files.
TOS

The tos file is the operated system. By default, it should be named tos.img, and placed in /apps/hatari. The included Tos is a free version, and is called the Emutos. For better compatibility, use original TOS 1.02 or 1.04 for ST games, and TOS 1.62 or 2.06 for STE games. Tos 3.0x are for TT, and 4.0x are for falcon (untested in the Wii version).

## TOS

The tos file is the operated system. By default, it should be named tos.img, and placed in /apps/hatari. The included Tos is a free version, and is called the Emutos. For better compatibility, use original TOS 1.02 or 1.04 for ST games, and TOS 1.62 or 2.06 for STE games. Tos 3.0x are for TT, and 4.0x are for falcon (untested in the Wii version).

## LINKS

[Wiibrew](http://wiibrew.org/wiki/Hatari_Wii "Wiibrew")




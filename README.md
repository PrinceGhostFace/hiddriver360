# HidDriver 360

> [!NOTE]
> **About this branch:** it adds support for wired **Xbox One and Xbox Series controllers**. Those don't use HID at all, they speak Microsoft's own GIP protocol, which is why the original driver can't see them. This branch registers its own USB driver for them, does the GIP handshake (the pad has to be asked to identify itself before it sends any input) and turns their input into a regular controller. The Xbox button opens the guide, rumble works, and unplugging and replugging works without a restart.
>
> If the light on the pad doesn't come on after plugging it in, press the Xbox button once to wake it up.
>
> The original readme below still lists Xbox One and Series controllers as unsupported. On this branch they work.
>
> It also has the BadUpdate boot freeze fix from `fix/badupdate-boot-freeze`: hiddriver no longer resets the USB driver while a USB drive is connected, which froze BadUpdate consoles right after XeUnshackle. A controller plugged in before boot needs to be replugged once. And it can be built from the command line with `build.cmd` (just the XDK, no Visual Studio).
>
> Tested with a wired Xbox Series controller (045E:0B12) on a retail 17559 BadUpdate console only. Other GIP controllers (Xbox One, Elite, third party) should work but aren't tested.
>
> Heads up: this fork is kind of slopified with Claude (AI). Claude wrote the GIP support by watching what the console and the pad send each other, live over xbdm. It works on my console, but review it before you trust it.

Experimental on-console HID driver focussing on providing third party, non xinput controller support to the xbox 360 without dongles.

## What works
- Supports both 17559 retail and 17489 devkit dashboards
- Reading controller input via USB
- Up to 4 concurrent controllers, in combination with original xbox 360 controllers or without them
- Controllers are registered inside xam and therefore fully recognized in the entire xbox 360 user interface, ring of light will update accordingly, no original controllers are needed etc
- The controller works in all tested games

## Current limitations
- no rumble support

## Supported controllers
- DualShock 4 (PS4 controller)
- DualShock 4 Wireless Adapter (CUH-ZWA1E)
- DualSense (PS5 controller)
- DualSense Edge
- DualShock 3
- Nintendo Switch Pro controller
- Any USB HID compliant controller thanks to built in mapping assistant :) (this excludes modern xbox controllers like xbox one and xbox series because microsoft made them GIP only)

## How to use
1. load plugin (either at runtime or at boot via your launch.ini)
2. Connect controllers via USB
3. Controllers with a built in mapping will start working right away, for all other controllers that are USB HID compliant a mapping assistant will be started that guides you through the process of mapping your controller
4. enjoy :)

## How to build
1. Acquire the official Xbox 360 SDK using black magic
2. Install visual studio 2010 ultimate and visual studio 2019
3. Install the sdk using the "FULL" preset
4. Open the solution in visual studio 2019 and build
5. Hopefully enjoy :)

## Showcase
https://github.com/user-attachments/assets/f090e5f4-538d-457f-8189-2c5b98579984


## Attributions
- [EinTim23](https://github.com/EinTim23/) for Reverse engineering the xbox 360s HID and controller implementation and implementing this driver
- [localcc](https://github.com/localcc/) for providing help about low level USB related questions and beaming the idea of doing this into my head
- [Rapidjson](https://github.com/Tencent/rapidjson/) JSON library used to save/load user defined mappings
- [Lufa](https://github.com/abcminiuser/lufa) HID report descriptor parser implementation used in hiddriver360 is derived from their implementation
- [iMoD1998](https://github.com/iMoD1998) Creating the Detours library used in hiddriver360

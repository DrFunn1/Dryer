Dryer can send WebMIDI to your softsynth, DAW, or external MIDI interface.

You will need a Virtual MIDI cable to connect Dryer WebMIDI output to your software or external equipment.
If you have any problems contact me below and we will get it sorted out.

## Windows

1. Set up loopMIDI
Install and launch loopMIDI from Tobias Erichsen’s site. https://www.tobias-erichsen.de/software/loopmidi.html
​In the loopMIDI window, click the + button to create a new port; give it a clear name, e.g. WebMIDI to Windows.
Leave loopMIDI running in the tray; the virtual port only exists while loopMIDI is running.
This port is now a bidirectional virtual MIDI cable visible to all MIDI apps on Windows (DAWs, softsynths, etc.).

2. Configure the Dryer WebMIDI webpage
Open the Dryer webpage in a browser with WebMIDI support (typically Chrome).
Click on WebMIDI button at bottom
Choose the loopMIDI port you created (WebMIDI to Windows).
If the port is not listed, refresh the Dryer page
Start the Dryer. Drum and Vane object contacts are now sent to the WebMIDI to Windows virtual MIDI port.

3. Make the port visible in your softsynth, DAW, or external interface
Windows apps should see the loopMIDI device, but they often need it explicitly configured to drive a synth voice or sample.
You have to configure it to use all or a single MIDI channel of your choosing to be seen as an external MIDI keyboard device.

4. Have Funn!

## MAC

1. Create a virtual MIDI port (IAC)
Open Applications › Utilities › Audio MIDI Setup.
In the menu, choose Window › Show MIDI Studio to see your MIDI devices.
Double-click the IAC Driver icon.
Check Device is online to enable it.
In the Ports/Buses area, click + to add a new one and name it something like WebMIDI to MAC.
That bus is now a virtual MIDI cable available system-wide to route MIDI between apps on the Mac.

2. Configure the Dryer WebMIDI webpage
Open the Dryer webpage in a browser with WebMIDI support (typically Chrome).
Click on WebMIDI button at bottom
Choose the IAC port you created (WebMIDI to MAC).
If the port is not listed, refresh the Dryer page
Start the Dryer. Drum and Vane object contacts are now sent to the WebMIDI to MAC virtual MIDI port.

3. Make the port visible in your softsynth, DAW, or external interface
MAC apps should see the IAC device (WebMIDI to MAC), but they often need it explicitly configured to drive a synth voice or sample.
You have to configure it to use all or a single MIDI channel of your choosing to be seen as an external MIDI keyboard device.

4. Have Funn!

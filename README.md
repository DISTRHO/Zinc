# DISTRHO Zinc

An utility plugin for getting sound out of plugin hosts into JACK.

There are 2 variants - Soft Zinc and Hard Zinc.  
Both variants create a JACK client where audio from the host is played through.

These plugins do not have any GUI or configuration whatsoever.

They present stereo input and output, but this is only for convenience, getting audio into the host from JACK is outside the scope of this project.

## Soft Zinc

The Soft Zinc plugin will copy audio data from the plugin host until it can be synced and sent into JACK.  
It is meant to be used on plugin hosts that are not using JACK, as a way to get audio from them into the JACK graph.

There is always some latency with this method.

One use-case is using OBS audio capture through PulseAudio.  
Placing the plugin on the "Filters" for this audio capture stream will send that audio into a JACK client for easy local monitoring.

## Hard Zinc

The Hard Zinc plugin will attempt to directly sync the plugin host audio thread with the plugin-created JACK client.  
It requires that the plugin host is already using JACK and uses it to drive its audio engine.  
Any other usage is unsupported and is undefined behaviour (but typically results in xruns non-stop).

The plugin uses the JACK non-callback API and semaphores in order to get everything in sync.  
Under normal circunstances it shouldn't add any extra latency or DSP load.

One use-case is using OBS audio capture through JACK.  
Placing the plugin on the "Filters" for this audio capture stream allows to get the sound out of OBS without any extra latency.

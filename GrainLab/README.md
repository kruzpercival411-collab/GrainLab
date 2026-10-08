# GRAIN LAB — Granular Sampler VST3 (Windows)

A real-time granular sampler / sample mangler built in C++ with JUCE 8.
Load a sample, the engine cuts it into grains, and every grain is pitched, panned,
reversed, enveloped and scattered independently to build new sounds.

---

## 1. Build the Windows VST3

Pick **one** of the two routes.

### Route A: on your own PC (about 10 minutes the first time)

1. Install **Visual Studio 2022 Community** (free). In the installer, tick **"Desktop development with C++"**.
2. Install **CMake** (https://cmake.org/download, tick *Add CMake to PATH*) and **Git** (https://git-scm.com/download/win).
3. Unzip this project somewhere, e.g. `C:\Dev\GrainLab`.
4. Double-click **`build-windows.bat`**.
   JUCE downloads automatically, and the build takes a few minutes the first time.
5. Copy the folder `build\GrainLab_artefacts\Release\VST3\Grain Lab.vst3`
   into `C:\Program Files\Common Files\VST3\`.
6. Rescan plugins in your DAW. Look for **Grain Lab** under *Instruments* (vendor "GrainLab Audio").

You can also open the project in Visual Studio directly (**File → Open → Folder**, select the project folder) and build the `GrainLab_VST3` target.

### Route B: let GitHub build it (no software to install)

1. Create a free GitHub account and a new **private** repository.
2. Upload the contents of this folder, including the hidden `.github` folder.
3. Go to **Actions → Build Windows VST3 → Run workflow**.
4. When it finishes (about 10 minutes), download **GrainLab-Windows-VST3** from the run page. It contains `Grain Lab.vst3` plus a standalone `Grain Lab.exe`.

> A standalone app (`Grain Lab.exe`) is built too. It's handy for trying the plugin without a DAW.

---

## 2. Using it

It's an **instrument** plugin: put it on an instrument/MIDI track.

| Area | What it does |
|---|---|
| **LOAD SAMPLE** / drag-and-drop | WAV, AIFF, MP3, FLAC and OGG. Loading happens on a background thread. Click **x** on the sample card to unload. |
| **Grain field** (big screen) | The real waveform of the sample. Click or drag on it to set **Position**. The teal band is the region grains are drawn from, orange dots are live grains (lighter = reversed), and the white line is the playhead. |
| **GRAIN** | Size 5–1000 ms · Density 0.5–200 grains/s · Position · Spread (0% = tight, 100% = whole sample) |
| **MOTION** | Speed 0.25–4x (how fast the playhead scans) · Pitch ±24 st per grain · Direction FWD / REV / RAND / F+R (alternating) · REVERSE scans the playhead backwards |
| **RANDOM** | Per-grain randomness for position, pitch (up to ±12 st), scan speed and pan |
| **ENVELOPE** | Hann, Gaussian, Triangle, Hamming, Rectangle, Blackman, applied to every grain |
| **TIME** | Stretch 25–400% (400% = 4x longer at the same pitch) · Global Pitch ±24 st · Fine Tune ±100 cents |
| **MASTER** | Gain −∞ to +12 dB · Pan · Width 0–200% · soft safety clipper with a CLIP light |
| **LOOP MODE** | Off (stops at the end) · Forward · Ping-Pong · Random (jumps around) |
| **MIDI MODE** | **OFF**: the ▶ button plays freely, no MIDI needed · **ONE SHOT**: a note plays through the sample once · **GATE**: sound while the key is held · **LOOP**: a note latches on, and the same note again turns it off |
| **FREEZE** | Stops the playhead so grains keep regenerating from the same spot: an endless, evolving texture. In GATE/ONE SHOT it also sustains notes after release. |
| **RANDOMIZE** | Musically constrained randomization: biased toward smooth envelopes, sensible sizes and in-key pitch intervals |
| **PRESETS** | Atmosphere, Vocal Cloud, Glitch, Frozen Texture, Stutter, Chaos, Ambient (the `<` `>` arrows step through them) |

MIDI notes transpose relative to **C4 (note 60)**, and velocity sets grain level. Up to 8 notes can sound at once.

**Resize**: drag the bottom-right corner (50%–150%). The size is saved with the project.

**Automation & state**: every control is a real host parameter (23 in all). Settings, the sample path and the window size are saved in your DAW project. If you move the sample file, the plugin shows "Missing sample".

---

## 3. How the engine works (short version)

* **Grains**: 128-voice pool, allocated once. Each grain has its own start position, length, playback increment (pitch), direction, pan, gain and window. Samples are read with 4-point Hermite interpolation.
* **Pitch vs. time are independent**: pitch changes each grain's playback rate, while Speed/Stretch move the playhead that grains are taken from. So you can stretch a 2 s vocal into a 10 s pad without changing its pitch.
* **Voice management**: when density × size needs more than 128 grains, new grains are dropped instead of allocating more. Gain is compensated for overlap, so dense clouds don't explode.
* **Real-time safety**: no allocation, no locks and no file I/O on the audio thread. Samples decode on a background thread, swap in through an atomic pointer, and the old sample is only freed once the audio thread has moved past it. Position, gain, pan and width are smoothed, and notes fade in and out to avoid clicks.

### Project layout
```
CMakeLists.txt           build config (rename the plugin: PLUGIN_NAME at the top)
build-windows.bat        one-click Windows build
Source/GrainEngine.*     the granular DSP engine
Source/PluginProcessor.* parameters, MIDI, sample loading, presets, state
Source/Params.h          all parameter definitions/ranges
Source/Presets.h         factory presets (easy to edit or add to)
Source/PluginEditor.*    the UI layout
Source/UI/*              knobs, buttons, waveform/meters/radar displays, theme
Tests/TestHarness.cpp    offline test suite (optional, -DGRAINLAB_BUILD_TESTS=ON)
```

### Renaming the plugin
Change `set(PLUGIN_NAME "Grain Lab" ...)` at the top of `CMakeLists.txt`. The DAW name, title bar and logo text all follow it. If you rename it after already using it in projects, also change `PLUGIN_CODE` so DAWs treat it as a new plugin.

---

## 4. What has and hasn't been tested

Verified by compiling and running the offline test suite (33 checks, all passing) on Linux with GCC 13 and Clang:

* WAV loading, waveform peaks and metadata
* Every factory preset produces clean audio that never goes over 0 dBFS
* Voice cap holds at 128 grains
* Pitch is correct (+12 → 440 Hz, −12 → 110 Hz from a 220 Hz sine)
* 400% stretch scans 4x slower at the same pitch
* Reverse grains, MIDI GATE/ONE SHOT, Freeze, and save/restore including reloading the sample
* CPU: about 0.8% of one core in the CHAOS preset

**Not yet tested:** the Windows build itself and actual DAWs (Ableton, FL Studio, Reaper, Cubase, Studio One). The code uses only standard JUCE/VST3 features, but compatibility is not claimed until it has been tried in each one.

---

## 5. Version 2 ideas (not built yet)
Grain sequencing and rhythmic sync to host tempo · LFOs and a modulation matrix · macro controls · multi-sample layers · resampling/export · user preset saving · probability controls.

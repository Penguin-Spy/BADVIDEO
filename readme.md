### BADVIDEO for the TI-84+ CE

Really bad 2-color video player for a calculator.

The actual video player program can be found in `bin/BADVIDEO.8xp`. Be warned, it is in the testing phase, if it runs out of files or encounters any problems with decompressing/decoding it will freeze/crash your calculator, requiring a reset.
When in the main menu, use ↑↓ & Enter to select and play a video.
While playing a video, these are the controls:
| Button               | Action
|---|-
| 2nd/Enter            | Pause/Play
| Mode (While paused)  | Display debug frame info
| Clear (While paused) | Stop playing & return to main menu
| → (While paused)     | Skip 1 frame ahead
| → (While playing)    | Fast forward

(No rewind because of how the video is stored, it's basically impossible)

The pre-converted [Bad Apple](https://www.nicovideo.jp/watch/sm8628149) video can be found in `bin/badapple/`. All 8xv files in this directory (`BADAPPLE.8xv` & `BADAPP00 to 45`) are required for the video to play. Currently, the full converted video wouldn't fit on a calculator, so the final 15-ish seconds have been truncated (yes, it's really that close).
Additionally, a pre-converted [Lagtrain](https://www.youtube.com/watch?v=UnIhRpIT7nc) can also be found, in `bin/lagtrain/`. Again, all 8xv files are required, and the full video is not stored (only like half this time because the line art is bad for LLV encoding). There are some issues with playing this video, mainly the speed is inconsistent (even with the frame-skipping & waiting code?????), and occasionally, randomly, it will refuse to update the screen for large portions of the video.

A python script to convert a video file (uses OpenCV, I've tested with .mp4) to a collection of AppVars can be found in `tools/convertFrame.py`. Usage is as follows:
```
convertFrame.py <input video file> <output directory> <AppVar name (1-8 chars)> [# of frames to convert]
```
Also, the makefile has a QoL target that can build a video from an .mp4 file in the `src/video/` directory:
```
make <video FILENAME no extension> [# of frames to encode, or leave blank for all]
```

The video is stored in my own file format (Line-Length Video), the specification can be found in `format.md`.
Any changes to the format are assumed to be non-backwards or forwards compatible.

---

USE AT YOUR OWN RISK! I AM NOT RESPONSIBLE FOR ANY CORRUPTIONS/ERRORS THAT ARISE FROM USING THIS PROGRAM! (although if it does cause problems, submit an [issue report](https://github.com/Penguin-Spy/BADVIDEO/issues/new)!)

Uses the [CE programming toolchain](https://github.com/CE-Programming/toolchain/). The [CE C Standard Libraries](https://github.com/CE-Programming/libraries/releases/latest) (specifically `LibLoad`, `FILEIOC`, `GRAPHX`, and `KEYPADC`) are required to run the program.

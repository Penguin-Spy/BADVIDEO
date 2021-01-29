### BADVIDEO for the TI-84+ CE

Really bad video player for a calculator.

The actual video player program can be found in `bin/BADVIDEO.8xp`. Be warned, it is in the testing phase, there are no play/pause/skip controls, once you play the video it's not gonna stop until it displays all the frames and exits cleanly, or runs out of files and crashes/freezes.

USE AT YOUR OWN RISK! I AM NOT RESPONSIBLE FOR ANY CORRUPTIONS/ERRORS THAT ARISE FROM USING THIS PROGRAM! (although if it does cause problems, submit an [issue report](https://github.com/Penguin-Spy/BADVIDEO/issues/new)!)

The pre-converted [Bad Apple](https://www.nicovideo.jp/watch/sm8628149) video can be found in `bin/video/`. All files in this directory (`BADAPPLE.8xv` & `BADAPP00 to 99`) are required for the video to play in full.

A tool to convert a video file (uses OpenCV, I've tested with .mp4) to a collection of AppVars can be found in the root directory as `convertFrame.py`. Usage is as follows:
```
convertFrame.py <input video file> <output directory> <AppVar name (1-8 chars)>
```

The video is stored in my own file format (Line-Length Video), the specification can be found in `format.md`.

Please not that this specification is currently not complete, and is subject to change at any time without a version increase.

---

Uses the [CE programming toolchain](https://github.com/CE-Programming/toolchain/). The [CE C Standard Libraries](https://github.com/CE-Programming/libraries/releases/latest) (specifically `LibLoad`, `FILEIOC`, and `GRAPHX`) are required to run the program.


# BPG Image Format Plugins

What is BPG format?
> Its purpose is to replace the JPEG image format when quality or file size is an issue.<br>
> http://bellard.org/bpg/

Supported image viewers:
- XnView / XnViewMP
- Susie
- Imagine

Features:
- BPG read
- BPG write (XnView, XnViewMP)
- Multi-thread support

Not supported:
- x64 (under development)
- YCgCo / CMYK colorspace
- Grayscale with limited component range
- Premultiplied alpha
- Writing 8-bit colour / 32-bit source images
- Animation

## Install Instructions

### XnView (32-bit)
http://www.xnview.com/

1. Download `BPG-xnview-XXXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
1. Extract files into `[program dir]\Plugins`
1. Rename or delete `[program dir]\Plugins\bpgdec.exe`
1. Download `BPG-dll-XXXX.7z`
1. Extract files into `[program dir]` (along with xnview.exe)
1. Edit `[program dir]\Plugins\Xbpg.ini` to change encoding options


### XnViewMP (32-bit)
http://www.xnview.com/

1. Download `BPG-xnview-XXXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
1. Extract files into `[program dir]\plugins`
1. Rename or delete `[program dir]\plugins\bpgdec.exe`
1. Check if `avutil-55.dll` & `swscale-4.dll` exist in `[program dir]`
   1. If not, download `BPG-dll-XXXX.7z` & extract files into `[program dir]` (along with xnviewmp.exe)
1. Edit `[program dir]\plugins\Xbpg.ini` to change encoding options


### Susie Plugin (for Hamana, etc.)
http://www.digitalpad.co.jp/~takechin/

1. Download `BPG-susie-XXXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
1. Extract files into the plugin directory of your image viewers
1. Download `BPG-dll-XXXX.7z`
1. Extract files into the directory where the *.exe file exists


### Imagine (32-bit)
http://www.nyam.pe.kr/blog/entry/Imagine

1. Download `BPG-imagine-XXXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
1. Extract files into `[program dir]\Plugin`
1. Download `BPG-dll-XXXX.7z`
1. Extract files into `[program dir]` (along with Imagine.exe)


## Encoding options
Most of image viewers have no GUI for user plugins, so you need a file to set encoding options.
You can specify different options for 8-bit & 24-bit images.
The line started with `24:` indicates the options for 24-bit images, and `8:` for 8-bit images.

Supported options (see: [bpgenc](http://bellard.org/bpg/)):
- `-q (0~51)` set quantizer parameter (smaller gives better quality, default=`29`)
- `-f (420|422|444)` set the preferred chroma format (default=`420`)
- `-loseless` enable lossless mode (default=off)
- `-m (1~9)` select the compression level (1=fast, 9=slow, default=`8`)


Example:
```
8:-q 30
24:-q 25
```

## References

- MinGW
  - http://www.mingw.org/

- libbpg
  - http://bellard.org/bpg/
  - v0.9.7

- libx265
  - http://x265.org/
  - V2.5

- ffmpeg
  - https://ffmpeg.org/
  - v3.4

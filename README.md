# BPG Image Format Plugins

What is BPG format?
> Its purpose is to replace the JPEG image format when quality or file size is an issue.<br>
> ~http://bellard.org/bpg/

# Supported Image Viewers

## XnView (32-bit only)
http://www.xnview.com/

Features:
- BPG read /write support
- Do not support animation

Install instructions:

0. Download `BPG-XnView-XXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
0. Extract files into `[XnView dir]\Plugins`
0. Rename `[XnView dir]\Plugins\bpgdec.exe` to `bpgdec.exe.bak` if the file exists.

You may want to change BPG encoding options.
Because XnView provides no GUI for user plugin configurations, we store the encoding options in the file `[XnView dir]\Plugins\Xbpg.ini`.
You can edit it using text editors, such as NotePad.
The line started with `24:` indicates the options for 24-bit images, and `8:` for 8-bit images.

Supported options (see: [bpgenc](http://bellard.org/bpg/)):
- `-q (0~51)` set quantizer parameter (smaller gives better quality, default=`20`)
- `-f (420|422|444)` set the preferred chroma format (default=`422`)
- `-b (8|10)` set the bit depth (default=`8`)
- `-loseless` enable lossless mode
- `-e (jctvc|x265)` select the HEVC encoder (default=`jctvc`)
- `-m (1~9)` select the compression level (1=fast, 9=slow, default=`8`)


## Susie Plugin (for Hamana, etc.)
http://www.digitalpad.co.jp/~takechin/

Features:
- BPG read support

Install instructions:

0. Download `BPG-Susie-XXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
0. Extract files into corresponding plugin directory of your image viewers

## Imagine (32-bit only)
http://www.nyam.pe.kr/blog/entry/Imagine

Features:
- BPG read support
- Do not support animation

Install instructions:

0. Download `BPG-Imagine-XXX.7z` from [Releases](https://github.com/leavinel/bpg_plugins/releases)
0. Extract files into `[Imagine dir]\Plugin`


# Reference Libaries

## MinGW
http://www.mingw.org/

## libbpg
http://bellard.org/bpg/

Ver: 0.9.5

## libx265
http://x265.org/

Ver: 0.7

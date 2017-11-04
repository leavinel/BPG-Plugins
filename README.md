# BPG Image Format Plugins

### What is BPG format?
> Its purpose is to replace the JPEG image format when quality or file size is an issue.<br>
> ~ http://bellard.org/bpg/

### What does this project do?
- Implement BPG plugins for image viewers
  - XnView / XnViewMP
  - Susie
  - Imagine
- Modify libbpg to improve API usability
- Provide C++ wrapper classes to libbpg

### Features
- BPG read
- BPG write
- Fast multi-thread YUV <-> RGB conversion

-|XnView|Susie|Imagine
-|------|-----|-------
Read  | O | O | O
Write | O | - | -

### Not supported
- YCgCo / CMYK colorspace
- Grayscale with limited component range
- Premultiplied alpha
- Writing 8-bit colour / 32-bit source images
- Animation

### How to install
- See [wiki](https://github.com/leavinel/BPG-Plugins/wiki)

### References

- libbpg
  - http://bellard.org/bpg/
  - v0.9.7

- libx265
  - http://x265.org/
  - V2.5

- ffmpeg
  - https://ffmpeg.org/
  - v3.4

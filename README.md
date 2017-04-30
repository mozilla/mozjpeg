Mozilla JPEG Encoder Project
============================

MozJPEG reduces file sizes of JPEG images while retaining quality and compatibility with the vast majority of the world's deployed decoders.

MozJPEG is based on [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo). It's compatible with libjpeg API and ABI, and can be used as a drop-in replacement for libjpeg. MozJPEG makes tradeoffs that are intended to benefit Web use cases and focuses solely on improving encoding, so it's best used as part of a Web encoding workflow.

MozJPEG is meant to be used as a library in graphics programs and image processing tools. We include a demo `cjpeg` tool, but it's not intended for serious use. We encourage authors of graphics programs to use MozJPEG's [C API](libjpeg.txt) instead.

## Features

* Progressive encoding with "jpegrescan" optimization. It can be applied to any JPEG file (with `jpegtran`) to losslessly reduce file size.
* Trellis quantization. When converting other formats to JPEG it maximizes quality/filesize ratio.
* Comes with new quantization table presets, e.g. tuned for high-resolution displays.
* Fully compatible with all web browsers.
* Can be seamlessly integrated into any program using libjpeg.

## Releases

* [Latest release](https://github.com/mozilla/mozjpeg/releases/latest)
* [Version 3.0 Announcement](https://boomswaggerboom.wordpress.com/2014/12/30/mozjpeg-3-0-released/) ([overview of 3.0 features](https://calendar.perfplanet.com/2014/mozjpeg-3-0/))
* [Version 2.0 Announcement](https://blog.mozilla.org/research/2014/07/15/mozilla-advances-jpeg-encoding-with-mozjpeg-2-0/)
* [Version 1.0 Announcement](https://blog.mozilla.org/research/2014/03/05/introducing-the-mozjpeg-project/)

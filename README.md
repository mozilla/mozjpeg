Mozilla JPEG Encoder Project
============================

This project's goal is to reduce the size of JPEG files without reducing quality or compatibility with the vast majority of the world's deployed decoders.

The idea is to reduce transfer times for JPEGs on the Web, thus reducing page load times.

'mozjpeg' is not intended to be a general JPEG library replacement. It makes tradeoffs that are intended to benefit Web use cases and focuses solely on improving encoding. It is best used as part of a Web encoding workflow. For a general JPEG library (e.g. your system libjpeg), especially if you care about decoding, we recommend libjpeg-turbo.

For more information, see the project announcement:

https://blog.mozilla.org/research/2014/03/05/introducing-the-mozjpeg-project/

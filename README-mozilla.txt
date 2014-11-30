Mozilla JPEG Encoder Project
============================

This project's goal is to reduce the size of JPEG files without reducing
quality or compatibility with the vast majority of the world's deployed decoders.

The idea is to reduce transfer times for JPEGs on the Web, thus reducing page load times.

'mozjpeg' is not intended to be a general JPEG library replacement. It makes tradeoffs that
are intended to benefit Web use cases and focuses solely on improving encoding. It is best
used as part of a Web encoding workflow. For a general JPEG library (e.g. your system libjpeg),
especially if you care about decoding, we recommend libjpeg-turbo.


Data structures
===============

New parameters introduced by the Mozilla JPEG encoder are placed into the jpeg_comp_master data
structure which his not directly accessible. Several functions are introduced to get and set these
parameters:

EXTERN(boolean) jpeg_c_bool_param_supported (j_compress_ptr cinfo, J_BOOLEAN_PARAM param);
EXTERN(void) jpeg_c_set_bool_param (j_compress_ptr cinfo, J_BOOLEAN_PARAM param, boolean value);
EXTERN(boolean) jpeg_c_get_bool_param (j_compress_ptr cinfo, J_BOOLEAN_PARAM param);

EXTERN(boolean) jpeg_c_float_param_supported (j_compress_ptr cinfo, J_FLOAT_PARAM param);
EXTERN(void) jpeg_c_set_float_param (j_compress_ptr cinfo, J_FLOAT_PARAM param, float value);
EXTERN(float) jpeg_c_get_float_param (j_compress_ptr cinfo, J_FLOAT_PARAM param);

EXTERN(boolean) jpeg_c_int_param_supported (j_compress_ptr cinfo, J_INT_PARAM param);
EXTERN(void) jpeg_c_set_int_param (j_compress_ptr cinfo, J_INT_PARAM param, int value);
EXTERN(int) jpeg_c_get_int_param (j_compress_ptr cinfo, J_INT_PARAM param);


Boolean parameters
------------------

* JBOOLEAN_USE_MOZ_DEFAULTS indicates whether the Mozilla default settings should be used. Otherwise
  the behavior reverts to the default from libjpeg-turbo. Note that this parameter should be set
  before calling jpeg_set_defaults(). By default this parameter is enabled.

* JBOOLEAN_OPTIMIZE_SCANS indicates whether to optimize scan parameters. Parameter optimization is
  done as in jpgcrush. By default this parameter is enabled.

* JBOOLEAN_TRELLIS_QUANT indicates whether to apply trellis quantization. For each 8x8 block trellis
  quantization determines the best trade-off between rate and distortion. By default this parameter
  is enabled.

* JBOOLEAN_TRELLIS_QUANT_DC indicates whether to apply trellis quantization to DC coefficients. By
  default this parameter is enabled.

* JBOOLEAN_TRELLIS_EOB_OPT indicates whether to optimize runs of zero blocks in trellis quantization.
  This is applicable only when JBOOLEAN_USE_SCANS_IN_TRELLIS is enabled. By default this parameter
  is disabled.

* JBOOLEAN_USE_LAMBDA_WEIGHT_TBL has currently no effect.

* JBOOLEAN_USE_SCANS_IN_TRELLIS indicates whether multiple scans are considered during trellis
  quantization. By default this parameter is disabled.

* JBOOLEAN_TRELLIS_Q_OPT indicates whether to optimize the quantization table after trellis quantization.
  If enabled a revised quantization table is derived such as to minimize the reconstruction error
  given the quantized coefficients. By default this parameter is disabled.

* JBOOLEAN_OVERSHOOT_DERINGING indicates whether overshooting is applied to samples with extreme
  values (e.g., 0 and 255 for 8-bit samples). Overshooting may reduce ringing artifacts from
  compression, in particular in areas where black text appears on a white background. By default
  this parameter is enabled.

Floating-point parameters
-------------------------

* JFLOAT_LAMBDA_LOG_SCALE1 and JFLOAT_LAMBDA_LOG_SCALE2 determine the lambda value used in
  trellis quantization. By default these parameters are set to 14.75 and 16.5. The lambda value
  (Lagrance multiplier) in the R + lambda * D equation is derived from
  lambda = 2^s1 / ((2^s2 + n) * q^2) where s1 and s2 are the values of JFLOAT_LAMBDA_LOG_SCALE1
  and JFLOAT_LAMBDA_LOG_SCALE2, n is the average of the squared unquantized AC coefficients
  within the current 8x8 block, and q is the quantization table entry associated with the
  current coefficient frequency. If JFLOAT_LAMBDA_LOG_SCALE2 is 0, an alternate form is used that
  does not rely on n: lambda = 2^(s1-12) / q^2.

Integer parameters
------------------

* JINT_TRELLIS_FREQ_SPLIT determines the position within the zigzag scan at which the split between
  scans is positioned in the context of trellis quantization. JBOOLEAN_USE_SCANS_IN_TRELLIS must
  be enabled for this parameter to take effect. By default this parameter is set to value 8.

* JINT_TRELLIS_NUM_LOOPS determines the number of trellis quantization passes. Huffman tables are
  updated between passes. By default this parameter is set to value 1.

* JINT_BASE_QUANT_TBL_IDX determines which quantization table set to use. Multiple sets are defined
  as below. By default this parameter is set to value 3.
  - 0  Tables from JPEG Annex K
  - 1  Flat table
  - 2  Table tuned for MSSIM on Kodak image set
  - 3  Table from http://www.imagemagick.org/discourse-server/viewtopic.php?f=22&t=20333&p=98008#p98008
  - 4  Table tuned for PSNR-HVS-M on Kodak image set
  - 5  Table from Relevance of human vision to JPEG-DCT compression (1992) Klein, Silverstein and Carney
  - 6  Table from DCTune perceptual optimization of compressed dental X-Rays (1997) Watson, Taylor, Borthwick
  - 7  Table from A visual detection model for DCT coefficient quantization (12/9/93) Ahumada, Watson, Peterson
  - 8  Table from An improved detection model for DCT coefficient quantization (1993) Peterson, Ahumada and Watson

* JINT_DC_SCAN_OPT_MODE determines the DC scan optimization mode. Modes are defined as below. By default
  this parameter is set to value 1.
  - 0  One scan for all components
  - 1  One scan per component
  - 2  Optimize between one scan for all components and one scan for 1st component plus one scan for
       remaining components

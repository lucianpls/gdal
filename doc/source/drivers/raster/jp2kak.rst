.. _raster.jp2kak:

================================================================================
JP2KAK -- JPEG 2000 (based on Kakadu SDK)
================================================================================

.. shortname:: JP2KAK

.. build_dependencies:: Kakadu SDK

The JP2KAK driver, which uses the proprietary `Kakadu SDK <http://www.kakadusoftware.com/>`__, supports JPEG 2000 images, which are
specified in the Rec. ITU-T T.8xx | ISO/IEC 15444 family of standards. JPEG 2000
uses a substantially different format and compression mechanism than the
traditional JPEG compression and JPEG JFIF format. JPEG and JPEG 2000 are
distinct compression standards produced by the same group. JPEG 2000 is based on
wavelet compression.

The driver includes support for:

* reading both JPEG 2000 codestreams (.j2c or .jpc) and JP2 files (.jp2). New
  images can be also written, but existing images cannot be updated in place.

* lossy and lossless compression of 8-bit and 16-bit images with 1 or more bands
  (components).

* GeoTIFF style coordinate system and georeferencing information in JP2 file via
  the
  `GeoJP2(tm) <https://web.archive.org/web/20151028081930/http://www.lizardtech.com/download/geo/geotiff_box.txt>`__
  mechanism.

* JPEG 2000 Part 1 (Rec. ITU-T T.800 | ISO/IEC 15444-1), Part 2 (Rec. ITU-T
  T.801 | ISO/IEC 15444-2) and Part 15 (HTJ2K) (Rec. ITU-T T.814 | ISO/IEC 15444-15)

The Kakadu SDK is a high quality and high performance JPEG 2000 library in wide
used in the geospatial and general imaging community. However, it is not free,
and, by default, a builds of GDAL from source does not include support for the
JP2KAK driver. The builder must acquire a license for the Kakadu SDK and
configure GDAL accordingly.

When reading images this driver will represent the bands as being Byte (8-bit
unsigned), 16-bit signed/unsigned, and 32-bit signed/unsigned. Georeferencing
and coordinate system information will be available if the file is a GeoJP2 (tm)
file. Files color encoded in YCbCr color space will be automatically translated
to RGB. Paletted images are also supported.

XMP metadata can be extracted from JPEG 2000 files, and will be stored as XML
raw content in the xml:XMP metadata domain.

Driver capabilities
-------------------

.. supports_createcopy::

.. supports_georeferencing::

.. supports_virtualio::

Configuration Options
---------------------

|about-config-options|
The JP2KAK driver supports the following configuration options:

-  .. config:: JP2KAK_THREADS

      By default an effort is made to take
      advantage of multi-threading on multi-core computers using default
      rules from the Kakadu SDK. This option may be set to a value of
      zero to avoid using additional threads or to a specific count to
      create the requested number of worker threads.

-  .. config:: JP2KAK_FUSSY
      :choices: YES, NO
      :default: NO

      This can be set to YES to turn on fussy
      reporting of problems with the JPEG 2000 data stream.

-  .. config:: JP2KAK_RESILIENT
      :choices: YES, NO
      :default: NO

      This can be set to YES to force Kakadu
      to maximize resilience with incorrectly created JPEG 2000 data files,
      likely at some cost in performance. This is likely to be necessary
      if, among other reasons, you get an error message about "Expected to
      find EPH marker following packet header" or error reports indicating
      the need to run with the resilient and sequential flags on.

-  .. config:: USE_TILE_AS_BLOCK
      :choices: YES, NO
      :default: NO

      Whether to use the JPEG 2000 block size as the GDAL block size.

Georeferencing
--------------

Georeferencing information can come from different sources : internal
(GeoJP2 or GMLJP2 boxes), worldfile .j2w/.wld sidecar files, or PAM
(Persistent Auxiliary metadata) .aux.xml sidecar files. By default,
information is fetched in following order (first listed is the highest
priority): PAM, GeoJP2, GMLJP2, WORLDFILE.

Starting with GDAL 2.2, the allowed sources and their priority order can
be changed with the :config:`GDAL_GEOREF_SOURCES` configuration option (or
:oo:`GEOREF_SOURCES` open option) whose value is a comma-separated list of the
following keywords : PAM, GEOJP2, GMLJP2, INTERNAL (shortcut for
GEOJP2,GMLJP2), WORLDFILE, NONE. Earlier mentioned sources take
priority over later ones. A non mentioned source will be ignored.

For example setting it to "WORLDFILE,PAM,INTERNAL" will make a
geotransformation matrix from a potential worldfile priority over PAM
or internal JP2 boxes. Setting it to "PAM,WORLDFILE,GEOJP2" will use the
mentioned sources and ignore GMLJP2 boxes.

Option Options
--------------

|about-open-options|
The following open options are available:

-  .. oo:: 1BIT_ALPHA_PROMOTION
      :choices: YES, NO
      :default: YES

      Whether a 1-bit alpha channel should be promoted to 8-bit.

-  .. oo:: GEOREF_SOURCES
      :since: 2.2

      Define which georeferencing
      sources are allowed and their priority order. See
      `Georeferencing`_ paragraph.

Creation Issues
---------------

JPEG 2000 files can only be created using the CreateCopy mechanism to
copy from an existing dataset.

JPEG 2000 overviews are maintained as part of the mathematical
description of the image. Overviews cannot be built as a separate
process, but on read the image will generally be represented as having
overview levels at various power of two factors.

|about-creation-options|
The following creation options are supported:

-  .. co:: CODEC
      :choices: JP2, J2K

      Codec to use. If not specified, guess based on file
      extension. If unknown, default to JP2.

-  .. co:: QUALITY
      :default: 20

      Set the compressed size ratio as a percentage of the
      size of the uncompressed image. The default is 20 indicating that the
      resulting image should be 20% of the size of the uncompressed image.
      Actual final image size may not exactly match that requested
      depending on various factors. A value of 100 will result in use of
      the lossless compression algorithm . On typical image data, if you
      specify a value greater than 65, it might be worth trying with
      :co:`QUALITY=100` instead as lossless compression might produce better
      compression than lossy compression.

-  .. co:: BLOCKXSIZE
      :default: 20000

      Set the tile width to use.

-  .. co:: BLOCKYSIZE

      Set the tile height to use. Defaults to image height.

-  .. co:: FLUSH
      :choices: TRUE, FALSE
      :default: TRUE

      Enable/Disable incremental flushing when
      writing files. Required to be FALSE for RLPC and LRPC Corder. May use
      a lot of memory when FALSE while writing large images.

-  .. co:: GMLJP2
      :choices: YES, NO
      :default: YES

      Indicates whether a GML box conforming to the OGC
      GML in JPEG 2000 specification should be included in the file. Unless
      GMLJP2V2_DEF is used, the version of the GMLJP2 box will be version
      1.

-  .. co:: GMLJP2V2_DEF
      :choices: <filename>, <json>, YES

      Indicates whether
      a GML box conforming to the `OGC GML in JPEG 2000, version 2 <http://docs.opengeospatial.org/is/08-085r4/08-085r4.html>`__
      specification should be included in the file. *filename* must point
      to a file with a JSON content that defines how the GMLJP2 v2 box
      should be built. See :ref:`GMLJP2v2 definition file section <gmjp2v2def>` in documentation of
      the JP2OpenJPEG driver for the syntax of the JSON configuration file.
      It is also possible to directly pass the JSON content inlined as a
      string. If filename is just set to YES, a minimal instance will be
      built.

-  .. co:: GeoJP2
      :choices: YES, NO
      :default: YES

      Indicates whether a UUID/GeoTIFF box conforming to
      the GeoJP2 (GeoTIFF in JPEG 2000) specification should be included in
      the file.

-  .. co:: LAYERS
      :default: 12

      Control the number of layers produced. These are sort
      of like resolution layers, but not exactly. The default value of 12
      works well in most situations.

-  .. co:: ROI
      :choices: <xoff\,yoff\,xsize\,ysize>

      Selects a region to be a region of
      interest to process with higher data quality. The various "R" flags
      below may be used to control the amount better. For example the
      settings "ROI=0,0,100,100", "Rweight=7" would encode the top left
      100x100 area of the image with considerable higher quality compared
      to the rest of the image.

The following creation options are tightly tied to the Kakadu SDK, and are
considered to be for advanced use only. Consult the Kakadu SDK documentation to
better understand their meaning.

-  **Corder**: Progression order. Defaults to "PRCL".
-  **Cprecincts**: Precincts settings. Defaults to
   "{512,512},{256,512},{128,512},{64,512},{32,512},{16,512},{8,512},{4,512},{2,512}".
-  **ORGgen_plt**: Whether to generate PLT markers (Paquet Length). Defaults to "yes".
-  **ORGgen_tlm**: Whether to generate TLM markers (Tile Length). Kakadu SDK defaults used.
-  **ORGtparts**: Kakadu SDK defaults used.
-  **Cmodes**: Kakadu SDK defaults used.
-  **Clevels**: Quality levels. Kakadu SDK defaults used.
-  **Cycc** =YES/NO: Whether to use YCbCr (lossy compression) or YUV (lossless compression) color space when encoding RGB images. Default to YES starting with GDAL 3.10 (was hardcoded to NO in previous versions)
-  **Rshift**: Kakadu SDK defaults used.
-  **Rlevels**: Kakadu SDK defaults used.
-  **Rweight**: Kakadu SDK defaults used.
-  **Qguard**: Kakadu SDK defaults used.
-  **Creversible** = YES/NO: If not set and QUALITY >= 99.5, set to "yes", otherwise to "false".
-  **Sprofile**: Kakadu SDK defaults used.
-  **RATE**: Kakadu SDK defaults used.
   One or more bit-rates, expressed in terms of the ratio between the total number of compressed bits
   (including headers) and the product of the largest horizontal and  vertical image component dimensions. A dash, -,
   may be used in place of the first bit-rate in the list to indicate that the final quality layer should include all
   compressed bits. If Clayers is not used, the number of layers is set to the number of rates specified here.
   If Clayers is used to specify an actual number of quality layers, one of the following must be true: 1) the number
   of rates specified here is identical to the specified number of layers; or 2) one or two rates are specified using
   this argument.  When two rates are specified, the number of layers must be 2 or more and intervening layers will be
   assigned roughly logarithmically spaced bit-rates. When only one rate is specified, an internal heuristic determines
   a lower bound and logarithmically spaces the layer rates over the range. The rates have to be in ASC order.

Known Kakadu SDK Issues
-----------------------

Alpha Channel Writing in v7.8
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Kakadu SDK v7.8 has a bug in jp2_channels::set_opacity_mapping that can
cause an error when writing images with an alpha channel. Please upgrade
to version 7.9.

::

   Error: GdalIO: Error in Kakadu File Format Support: Attempting to
   create a Component Mapping (cmap) box, one of whose channels refers to
   a non-existent image component or palette lookup table. (code = 1)

kdu_get_num_processors always returns 0 for some platforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On non-windows / non-mac installs (e.g. Linux), Kakadu SDK might not include
unistd.h in kdu_arch.cpp. This means that \_SC_NPROCESSORS_ONLN and
\_SC_NPROCESSORS_CONF are not defined and kdu_get_num_processors will
always return 0. Therefore the jp2kak driver might not default to
creating worker threads.

Standalone plugin compilation
-----------------------------

.. versionadded:: 3.10

While this driver may be built as part of a whole GDAL build, either in libgdal
itself, or as a plugin, it is also possible to only build this driver as a plugin,
against an already built libgdal.

The version of the GDAL sources used to build the driver must match the version
of the libgdal it is built against.

For example, from a "build_jp2kak" directory under the root of the GDAL source tree:

::

    cmake -S ../frmts/jp2kak -DCMAKE_PREFIX_PATH=/path/to/GDAL_installation_prefix -DKDU_ROOT=/path/to/kakadu_root
    cmake --build .


Note that such a plugin, when used against a libgdal not aware of it, will be
systematically loaded at GDAL driver initialization time, and will not benefit from
`deferred plugin loading capabilities <rfc-96>`. For that, libgdal itself must be built with the
CMake variable GDAL_REGISTER_DRIVER_JP2KAK_FOR_LATER_PLUGIN=ON set.

See Also
--------

-  Implemented as :source_file:`frmts/jp2kak/jp2kakdataset.cpp`.
-  If you're using a Kakadu SDK release before v7.5, configure & compile
   GDAL with eg.
   `CXXFLAGS="-DKDU_MAJOR_VERSION=7 -DKDU_MINOR_VERSION=3 -DKDU_PATCH_VERSION=2"`
   for Kakadu SDK version 7.3.2.
-  Alternate :ref:`raster.jp2openjpeg` driver.

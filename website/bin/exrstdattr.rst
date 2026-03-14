..
  SPDX-License-Identifier: BSD-3-Clause
  Copyright Contributors to the OpenEXR Project.

exrstdattr
##########

::

    exrstdattr [commands] infile outfile

Description
-----------

Read OpenEXR image file infile, set the values of one
or more attributes in the headers of the file, and save
the result in outfile.  Infile and outfile must not refer
to the same file (the program cannot edit an image file "in place").

Options for selecting headers:
------------------------------

.. describe:: -part i

              If i is greater than or equal to zero, and less
              than the number of parts in the input file, then
              the header for part i becomes "current." If i
              is "any" or -1, then all headers become current.
              Subsequent attribute setting commands affect only
              the current header or headers.  All headers are
              current before the first -part command.

              For example, the command sequence

              ::

                  -focus 3 -part 2 -aperture 8 -expTime 0.01 -part any -owner luke

              sets the focus and owner attributes in all
              headers, as well as the aperture and expTime
              attributes in the header of part 2.

Options for setting attribute values:
-------------------------------------

.. describe:: -chromaticities f f f f f f f f
              
              CIE xy chromaticities for the red, green
              and blue primaries, and for the white point
              (8 floats)

.. describe:: -whiteLuminance f

              white luminance, in candelas per square meter
              (float, >= 0.0)

.. describe:: -adoptedNeutral f f

              CIE xy coordinates that should be considered
              "neutral" during color rendering.  Pixels in
              the image file whose xy coordinates match the
              adoptedNeutral value should be mapped to neutral
              values on the display. (2 floats)

.. describe:: -renderingTransform s

              name of the CTL rendering transform for this
              image (string)

.. describe:: -lookModTransform s

              name of the CTL look modification transform for
              this image (string)

.. describe:: -xDensity f

              horizontal output density, in pixels per inch
              (float, >= 0.0)

.. describe:: -owner s

              name of the owner of the image (string)

.. describe:: -comments s

              additional information about the image (string)

.. describe:: -capDate s

              date when the image was created or
              captured, in local time (string,
              formatted as YYYY:MM:DD hh:mm:ss)

.. describe:: -utcOffset f

              offset of local time at capDate from UTC, in
              seconds (float, UTC == local time + x)

.. describe:: -longitude f

.. describe:: -latitude f

.. describe:: -altitude f

              location where the image was recorded, in
              degrees east of Greenwich and north of the
              equator, and in meters above sea level
              (float)

.. describe:: -focus f

              the camera's focus distance, in meters
              (float, > 0, or "infinity")

.. describe:: -expTime f
              
              exposure time, in seconds (float, >= 0)

.. describe:: -aperture f

              lens apterture, in f-stops (float, >= 0)

.. describe:: -isoSpeed f

              effective speed of the film or image
              sensor that was used to record the image
              (float, >= 0)

.. describe:: -envmap s

              indicates that the image is an environment map
              (string, LATLONG or CUBE)

.. describe:: -framesPerSecond i i

              playback frame rate expressed as a ratio of two
              integers, n and d (the frame rate is n/d frames
              per second)

.. describe:: -keyCode i i i i i i i

              key code that uniquely identifies a motion
              picture film frame using 7 integers:

              * film manufacturer code (0 - 99)
              * film type code (0 - 99)
              * prefix to identify film roll (0 - 999999)
              * count, increments once every perfsPerCount
                perforations (0 - 9999)
              * offset of frame, in perforations from
                zero-frame reference mark (0 - 119)
              * number of perforations per frame (1 - 15)
              * number of perforations per count (20 - 120)

.. describe:: -timeCode i i

              SMPTE time and control code, specified as a pair
              of 8-digit base-16 integers.  The first number
              contains the time address and flags (drop frame,
              color frame, field/phase, bgf0, bgf1, bgf2).
              The second number contains the user data and
              control codes.

.. describe:: -wrapmodes s

              if the image is used as a texture map, specifies
              how the image should be extrapolated outside the
              zero-to-one texture coordinate range
              (string, e.g. "clamp" or "periodic,clamp")

.. describe:: -pixelAspectRatio f

              width divided by height of a pixel
              (float, >= 0)

.. describe:: -screenWindowWidth f

              width of the screen window (float, >= 0)

.. describe:: -screenWindowCenter f f

              center of the screen window (2 floats)

.. describe:: -string s s

              custom string attribute
              (2 strings, attribute name and value)

.. describe:: -float s f

              custom float attribute (string + float,
              attribute name and value)

.. describe:: -int s i

              custom integer attribute (string + integer,
              attribute name and value)

Other options:
--------------

.. describe:: -h, --help    

              print this message

.. describe:: --version

              print version information


#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import functools
import operator

import numpy as np
import pytest

import OpenEXR


def flags(*names):
    return functools.reduce(
        operator.or_, (getattr(OpenEXR.ColorMetadataWarning, n) for n in names)
    )


def test_no_color_metadata_is_ok():
    assert OpenEXR.checkColorMetadata({}) == OpenEXR.ColorMetadataWarning.OK


def test_empty_interop_id():
    w = OpenEXR.checkColorMetadata({"colorInteropID": ""})
    assert w == flags("EMPTY_INTEROP_ID")


def test_chromaticities_differ():
    header = {
        "colorInteropID": "lin_ap0_scene",
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_rec709_scene"),
    }
    w = OpenEXR.checkColorMetadata(header)
    assert w == flags("CHROMATICITIES_DIFFER")


def test_matching_chromaticities_is_ok():
    header = {
        "colorInteropID": "lin_ap0_scene",
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_ap0_scene"),
    }
    assert OpenEXR.checkColorMetadata(header) == OpenEXR.ColorMetadataWarning.OK


@pytest.mark.parametrize(
    "attribute, flag",
    [
        ("chromaticities", "DATA_HAS_CHROMATICITIES"),
        ("whiteLuminance", "DATA_HAS_WHITE_LUMINANCE"),
        ("adoptedNeutral", "DATA_HAS_ADOPTED_NEUTRAL"),
    ],
)
def test_data_with_color_attribute(attribute, flag):
    values = {
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_ap0_scene"),
        "whiteLuminance": 100.0,
        "adoptedNeutral": (0.3127, 0.329),
    }
    header = {"colorInteropID": "data", attribute: values[attribute]}
    w = OpenEXR.checkColorMetadata(header)
    assert w == flags(flag)


def test_data_with_all_color_attributes_at_once():
    # A header carrying all three attributes that are contradictory for
    # colorInteropID "data" should report all three flags together, and no
    # others.
    header = {
        "colorInteropID": "data",
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_ap0_scene"),
        "whiteLuminance": 100.0,
        "adoptedNeutral": (0.3127, 0.329),
    }
    w = OpenEXR.checkColorMetadata(header)
    assert w == flags(
        "DATA_HAS_CHROMATICITIES",
        "DATA_HAS_WHITE_LUMINANCE",
        "DATA_HAS_ADOPTED_NEUTRAL",
    )


def test_interop_id_not_shared_only_checked_against_first_part():
    first_part = {"colorInteropID": "lin_ap0_scene"}
    other_part = {"colorInteropID": "lin_rec709_scene"}

    # without a first part to compare against, no warning is raised
    assert OpenEXR.checkColorMetadata(other_part) == OpenEXR.ColorMetadataWarning.OK

    w = OpenEXR.checkColorMetadata(other_part, first_part)
    assert w == flags("INTEROP_ID_NOT_SHARED")

    # "data" is exempt from the shared-attribute rule
    assert (
        OpenEXR.checkColorMetadata({"colorInteropID": "data"}, first_part)
        == OpenEXR.ColorMetadataWarning.OK
    )


def test_aces_flag_requires_ap0_chromaticities():
    header = {
        "acesImageContainerFlag": 1,
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_rec709_scene"),
    }
    w = OpenEXR.checkColorMetadata(header)
    assert w == flags("ACES_FLAG_CHROMATICITIES_NOT_AP0")


def test_aces_flag_no_chromaticities():
    w = OpenEXR.checkColorMetadata({"acesImageContainerFlag": 1})
    assert w == flags("ACES_FLAG_NO_CHROMATICITIES")


def test_aces_flag_multiple_violations_at_once():
    # A header that violates all three of the acesImageContainerFlag rules
    # that a present chromaticities attribute allows: the flag is not 1, the
    # colorInteropID is not lin_ap0_scene, and the chromaticities are not
    # AP0's either. All three should be reported together, and no others.
    header = {
        "acesImageContainerFlag": 0,
        "colorInteropID": "lin_rec709_scene",
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_rec709_scene"),
    }
    w = OpenEXR.checkColorMetadata(header)
    assert w == flags(
        "ACES_FLAG_NOT_ONE",
        "ACES_FLAG_INTEROP_ID_NOT_AP0",
        "ACES_FLAG_CHROMATICITIES_NOT_AP0",
    )


def test_valid_aces_container_is_ok():
    header = {
        "acesImageContainerFlag": 1,
        "colorInteropID": "lin_ap0_scene",
        "chromaticities": OpenEXR.colorInteropIDToChromaticities("lin_ap0_scene"),
    }
    assert OpenEXR.checkColorMetadata(header) == OpenEXR.ColorMetadataWarning.OK


def test_color_metadata_warning_to_string_describes_single_flag():
    s = OpenEXR.colorMetadataWarningToString(
        OpenEXR.ColorMetadataWarning.EMPTY_INTEROP_ID
    )
    assert isinstance(s, str)
    assert len(s) > 0


def test_check_color_metadata_on_written_file(tmp_path):
    height, width = 4, 4
    channels = {"R": np.full((height, width), 0.5, dtype=np.float32)}
    header = {"colorInteropID": ""}

    out_path = tmp_path / "check_color_metadata.exr"
    with OpenEXR.File(header, channels) as out:
        out.write(str(out_path))

    with OpenEXR.File(str(out_path), header_only=True) as f:
        w = OpenEXR.checkColorMetadata(f.header(0))

    assert w == flags("EMPTY_INTEROP_ID")

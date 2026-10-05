#!/usr/bin/env python3

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

import numpy as np
import pytest

import OpenEXR

# The IDs that have a defined chromaticities mapping, per the Color Interop
# Forum recommendation for OpenEXR files.

IDS = [
    "lin_rec709_scene",
    "lin_ap0_scene",
    "lin_ap1_scene",
    "lin_p3d65_scene",
    "lin_rec2020_scene",
    "lin_adobergb_scene",
]


@pytest.mark.parametrize("id", IDS)
def test_round_trip(id):
    chromaticities = OpenEXR.colorInteropIDToChromaticities(id)

    assert isinstance(chromaticities, tuple)
    assert len(chromaticities) == 8

    assert OpenEXR.chromaticitiesToColorInteropID(chromaticities) == id


def test_entries_are_distinct():
    chromaticities = [OpenEXR.colorInteropIDToChromaticities(id) for id in IDS]
    assert len(set(chromaticities)) == len(IDS)


def test_ap0_values():
    # note that the blue y coordinate is negative
    assert OpenEXR.colorInteropIDToChromaticities("lin_ap0_scene") == pytest.approx(
        (0.7347, 0.2653, 0.0, 1.0, 0.0001, -0.077, 0.32168, 0.33767)
    )


def test_rec709_and_adobergb_differ_only_in_green():
    rec709 = OpenEXR.colorInteropIDToChromaticities("lin_rec709_scene")
    adobergb = OpenEXR.colorInteropIDToChromaticities("lin_adobergb_scene")

    # red, blue and white match; only green differs
    assert rec709[0:2] == adobergb[0:2]
    assert rec709[4:8] == adobergb[4:8]
    assert rec709[2:4] != adobergb[2:4]

    # so all four coordinates have to be compared
    green_swapped = rec709[0:2] + adobergb[2:4] + rec709[4:8]
    assert (
        OpenEXR.chromaticitiesToColorInteropID(green_swapped) == "lin_adobergb_scene"
    )


@pytest.mark.parametrize(
    "id", ["unknown", "data", "", "srgb_display", "lin_ap1", "LIN_AP1_SCENE"]
)
def test_unsupported_id_returns_none(id):
    assert OpenEXR.colorInteropIDToChromaticities(id) is None


def test_unmatched_chromaticities_return_none():
    assert (
        OpenEXR.chromaticitiesToColorInteropID((1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0))
        is None
    )


def test_tolerance():
    p3d65 = OpenEXR.colorInteropIDToChromaticities("lin_p3d65_scene")

    within = (p3d65[0] + 0.0005,) + p3d65[1:]
    assert OpenEXR.chromaticitiesToColorInteropID(within) == "lin_p3d65_scene"

    outside = (p3d65[0] + 0.002,) + p3d65[1:]
    assert OpenEXR.chromaticitiesToColorInteropID(outside) is None

    # a tolerance wide enough to cover the difference matches again
    assert (
        OpenEXR.chromaticitiesToColorInteropID(outside, tolerance=0.002)
        == "lin_p3d65_scene"
    )


@pytest.mark.parametrize("chromaticities", [(1.0, 2.0), (), "lin_ap1_scene", None, 1.0])
def test_invalid_chromaticities_raise(chromaticities):
    with pytest.raises(ValueError):
        OpenEXR.chromaticitiesToColorInteropID(chromaticities)


def test_file_round_trip(tmp_path):
    # the chromaticities a file carries can be turned back into an ID

    id = "lin_ap1_scene"

    height, width = 4, 4
    channels = {"R": np.full((height, width), 0.5, dtype=np.float32)}
    header = {
        "chromaticities": OpenEXR.colorInteropIDToChromaticities(id),
        "colorInteropID": id,
    }

    out_path = tmp_path / "color_interop_id.exr"
    with OpenEXR.File(header, channels) as out:
        out.write(str(out_path))

    # note that the header dict is only valid while the file is open
    with OpenEXR.File(str(out_path), header_only=True) as f:
        written_id = f.parts[0].header["colorInteropID"]
        written_chromaticities = f.parts[0].header["chromaticities"]

    assert written_id == id
    assert OpenEXR.chromaticitiesToColorInteropID(written_chromaticities) == id

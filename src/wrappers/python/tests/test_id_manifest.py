#!/usr/bin/env python3

#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.
#

"""Tests for writing and reading EXR with idManifest."""

from __future__ import print_function

import shutil
import subprocess
from pathlib import Path

import numpy as np
import pytest

import OpenEXR

_TEST_DIR = Path(__file__).parent
_MANIFEST_ID_EXR = _TEST_DIR / "manifest_id.exr"


def test_manifest_id_fixture_exists():
    """C++ generator output; used by exrheader smoke test below."""
    assert _MANIFEST_ID_EXR.is_file(), "file manifest_id.exr not found in tests/"

def test_manifest_id_listed_in_exrheader():
    exrheader = shutil.which("exrheader")
    if exrheader is None:
        pytest.skip("exrheader not on PATH")

    result = subprocess.run(
        [exrheader, str(_MANIFEST_ID_EXR)],
        capture_output=True,
        text=True,
        check=True,
    )
    out = result.stdout
    assert "idManifest" in out
    assert "idmanifest" in out.lower()


def test_minimal_read_fixture_like_deepidselect():
    """Walk a read manifest like ``deepidselect.cpp`` (size, groups, schemes, channels, rows).

    Assertions match ``makeTestManifest`` in ``testIDManifest.cpp``.
    """
    with OpenEXR.File(str(_MANIFEST_ID_EXR), header_only=True) as f:
        h = f.parts[0].header
        assert "idManifest" in h

        mid = h["idManifest"]
        assert isinstance(mid, OpenEXR.IDManifest)
        assert mid.size() == 2

        g0 = mid[0]
        assert g0.getHashScheme() == "none"
        assert set(g0.getChannels()) == {"id"}
        assert g0.size() == 4
        assert {int(it.id()): list(it.text()) for it in g0} == {
            1: ["merino/body", "wool"],
            2: ["merino/body", "skin"],
            3: ["merino/body", "skin"],
            4: ["merino/eye", "eye"],
        }

        g1 = mid[1]
        assert g1.getEncodingScheme() == "id2"
        assert g1.getHashScheme() == "MurmurHash3_64"
        assert list(g1.getComponents()) == ["instance"]
        assert set(g1.getChannels()) == {"instance1", "instance2"}
        assert g1.size() == 3

        assert mid.find("id") == 0
        assert mid.find("instance1") == 1
        assert mid.find("missing") == mid.size()


def test_minimal_build_manifest_like_deepidexample():
    """Build channel groups like ``deepidexample`` / full insert overload coverage."""
    m = OpenEXR.IDManifest()
    g = m.add("modelid")
    g2 = m.add({"instance1", "instance2"})
    assert set(g2.getChannels()) == {"instance1", "instance2"}
    g.setEncodingScheme("id")
    g.setHashScheme("MurmurHash3_32")
    g.setComponent("model")
    g.setLifetime(2)
    g2.setLifetime(OpenEXR.IdLifetime.LIFETIME_STABLE)
    assert g2.getLifetime() == OpenEXR.IdLifetime.LIFETIME_STABLE
    h = g.insert("sphere/big")
    assert int(h) != 0
    assert m.size() == 2

    g3 = m.add("id3")
    g3.setComponent("name")
    it = g3.insert(99, "foo")
    assert int(it.id()) == 99
    assert it.text() == ["foo"]

    g4 = m.add("id4")
    g4.setComponents(["model", "material"])
    it2 = g4.insert(100, ["merino/bodies", "wool"])
    assert int(it2.id()) == 100

    g5 = m.add("id5")
    g5.setComponents(["model", "material"])
    g5.setHashScheme("MurmurHash3_32")
    g5.setEncodingScheme("id")
    assert int(g5.insert(["sphere/big", "wool"])) != 0


def test_set_channel_and_channels():
    m = OpenEXR.IDManifest()
    g = OpenEXR.ChannelGroupManifest()
    g.setChannel("solo")
    assert set(g.getChannels()) == {"solo"}
    m.add(g)

    g2 = OpenEXR.ChannelGroupManifest()
    g2.setChannels(["a", "b"])
    assert set(g2.getChannels()) == {"a", "b"}
    m.add(g2)
    assert m.size() == 2


def test_lshift_insert_like_cpp():
    m = OpenEXR.IDManifest()
    g = m.add("id")
    g.setComponents(["model", "material"])
    g.setHashScheme(OpenEXR.ID_MANIFEST_NOTHASHED)
    g.setLifetime(OpenEXR.LIFETIME_STABLE)
    g << 1 << "merino/body" << "wool"
    g << 2 << "merino/body" << "skin"
    assert {int(it.id()): list(it.text()) for it in g} == {
        1: ["merino/body", "wool"],
        2: ["merino/body", "skin"],
    }


def test_find_and_erase_on_group():
    m = OpenEXR.IDManifest()
    g = m.add("id")
    g.setComponent("name")
    g.insert(10, "ten")
    g.insert(20, "twenty")
    assert int(g.find(10).id()) == 10
    assert g.find(20).text() == ["twenty"]
    g.erase(10)
    assert g.size() == 1
    assert {int(it.id()) for it in g} == {20}


def test_merge_manifests():
    m1 = OpenEXR.IDManifest()
    g1 = m1.add("id")
    g1.setComponent("name")
    g1.insert(1, "entryOne")

    m2 = OpenEXR.IDManifest()
    g2 = m2.add("id2")
    g2.setComponent("name")
    g2.insert(2, "entryTwo")

    m3 = OpenEXR.IDManifest()
    assert m3.merge(m1) is False
    assert m3.merge(m2) is False
    assert m3.size() == 2
    assert m3[0].find(1).text() == ["entryOne"]
    assert m3[1].find(2).text() == ["entryTwo"]


def test_roundtrip_idmanifest_write_read(tmp_path):
    """Write idManifest in Python, read back, and compare manifest content."""
    m = OpenEXR.IDManifest()
    g = m.add("id")
    g.setComponents(["model", "material"])
    g.setHashScheme("none")
    g.setLifetime(2)
    g.insert(1, ["merino/body", "wool"])

    height, width = 2, 2
    header = {
        "type": OpenEXR.scanlineimage,
        "compression": OpenEXR.ZIP_COMPRESSION,
        "idManifest": m,
    }
    channels = {
        "R": np.full((height, width), 0.5, dtype=np.float32),
        "G": np.full((height, width), 0.5, dtype=np.float32),
        "B": np.full((height, width), 0.5, dtype=np.float32),
        "id": np.ones((height, width), dtype=np.uint32),
    }

    out_path = tmp_path / "roundtrip_id_manifest.exr"
    with OpenEXR.File(header, channels) as out:
        out.write(str(out_path))

    if (exrheader := shutil.which("exrheader")) is not None:
        result = subprocess.run(
            [exrheader, str(out_path)],
            capture_output=True,
            text=True,
            check=True,
        )
        assert "idManifest" in result.stdout

    with OpenEXR.File(str(out_path), header_only=True) as f:
        mid = f.parts[0].header["idManifest"]

    assert isinstance(mid, OpenEXR.IDManifest)
    assert mid.size() == 1

    g = mid[0]
    assert g.getHashScheme() == "none"
    assert set(g.getChannels()) == {"id"}
    assert list(g.getComponents()) == ["model", "material"]
    assert g.getLifetime() == OpenEXR.IdLifetime.LIFETIME_STABLE
    assert {int(it.id()): list(it.text()) for it in g} == {1: ["merino/body", "wool"]}

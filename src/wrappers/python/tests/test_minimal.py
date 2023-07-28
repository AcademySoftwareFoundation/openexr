import pytest


def test_import():
    import OpenEXR
    assert OpenEXR.__name__ == "OpenEXR"

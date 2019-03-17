try:
    import imath
except ImportError:
    print("Failed to import imath!")
    import traceback
    traceback.print_exc()
    import sys
    print("sys.path=%s"%sys.path)

print("V2f(10) + V2f(20) = %s" % (imath.V2f(10) + imath.V2f(20)))

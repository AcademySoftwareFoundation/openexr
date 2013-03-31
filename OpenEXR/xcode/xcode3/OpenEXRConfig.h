//
// Define and set to 1 if the target system supports a proc filesystem
// compatible with the Linux kernel's proc filesystem.  Note that this
// is only used by a program in the IlmImfTest test suite, it's not
// used by any OpenEXR library or application code.
//

#undef HAVE_LINUX_PROCFS

//
// Define and set to 1 if the target system is a Darwin-based system
// (e.g., OS X).
//

#define HAVE_DARWIN 1

//
// Define and set to 1 if the target system has a complete <iomanip>
// implementation, specifically if it supports the std::right
// formatter.
//

#define HAVE_COMPLETE_IOMANIP 1

//
// Define and set to 1 if the target system has support for large
// stack sizes.
//

#undef HAVE_LARGE_STACK

//
// Version string for runtime access
//
#undef OPENEXR_VERSION_STRING
#undef OPENEXR_PACKAGE_STRING

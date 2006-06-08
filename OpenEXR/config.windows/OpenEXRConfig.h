//
// This is a hard-coded config file for Windows platforms.  The only
// thing you should change is to define HAVE_FRAGMENT_SHADERS to 1
// if you have the NVIDIA Cg SDK installed, and you want exrdisplay
// to use it to accelerate the display of OpenEXR images.  It is off
// by default.
//

//
// Define and set to 1 if the target system has POSIX thread support
// and you want OpenEXR to use it for multithreaded file I/O.
//

/* #undef HAVE_PTHREAD */

//
// Define and set to 1 if the target system supports POSIX semaphores
// and you want OpenEXR to use them; otherwise, OpenEXR will use its
// own semaphore implementation.
//

/* #undef HAVE_POSIX_SEMAPHORES */

//
// Define and set to 1 if the target system is a Darwin-based system
// (e.g., OS X).
//

/* #undef HAVE_DARWIN */

//
// Define and set to 1 if the target system supports a proc filesystem
// compatible with the Linux kernel's proc filesystem.  Note that this
// is only used by a program in the IlmImfTest test suite, it's not
// used by any OpenEXR library or application code.
//

/* #undef HAVE_LINUX_PROCFS */

//
// Define and set to 1 if the target system includes the NVIDIA Cg
// runtime.  The exrdisplay program will use a fragment shader to
// accelerate the display of OpenEXR images.
//

/* #define HAVE_FRAGMENT_SHADERS 1 */

//
// Define and set to 1 if the target system has a complete <iomanip>
// implementation, specifically if it supports the std::right
// formatter.
//

#define HAVE_COMPLETE_IOMANIP 1

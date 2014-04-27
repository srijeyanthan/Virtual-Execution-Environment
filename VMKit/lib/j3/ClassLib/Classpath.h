//===-------- Classpath.h - Configuration for classpath -------------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Historically has been included here, keep it for now
#include "j3/jni.h"
#include "vmkit/System.h"

#ifndef USE_OPENJDK

// GNU Classpath values
#define GNUClasspathLibs "/usr/lib/classpath"
#define GNUClasspathGlibj "/usr/share/classpath/glibj.zip"
#define GNUClasspathVersion "0.97.2"

#define ClasslibBootEnv GNUClasspathGlibj
#define ClasslibLibEnv GNUClasspathLibs

#else

// OpenJDK values
#define OpenJDKPath "/usr/lib/jvm/java-1.6.0-openjdk-amd64"

// Select which architecture directory name to use
#if ARCH_X86
#define OpenJDKArchComp "i386"
#elif ARCH_X64
#define OpenJDKArchComp "amd64"
#else
#error "Unsupported architecture"
#endif

// Build various other paths from OpenJDKPath
// Done this way to make them compile-time constants.
#define OpenJDKJRE OpenJDKPath "/jre"
#define OpenJDKArchDir OpenJDKJRE "/lib/" OpenJDKArchComp
#define OpenJDKLibJava OpenJDKArchDir "/libjava.so"
#define OpenJDKExtDirs OpenJDKJRE "/lib/ext"

// Search path for native library files
#define OpenJDKSystemLibPaths "/lib:/lib64:/usr/lib:/usr/lib64"
#define OpenJDKLibPath \
      OpenJDKArchDir \
  ":" OpenJDKArchDir "/server/" \
  ":" OpenJDKSystemLibPaths

// Bootstrap path. Defined in JavaUpcalls.cpp to add VMString location.
extern const char * OpenJDKBootPath;

#define ClasslibBootEnv OpenJDKBootPath
#define ClasslibLibEnv OpenJDKLibPath

#endif // USE_OPENJDK

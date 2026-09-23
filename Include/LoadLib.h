#ifndef LOADLIB_H
#define LOADLIB_H

/*
	One portable shim for loading a shared library and binding its entry points.

	ITURHFProp, CircuitCSV, P533 and ITURNoise each wrote out their own
	#ifdef _WIN32 / LoadLibrary / GetProcAddress / else dlopen / dlsym block
	against overlapping symbol sets, and the four had drifted: they disagreed on
	whether to check the resolved pointers, whether to close the handle on
	failure, and whether to return an error or call exit(). Binding through one
	table removes that whole class of divergence.

	Header-only, because the four callers are in three separately built
	artifacts and none of them links to the others.
*/

#include <stdio.h>

#ifdef _WIN32
	#include <windows.h>
	#define HFLIBHANDLE       HMODULE
	#define hfLibOpen(name)   LoadLibrary(name)
	#define hfLibSym(h, s)    ((void *)GetProcAddress((h), (s)))
	#define hfLibClose(h)     FreeLibrary(h)
	#define hfLibError()      "see GetLastError()"
	#define HFLIB_P372        "P372.dll"
	#define HFLIB_P533        "P533.dll"
#else
	#include <dlfcn.h>
	#define HFLIBHANDLE       void *
	#define hfLibOpen(name)   dlopen((name), RTLD_NOW)
	#define hfLibSym(h, s)    dlsym((h), (s))
	#define hfLibClose(h)     dlclose(h)
	#define hfLibError()      dlerror()
	#define HFLIB_P372        "libp372.so"
	#define HFLIB_P533        "libp533.so"
#endif

// One row per entry point: the exported name, and where to put the pointer.
struct hfSymbol {
	const char *name;
	void      **slot;
};

/*
	hfLibBind() - Binds every symbol in a table, or reports the first missing.

		INPUT
			HFLIBHANDLE h        an open library handle
			const struct hfSymbol *tab
			int n                number of entries
			const char *who      caller name, for the message

		OUTPUT
			returns 1 when every symbol resolved, 0 otherwise

		SUBROUTINES
			None
*/
static int hfLibBind(HFLIBHANDLE h, const struct hfSymbol *tab, int n, const char *who) {

	int i;

	for (i = 0; i < n; i++) {
		*(tab[i].slot) = hfLibSym(h, tab[i].name);
		if (*(tab[i].slot) == NULL) {
			printf("%s: ERROR entry point '%s' not found\n", who, tab[i].name);
			return 0;
		}
	}

	return 1;

}

#endif // LOADLIB_H

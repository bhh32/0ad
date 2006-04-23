#include "precompiled.h"

#include <unistd.h>
#include <stdio.h>
#include <wchar.h>

#include "sdl.h"
#include "lib.h"
#include "sysdep/sysdep.h"
#include "udbg.h"

#define GNU_SOURCE
#include <dlfcn.h>

// these are basic POSIX-compatible backends for the sysdep.h functions.
// Win32 has better versions which override these.

void sys_display_msg(const char* caption, const char* msg)
{
	fprintf(stderr, "%s: %s\n", caption, msg);
}

void sys_display_msgw(const wchar_t* caption, const wchar_t* msg)
{
	fwprintf(stderr, L"%ls: %ls\n", caption, msg);
}


LibError sys_get_executable_name(char* n_path, size_t buf_size)
{
	Dl_info dl_info;

	memset(&dl_info, 0, sizeof(dl_info));
	if (!dladdr((void *)sys_get_executable_name, &dl_info) ||
		!dl_info.dli_fname )
	{
		return ERR_NO_SYS;
	}

	strncpy(n_path, dl_info.dli_fname, buf_size);
	return ERR_OK;
}

extern int cpus;
int unix_get_cpu_info()
{
	long res = sysconf(_SC_NPROCESSORS_CONF);
	if (res == -1)
		cpus = 1;
	else
		cpus = (int)res;
	return 0;
}

// apparently not possible on non-Windows OSes because they seem to lack
// a CPU affinity API. see sysdep.h comment.
LibError sys_on_each_cpu(void(*cb)())
{
	UNUSED2(cb);

	return ERR_NO_SYS;
}

ErrorReaction sys_display_error(const wchar_t* text, int flags)
{
	printf("%ls\n\n", text);

	const bool manual_break   = flags & DE_MANUAL_BREAK;
	const bool allow_suppress = flags & DE_ALLOW_SUPPRESS;
	const bool no_continue    = flags & DE_NO_CONTINUE;

	// until valid input given:
	for(;;)
	{
		if(!no_continue)
			printf("(C)ontinue, ");
		if(allow_suppress)
			printf("(S)uppress, ");
		printf("(B)reak, Launch (D)ebugger, or (E)xit?\n");
		// TODO Should have some kind of timeout here.. in case you're unable to
		// access the controlling terminal (As might be the case if launched
		// from an xterm and in full-screen mode)
		int c = getchar();
		// note: don't use tolower because it'll choke on EOF
		switch(c)
		{
		case EOF:
		case 'd': case 'D':
			udbg_launch_debugger();
			//-fallthrough

		case 'b': case 'B':
			if(manual_break)
				return ER_BREAK;
			debug_break();
			return ER_CONTINUE;

		case 'c': case 'C':
			if(!no_continue)
                return ER_CONTINUE;
			// continue isn't allowed, so this was invalid input. loop again.
			break;
		case 's': case 'S':
			if(allow_suppress)
				return ER_SUPPRESS;
			// suppress isn't allowed, so this was invalid input. loop again.
			break;

		case 'e': case 'E':
			abort();
			return ER_EXIT;	// placebo; never reached
		}
	}
}


LibError sys_error_description_r(int err, char* buf, size_t max_chars)
{
	UNUSED2(err);
	UNUSED2(buf);
	UNUSED2(max_chars);

	// don't need to do anything: lib/errors.cpp already queries
	// libc's strerror(). if we ever end up needing translation of
	// e.g. Qt or X errors, that'd go here.
	return ERR_FAIL;
}

// stub for sys_cursor_create - we don't need to implement this (SDL/X11 only
// has monochrome cursors so we need to use the software cursor anyways)

// note: do not return ERR_NOT_IMPLEMENTED or similar because that
// would result in WARN_ERRs.
LibError sys_cursor_create(uint w, uint h, void* bgra_img,
	uint hx, uint hy, void** cursor)
{
	UNUSED2(w);
	UNUSED2(h);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(bgra_img);

	*cursor = 0;
	return ERR_OK;
}

// creates an empty cursor
LibError sys_cursor_create_empty(void **cursor)
{
	/* bitmap for a fully transparent cursor */
	u8 data[] = {0};
	u8 mask[] = {0};

	// size 8x1 (cursor size must be a whole number of bytes ^^)
	// hotspot at 0,0
	// SDL will make its own copies of data and mask
	*cursor=SDL_CreateCursor(data, mask, 8, 1, 0, 0);

	return cursor?ERR_OK:ERR_FAIL;
}

SDL_Cursor *defaultCursor=NULL;
// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
LibError sys_cursor_set(void* cursor)
{
	// Gaah, SDL doesn't have a good API for setting the default cursor
	// SetCursor(NULL) just /repaints/ the cursor (well, obviously! or...)
	ONCE(defaultCursor = SDL_GetCursor());

	// restore default cursor.
	if(!cursor)
		SDL_SetCursor(defaultCursor);

	SDL_SetCursor((SDL_Cursor *)cursor);

	return ERR_OK;
}

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
LibError sys_cursor_free(void* cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return ERR_OK;

	// if the cursor being freed is active, restore the default cursor
	// (just for safety).
	if (SDL_GetCursor() == (SDL_Cursor *)cursor)
		WARN_ERR(sys_cursor_set(NULL));

	SDL_FreeCursor((SDL_Cursor *)cursor);

	return ERR_OK;
}

// note: just use the sector size: Linux aio doesn't really care about
// the alignment of buffers/lengths/offsets, so we'll just pick a
// sane value and not bother scanning all drives.
size_t sys_max_sector_size()
{
	// users may call us more than once, so cache the results.
	static size_t cached_sector_size;
	if(!cached_sector_size)
		cached_sector_size = sysconf(_SC_PAGE_SIZE);
	return cached_sector_size;
}

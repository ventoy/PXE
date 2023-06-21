/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2017  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_VERIFY_HEADER
#define GRUB_VERIFY_HEADER 1

#include <grub/file.h>
#include <grub/list.h>

enum grub_verify_flags
  {
    GRUB_VERIFY_FLAGS_SKIP_VERIFICATION	= 1,
    GRUB_VERIFY_FLAGS_SINGLE_CHUNK	= 2,
    /* Defer verification to another authority. */
    GRUB_VERIFY_FLAGS_DEFER_AUTH	= 4
  };

enum grub_verify_string_type
  {
    GRUB_VERIFY_KERNEL_CMDLINE,
    GRUB_VERIFY_MODULE_CMDLINE,
    GRUB_VERIFY_COMMAND,
  };

struct grub_file_verifier
{
  struct grub_file_verifier *next;
  struct grub_file_verifier **prev;

  const char *name;

  /*
   * Check if file needs to be verified and set up context.
   * init/read/fini is structured in the same way as hash interface.
   */
  grub_err_t (*init) (grub_file_t io, enum grub_file_type type,
		      void **context, enum grub_verify_flags *flags);

  /*
   * Right now we pass the whole file in one call but it may
   * change in the future. If you insist on single buffer you
   * need to set GRUB_VERIFY_FLAGS_SINGLE_CHUNK in verify_flags.
   */
  grub_err_t (*write) (void *context, void *buf, grub_size_t size);

  grub_err_t (*fini) (void *context);
  void (*close) (void *context);

  grub_err_t (*verify_string) (char *str, enum grub_verify_string_type type);
};

extern struct grub_file_verifier *grub_file_verifiers;

static inline void
grub_verifier_register (struct grub_file_verifier *ver)
{
  grub_list_push (GRUB_AS_LIST_P (&grub_file_verifiers), GRUB_AS_LIST (ver));
}

static inline void
grub_verifier_unregister (struct grub_file_verifier *ver)
{
  grub_list_remove (GRUB_AS_LIST (ver));
}

grub_err_t
grub_verify_string (char *str, enum grub_verify_string_type type);

#endif /* ! GRUB_VERIFY_HEADER */

/* cat.c - command to show the contents of a file  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2007,2008  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/charset.h>

GRUB_MOD_LICENSE ("GPLv3+");

#if 0
static const struct grub_arg_option options[] =
  {
    {"dos", -1, 0, N_("Accept DOS-style CR/NL line endings."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };
#endif

grub_err_t
grub_cmd_cat (grub_extcmd_context_t ctxt, int argc, char **args)
{
  struct grub_arg_list *state = ctxt->state;
  int dos = 0;
  grub_file_t file;
  unsigned char buf[GRUB_DISK_SECTOR_SIZE];
  grub_ssize_t size;
  int key = GRUB_TERM_NO_KEY;
  grub_uint32_t code = 0;
  int count = 0;
  unsigned char utbuf[GRUB_MAX_UTF8_PER_CODEPOINT + 1];
  int utcount = 0;
  int is_0d = 0;
  int j;

  if (state[0].set)
    dos = 1;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = grub_file_open (args[0], GRUB_FILE_TYPE_CAT);
  if (! file)
    return grub_errno;

  while ((size = grub_file_read (file, buf, sizeof (buf))) > 0
	 && key != GRUB_TERM_ESC)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  utbuf[utcount++] = buf[i];

	  if (is_0d && buf[i] != '\n')
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      grub_printf ("<%x>", (int) '\r');
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	    }

	  is_0d = 0;

	  if (!grub_utf8_process (buf[i], &code, &count))
	    {
	      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	      for (j = 0; j < utcount - 1; j++)
		grub_printf ("<%x>", (unsigned int) utbuf[j]);
	      code = 0;
	      count = 0;
	      if (utcount == 1 || !grub_utf8_process (buf[i], &code, &count))
		{
		  grub_printf ("<%x>", (unsigned int) buf[i]);
		  code = 0;
		  count = 0;
		  utcount = 0;
		  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
		  continue;
		}
	      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	      utcount = 1;
	    }
	  if (count)
	    continue;

	  if ((code >= 0xa1 || grub_isprint (code)
	       || grub_isspace (code)) && code != '\r')
	    {
	      grub_printf ("%C", code);
	      count = 0; 
	      code = 0;
	      utcount = 0;
	      continue;
	    }

	  if (dos && code == '\r')
	    {
	      is_0d = 1;
	      count = 0; 
	      code = 0;
	      utcount = 0;
	      continue;
	    }

	  grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
	  for (j = 0; j < utcount; j++)
	    grub_printf ("<%x>", (unsigned int) utbuf[j]);
	  grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
	  count = 0; 
	  code = 0;
	  utcount = 0;
	}

      do
	key = grub_getkey_noblock ();
      while (key != GRUB_TERM_ESC && key != GRUB_TERM_NO_KEY);
    }

  if (is_0d)
    {
      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
      grub_printf ("<%x>", (unsigned int) '\r');
      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
    }

  if (utcount)
    {
      grub_setcolorstate (GRUB_TERM_COLOR_HIGHLIGHT);
      for (j = 0; j < utcount; j++)
	grub_printf ("<%x>", (unsigned int) utbuf[j]);
      grub_setcolorstate (GRUB_TERM_COLOR_STANDARD);
    }

  grub_xputs ("\n");
  grub_refresh ();
  grub_file_close (file);

  return 0;
}

#if 0
static grub_extcmd_t cmd;

GRUB_MOD_INIT(cat)
{
  cmd = grub_register_extcmd ("cat", grub_cmd_cat, 0,
			      N_("FILE"), N_("Show the contents of a file."),
			      options);
}

GRUB_MOD_FINI(cat)
{
  grub_unregister_extcmd (cmd);
}
#endif



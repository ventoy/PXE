/* datetime.c - Module for common datetime function.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/datetime.h>
#include <grub/i18n.h>

static const char *const grub_weekday_names[] =
{
  N_("Sunday"),
  N_("Monday"),
  N_("Tuesday"),
  N_("Wednesday"),
  N_("Thursday"),
  N_("Friday"),
  N_("Saturday"),
};

int
grub_get_weekday (struct grub_datetime *datetime)
{
  unsigned a, y, m;

  if (datetime->month <= 2)
    a = 1;
  else
    a = 0;
  y = datetime->year - a;
  m = datetime->month + 12 * a - 2;

  return (datetime->day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
}

const char *
grub_get_weekday_name (struct grub_datetime *datetime)
{
  return _ (grub_weekday_names[grub_get_weekday (datetime)]);
}

#define SECPERMIN 60
#define SECPERHOUR (60*SECPERMIN)
#define SECPERDAY (24*SECPERHOUR)
#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)


void
grub_unixtime2datetime (grub_int32_t nix, struct grub_datetime *datetime)
{
  int i;
  grub_uint8_t months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  /* In the period of validity of unixtime all years divisible by 4
     are bissextile*/
  /* Convenience: let's have 3 consecutive non-bissextile years
     at the beginning of the counting date. So count from 1901. */
  int days_epoch;
  /* Number of days since 1st Januar, 1901.  */
  unsigned days;
  /* Seconds into current day.  */
  unsigned secs_in_day;
  /* Transform C divisions and modulos to mathematical ones */
  if (nix < 0)
    days_epoch = -(((unsigned) (SECPERDAY-nix-1)) / SECPERDAY);
  else
    days_epoch = ((unsigned) nix) / SECPERDAY;
  secs_in_day = nix - days_epoch * SECPERDAY;
  days = days_epoch + 69 * DAYSPERYEAR + 17;

  datetime->year = 1901 + 4 * (days / DAYSPER4YEARS);
  days %= DAYSPER4YEARS;
  /* On 31st December of bissextile years 365 days from the beginning
     of the year elapsed but year isn't finished yet */
  if (days / DAYSPERYEAR == 4)
    {
      datetime->year += 3;
      days -= 3*DAYSPERYEAR;
    }
  else
    {
      datetime->year += days / DAYSPERYEAR;
      days %= DAYSPERYEAR;
    }
  for (i = 0; i < 12
	 && days >= (i==1 && datetime->year % 4 == 0
		      ? 29 : months[i]); i++)
    days -= (i==1 && datetime->year % 4 == 0
			    ? 29 : months[i]);
  datetime->month = i + 1;
  datetime->day = 1 + days;
  datetime->hour = (secs_in_day / SECPERHOUR);
  secs_in_day %= SECPERHOUR;
  datetime->minute = secs_in_day / SECPERMIN;
  datetime->second = secs_in_day % SECPERMIN;
}

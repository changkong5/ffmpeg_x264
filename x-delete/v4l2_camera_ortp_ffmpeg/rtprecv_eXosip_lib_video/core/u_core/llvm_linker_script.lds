/* Script for -z combreloc -z separate-code */
/* Copyright (C) 2014-2022 Free Software Foundation, Inc.
   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OVERWRITE_SECTIONS
{
  .initcall :
  {
    KEEP(*(.data.initcall.*))
    SORT(CONSTRUCTORS)
  }
}

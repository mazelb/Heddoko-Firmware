/*
 * common.c
 *
 * Created: 9/21/2015 8:34:08 AM
 *  Author: sean
 * Copyright Heddoko(TM) 2015, all rights reserved
 */ 

int itoa(int value, char* sp, int radix)
{
	 char tmp[16];		// be careful with the length of the buffer
	 char *tp = tmp;
	 int i;
	 unsigned v;

	 int sign = (radix == 10 && value < 0);
	 if (sign)
		 v = -value;
	 else
		v = (unsigned)value;

	 while (v || tp == tmp)
	 {
		 i = v % radix;
		 v /= radix;	// v/=radix uses less CPU clocks than v=v/radix does
		 if (i < 10)
			*tp++ = i+'0';
		 else
			*tp++ = i + 'a' - 10;
	 }

	 int len = tp - tmp;

	 if (sign)
	 {
		 *sp++ = '-';
		 len++;
	 }

	 while (tp > tmp)
	 *sp++ = *--tp;

	 return len;
}
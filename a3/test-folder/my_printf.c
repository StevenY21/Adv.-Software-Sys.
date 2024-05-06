#include <stdio.h>
#include <stdlib.h>
// itoa provided by assign1
/* Convert the integer D to a string and save the string in BUF. If
   BASE is equal to 'd', interpret that D is decimal, and if BASE is
   equal to 'x', interpret that D is hexadecimal. */
void itoa (char *buf, int base, int d)
{
  char *p = buf;
  char *p1, *p2;
  unsigned long ud = d;
  int divisor = 10;
     
  /* If %d is specified and D is minus, put `-' in the head. */
  if (base == 'd' && d < 0)
    {
      *p++ = '-';
      buf++;
      ud = -d;
    }
  else if (base == 'x')
    divisor = 16;
     
  /* Divide UD by DIVISOR until UD == 0. */
  do
    {
      int remainder = ud % divisor;
     
      *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    }
  while (ud /= divisor);
     
  /* Terminate BUF. */
  *p = 0;
     
  /* Reverse BUF. */
  p1 = buf;
  p2 = p - 1;
  while (p1 < p2)
    {
      char tmp = *p1;
      *p1 = *p2;
      *p2 = tmp;
      p1++;
      p2--;
    }
}


int main(int argc, char *argv[]) {
// You are not allowed to use va_list, va_start, va_arg and va_end. 
//You must attempt to find an alternative, possibly less portable, way of handling variable arguments. 
//Your printf should support the following format codes: %s, %c, %d, %u and %x.
// %s = string of characters, %c is a char, %d base 10 decimal integer, %u unisgned decimal integer %x number in hexadecimal/base 16

// args that will be accepted : a formatting string, plus the values needed for the format codes in the string
// traverse the string, and stop when u see a %. check if the code is a valid code, and MAYBE check if variable type matches dam code, but thats feeling optional ngl
// for integers use itoa to convert to string
// for strings and chars, look into just appending it to the final result
}
/* APPLE LOCAL file ARM 5683689 */

/* Verify that the correct builtin definition is generated by default
   when generating code for the ARM architecture.  */

/* { dg-do compile { target powerpc*-*-darwin* i?86-*-darwin* } } */

#ifdef __ENVIRONMENT_ASPEN_VERSION_MIN_REQUIRED__
#error TEST FAILS
#endif

int main(void)
{
  return 0;
}

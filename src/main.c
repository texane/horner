#include <stdio.h>
#include <math.h>

inline static unsigned int to_index
(unsigned int n, unsigned int polynom_degree)
{
  /* degree to index
     deg the polynom degree
     n the degree to translate
  */

  return polynom_degree - n;
}

#if 0 /* todo */

inline static unsigned int to_degree
(unsigned int i, unsigned int polynom_degree)
{
  /* index to degree
     deg the polynom degree
     n the degree to translate
  */

  return polynom_degree - n;
}

typedef struct horner_work
{
  kaapi_workqueue_t wq;

  unsigned int degree;
  const double* a;

} horner_work_t;

static double horner_seq
(double x, const double* a, unsigned int n);

static double horner_par
(double x, const double* a, unsigned int n)
{
  /* todo */

  /* degrees */
  unsigned int hi, lo;

  while (extract_seq(&hi, &lo) != -1)
  {
    horner_seq(x, a, n, hi, lo);
  }
    

  return 4.;
}

static double horner_seq
(
 double x, const double* a, unsigned int n,
 unsigned int hi, unsigned int lo
)
{
  /* [hi, lo[ degree range */

  /* the current result */
  double res = 0.f;

  /* the degree being eval */
  unsigned int i;
  for (i = n + 1; i; --i)
    res = res * x + a[to_index(i - 1, n)];

  return res;
}
#endif /* todo */

static double horner_seq
(double x, const double* a, unsigned int n)
{
  /* x the point to be evaluated
     a the coeffs, ordered descending [an, an-1, ..., a0]
     n the polynom degree
   */

  /* the current result */
  double res = 0.f;

  /* the degree being eval */
  unsigned int i;
  for (i = n + 1; i; --i)
    res = res * x + a[to_index(i - 1, n)];

  return res;
}

static double eval_seq
(double x, const double* a, unsigned int n)
{
  double res;
  unsigned int i;
  for (res = 0., i = 0; i <= n; ++i)
    res += pow(x, (double)i) * a[to_index(i, n)];
  return res;
}

int main(int ac, char** av)
{
  /* 4x^3 + 3x^2 + 2x + 1 */
  static double a[] = { 4., 3., 2., 1. };
  static const double x = 2.;
  static const unsigned int n = sizeof(a) / sizeof(double) - 1;

  printf("%lf %lf\n",
	 eval_seq(x, a, n),
	 horner_seq(x, a, n));

  return 0;
}

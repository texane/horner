#include <stdio.h>
#include <sys/types.h>


// parallel implementation

#include "kaLinearWork.hh"

class varWork;

class varResult : public ka::linearWork::baseResult
{
public:
  double _sum_x;
  double _sum_xx;

  varResult() : _sum_x(0.), _sum_xx(0.) { }

  void initialize(const varWork&)
  { _res = 0.; }
};


class varWork : public ka::linearWork::baseWork
{
  // compute the sequence variance

public:

  static const bool is_reducable = true;
  static const unsigned int seq_grain = 256;
  static const unsigned int par_grain = 256;

  const double* _x;

  varWork(const double* x) : _x(x) {}

  void initialize(const varWork& w)
  {
    // initialize the splitted work
    _x = w._x;
  }

  void execute(varResult& res, const ka::linearWork::range& r)
  {
    double sum_x = 0.;
    double sum_xx = 0.;

    for (range::index_type i = r.begin(); i < r.end(); ++i)
    {
      const double x = _x[i];
      sum_x += x;
      sum_xx += x * x;
    }

    res._sum_x += sum_x;
    res._sum_xx += sum_xx;
  }

  void reduce
  (ka::linearWork::varResult& lhs, const ka::linearWork::varResult& rhs)
  {
    lhs._sum_x += rhs._sum_x;
    lhs._sum_xx += rhs._sum_xx;
  }

};


static double var
(const double* x, size_t n, bool)
{
  varWork work(x, n);
  varResult res;
  ka::linearWork::execute(work, res);
  const double ave = res._sum_x / (double)n;
  return res._sum_xx + (double)n * (ave * ave) - 2. * ave * res._sum_x;
}


// sequential implem

static double mean(const double* x, size_t n)
{
  double sum = 0.;
  for (size_t i = 0; i < n; ++i) sum += x[i];
  return sum / (double)n;
}

static double var(const double* x, size_t n)
{
  const double ave = mean(x, n);

  double sum = 0.;
  for (size_t i = 0; i < n; ++i)
  {
    const double dif = x[i] - ave;
    sum += dif * dif;
  }

  return sum / (double)n;
}


int main(int ac, char** av)
{
  double x[] = { 0, 1, 2, 3, 4, 5 };
  const size_t n = sizeof(x) / sizeof(x[0]);

  ka::linearWork::toRemove::initialize();
  printf("%lf %lf\n", var(x, n), var(x, n, true));
  ka::linearWork::toRemove::finalize();

  return 0;
}

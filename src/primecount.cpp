///
/// @file  primecount.cpp
/// @brief Function definitions of primecount.hpp
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primecount.hpp>
#include <primecount-internal.hpp>
#include <calculator.hpp>
#include <int128_t.hpp>
#include <imath.hpp>

#include <algorithm>
#include <ctime>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <stdint.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

#ifdef HAVE_MPI

#include <mpi.h>

namespace primecount {

int mpi_num_procs()
{
  int procs;
  MPI_Comm_size(MPI_COMM_WORLD, &procs);
  return procs;
}

int mpi_proc_id()
{
  int proc_id;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
  return proc_id;
}

int mpi_master_proc_id()
{
  return 0;
}

bool is_mpi_master_proc()
{
  return mpi_proc_id() == mpi_master_proc_id();
}

} // namespace

#endif

using namespace std;

namespace {

#ifdef _OPENMP
  int threads_ = 0;
#endif

int status_precision_ = -1;

double alpha_ = -1;

// Below 10^7 LMO is faster than Deleglise-Rivat
const int lmo_threshold = 10000000;

}

namespace primecount {

int64_t pi(int64_t x)
{
  return pi(x, get_num_threads());
}

int64_t pi(int64_t x, int threads)
{
  if (x < lmo_threshold)
    return pi_lmo5(x);
  else
    return pi_deleglise_rivat(x, threads);
}

#ifdef HAVE_INT128_T

int128_t pi(int128_t x)
{
  return pi(x, get_num_threads());
}

int128_t pi(int128_t x, int threads)
{
  // use 64-bit if possible
  if (x <= numeric_limits<int64_t>::max())
    return pi((int64_t) x, threads);
  else
    return pi_deleglise_rivat(x, threads);
}

#endif

string pi(const string& x)
{
  return pi(x, get_num_threads());
}

string pi(const string& x, int threads)
{
  maxint_t pi_x = pi(to_maxint(x), threads);
  ostringstream oss;
  oss << pi_x;
  return oss.str();
}

int64_t pi_legendre(int64_t x)
{
  return pi_legendre(x, get_num_threads());
}

int64_t pi_lehmer(int64_t x)
{
  return pi_lehmer(x, get_num_threads());
}

int64_t pi_lmo(int64_t x)
{
  return pi_lmo(x, get_num_threads());
}

int64_t pi_lmo(int64_t x, int threads)
{
  return pi_lmo_parallel(x, threads);
}

int64_t pi_meissel(int64_t x)
{
  return pi_meissel(x, get_num_threads());
}

int64_t pi_primesieve(int64_t x)
{
  return pi_primesieve(x, get_num_threads());
}

int64_t pi_deleglise_rivat(int64_t x)
{
  return pi_deleglise_rivat(x, get_num_threads());
}

int64_t pi_deleglise_rivat(int64_t x, int threads)
{
  return pi_deleglise_rivat_parallel1(x, threads);
}

#ifdef HAVE_INT128_T

int128_t pi_deleglise_rivat(int128_t x)
{
  return pi_deleglise_rivat(x, get_num_threads());
}

int128_t pi_deleglise_rivat(int128_t x, int threads)
{
  // use 64-bit if possible
  if (x <= numeric_limits<int64_t>::max())
    return pi_deleglise_rivat((int64_t) x, threads);
  else
    return pi_deleglise_rivat_parallel2(x, threads);
}

#endif

int64_t nth_prime(int64_t n)
{
  return nth_prime(n, get_num_threads());
}

int64_t phi(int64_t x, int64_t a)
{
  return phi(x, a, get_num_threads());
}

/// Returns the largest integer supported by pi(x). The
/// return type is a string as get_max_x() can be a 128-bit
/// integer which is not supported by all compilers.
///
string get_max_x(double alpha)
{
  ostringstream oss;

#ifdef HAVE_INT128_T
  // primecount is limited by:
  // z <= 2^62, with z = x^(2/3) / alpha
  // x^(2/3) / alpha <= 2^62
  // x <= (2^62 * alpha)^(3/2)
  //
  double max_x = pow(pow(2.0, 62.0) * alpha, 3.0 / 2.0);
  oss << (int128_t) max_x; 
#else
  unused_param(alpha); 
  oss << numeric_limits<int64_t>::max();
#endif

  return oss.str();
}

/// Get the wall time in seconds
double get_wtime()
{
#ifdef _OPENMP
  return omp_get_wtime();
#else
  return (double) std::clock() / CLOCKS_PER_SEC;
#endif
}

int ideal_num_threads(int threads, int64_t sieve_limit, int64_t thread_threshold)
{
  thread_threshold = max((int64_t) 1, thread_threshold);
  threads = (int) min((int64_t) threads, sieve_limit / thread_threshold);
  threads = max(1, threads);
  return threads;
}

void set_alpha(double alpha)
{
  alpha_ = alpha;
}

double get_alpha()
{
  return alpha_;
}

double get_alpha(maxint_t x, int64_t y)
{
  // y = x13 * alpha, thus alpha = y / x13
  double x13 = (double) iroot<3>(x);
  return (double) y / x13;
}

/// Get the Lagarias-Miller-Odlyzko alpha tuning factor.
/// alpha = a log(x)^2 + b log(x) + c
/// a, b and c have been determined empirically.
/// @see doc/alpha-factor-tuning.pdf
///
double get_alpha_lmo(maxint_t x)
{
  double alpha = get_alpha();

  // use default alpha if no command-line alpha provided
  if (alpha < 1)
  {
    double a = 0.00156512;
    double b = -0.0261411;
    double c = 0.990948;
    double logx = log((double) x);

    alpha = a * pow(logx, 2) + b * logx + c;
  }

  return in_between(1, alpha, iroot<6>(x));
}

/// Get the Deleglise-Rivat alpha tuning factor.
/// alpha = a log(x)^3 + b log(x)^2 + c log(x) + d
/// a, b, c and d have been determined empirically.
/// @see doc/alpha-tuning-factor.pdf
///
double get_alpha_deleglise_rivat(maxint_t x)
{
  double alpha = get_alpha();
  double x2 = (double) x;

  // use default alpha if no command-line alpha provided
  if (alpha < 1)
  {
    double a = 0.000356618;
    double b = 0.00263762;
    double c = -0.125227;
    double d = 1.39952;
    double logx = log(x2);

    alpha = a * pow(logx, 3) + b * pow(logx, 2) + c * logx + d;
  }

  return in_between(1, alpha, iroot<6>(x));
}

void set_num_threads(int threads)
{
#ifdef _OPENMP
  threads_ = in_between(1, threads, omp_get_max_threads());
#else
  unused_param(threads);
#endif
}

int get_num_threads()
{
#ifdef _OPENMP
  if (threads_)
    return threads_;
  else
    return max(1, omp_get_max_threads());
#else
  return 1;
#endif
}

void set_status_precision(int precision)
{
  status_precision_ = in_between(0, precision, 5);
}

int get_status_precision(maxint_t x)
{
  // use default precision when no command-line precision provided
  if (status_precision_ < 0)
  {
    if ((double) x >= 1e23)
      return 2;
    if ((double) x >= 1e21)
      return 1;
  }

  return max(status_precision_, 0);
}

maxint_t to_maxint(const string& expr)
{
  maxint_t n = calculator::eval<maxint_t>(expr);
  return n;
}

string primecount_version()
{
  return PRIMECOUNT_VERSION;
}

} // namespace

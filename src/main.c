#include <stdio.h>
#include <math.h>
#include <string.h>


/* degree to index translation routines
 */

inline static unsigned long to_index
(unsigned long n, unsigned long polynom_degree)
{
  return polynom_degree - n;
}

inline static unsigned long to_degree
(unsigned long i, unsigned long polynom_degree)
{
  return polynom_degree - i;
}


/* xkaapi adaptive horner
 */

#if CONFIG_USE_XKAAPI

#include "kaapi.h"

/* work routines
 */

typedef struct master_work
{
  /* workqueue [i, j[ */
  kaapi_workqueue_t wq;

  /* polynom */
  double x;
  const double* a;
  unsigned long n;

  /* result */
  double res;

} master_work_t;

typedef struct thief_work
{
  /* polynom */
  double x;
  const double* a;
  unsigned long n;

  /* [i, j[ to process */
  unsigned long i, j;

  /* computed result */
  double res;

  /* needed to avoid reducing twice */
  unsigned long is_reduced;

} thief_work_t;


/* fwd decl
 */

static void thief_entrypoint(void*, kaapi_thread_t*, kaapi_stealcontext_t*);


/* reduction
 */

static void common_reducer(master_work_t* vw, thief_work_t* tw)
{
  printf("reducer\n");

#if 0 /* todo */
  kaapi_workqueue_index_t pos = kaapi_workqueue_range_begin(&vw->wq);
  for (; pos < beg; ++pos)
    vw->oarray[pos] += vw->prefix;
  vw->prefix += tw->prefix;
#endif

  /* continue the thief work */
  kaapi_workqueue_set(&vw->wq, tw->i, tw->j);
}

static int thief_reducer
(kaapi_taskadaptive_result_t* ktr, void* varg, void* targ)
{
  /* called from the thief upon victim preemption request */

  common_reducer(varg, (thief_work_t*)ktr->data);

  /* inform the victim we did the reduction */
  ((thief_work_t*)ktr->data)->is_reduced = 1;

  return 0;
}

static int victim_reducer
(kaapi_stealcontext_t* sc, void* targ, void* tdata, size_t tsize, void* varg)
{
  /* called from the victim to reduce a thief result */

  if (((thief_work_t*)tdata)->is_reduced == 0)
  {
    /* not already reduced */
    common_reducer(varg, tdata);
  }

  return 0;
}


/* parallel work splitter
 */

static int splitter
(kaapi_stealcontext_t* sc, int nreq, kaapi_request_t* req, void* args)
{
  /* victim work */
  master_work_t* const vw = (master_work_t*)args;

  /* stolen range */
  kaapi_workqueue_index_t i, j;
  kaapi_workqueue_index_t range_size;

  /* reply count */
  int nrep = 0;

  /* size per request */
  kaapi_workqueue_index_t unit_size;

 redo_steal:
  /* do not steal if range size <= PAR_GRAIN */
#define CONFIG_PAR_GRAIN 1
  range_size = kaapi_workqueue_size(&vw->wq);
  if (range_size <= CONFIG_PAR_GRAIN)
    return 0;

  /* how much per req */
  unit_size = range_size / (nreq + 1);
  if (unit_size == 0)
  {
    nreq = (range_size / CONFIG_PAR_GRAIN) - 1;
    unit_size = CONFIG_PAR_GRAIN;
  }

  /* perform the actual steal. if the range
     changed size in between, redo the steal
   */
  if (kaapi_workqueue_steal(&vw->wq, &i, &j, nreq * unit_size))
    goto redo_steal;

  for (; nreq; --nreq, ++req, ++nrep, j -= unit_size)
  {
    /* for reduction, a result is needed. take care of initializing it */
    kaapi_taskadaptive_result_t* const ktr =
      kaapi_allocate_thief_result(req, sizeof(thief_work_t), NULL);

    thief_work_t* const tw = kaapi_reply_init_adaptive_task
      (sc, req, (kaapi_task_body_t)thief_entrypoint, sizeof(thief_work_t), ktr);

    /* initialize the thief work */
    tw->x = vw->x;
    tw->a = vw->a;
    tw->n = vw->n;
    tw->i = j - unit_size;
    tw->j = j;
    tw->res = 0.;
    tw->is_reduced = 0;

    /* initialize ktr task may be preempted before entrypoint */
    memcpy(ktr->data, tw, sizeof(thief_work_t));

    /* reply head, preempt head */
    kaapi_reply_pushhead_adaptive_task(sc, req);
  }

  return nrep;
}


/* extract sequential work
 */

static inline int extract_seq
(master_work_t* w, unsigned long* i, unsigned long* j)
{
  /* sequential size to extract */
  static const unsigned long seq_size = 1;
  const int err = kaapi_workqueue_pop
    (&w->wq, (long*)i, (long*)j, seq_size);
  return err ? -1 : 0;
}

/* thief task entrypoint
 */

static void thief_entrypoint
(void* args, kaapi_thread_t* thread, kaapi_stealcontext_t* sc)
{
  /* input work */
  thief_work_t* const work = (thief_work_t*)args;

  /* resulting work */
  thief_work_t* const res_work = kaapi_adaptive_result_data(sc);

  while (work->i != work->j)
  {
    /* todo_horner */

    /* update prior reducing */
    ++work->i;

    const unsigned long is_preempted = kaapi_preemptpoint
      (sc, thief_reducer, NULL, (void*)work, sizeof(thief_work_t), NULL);
    if (is_preempted) return ;
  }

  /* we are finished, update results. */
  memcpy(res_work, work, sizeof(thief_work_t));
}


/* parallel horner
 */

static double horner_seq_hilo
(double, const double*, unsigned long, double, unsigned long, unsigned long);

static double horner_par
(double x, const double* a, unsigned long n)
{
  /* stealcontext flags */
  static const unsigned long sc_flags =
    KAAPI_SC_CONCURRENT | KAAPI_SC_PREEMPTION;

  /* self thread, task */
  kaapi_thread_t* const thread = kaapi_self_thread();
  kaapi_taskadaptive_result_t* ktr;
  kaapi_stealcontext_t* sc;

  /* sequential indices */
  unsigned long i, j;

  master_work_t work;

  /* initialize horner work */
  work.x = x;
  work.a = a;
  work.n = n;
  work.res = 0.;
  kaapi_workqueue_init(&work.wq, 0, n + 1);

  /* enter adaptive section */
  sc = kaapi_task_begin_adaptive(thread, sc_flags, splitter, &work);

 continue_work:
  while (extract_seq(&work, &i, &j) != -1)
  {
    /* index to degrees */
    const unsigned long hi = to_degree(i, n);
    const unsigned long lo = to_degree(j, n);
    printf("%lu - %lu\n", hi, lo);
    work.res = horner_seq_hilo(x, a, n, work.res, hi, lo);
  }

  /* preempt and reduce thieves */
  if ((ktr = kaapi_get_thief_head(sc)) != NULL)
  {
    kaapi_preempt_thief(sc, ktr, (void*)&work, victim_reducer, (void*)&work);
    goto continue_work;
  }

  /* wait for thieves */
  kaapi_task_end_adaptive(sc);

  return work.res;
}

#endif /* CONFIG_USE_XKAAPI */


/* sequential horner
 */

static double horner_seq_hilo
(
 double x, const double* a, unsigned long n,
 double res, unsigned long hi, unsigned long lo
)
{
  /* the degree in [hi, lo[ being evaluated */
  unsigned long i;

  for (i = hi + 1; i > lo; --i)
    res = res * x + a[to_index(i - 1, n)];
  return res;
}

static inline double horner_seq
(double x, const double* a, unsigned long n)
{
  return horner_seq_hilo(x, a, n, 0., n, 0);
}


/* sequential naive implementation
 */

static double naive_seq
(double x, const double* a, unsigned long n)
{
  double res;
  unsigned long i;
  for (res = 0., i = 0; i <= n; ++i)
    res += pow(x, (double)i) * a[to_index(i, n)];
  return res;
}


/* main
 */

int main(int ac, char** av)
{
  /* 4x^3 + 3x^2 + 2x + 1 */
  static double a[] = { 4., 3., 2., 1. };
  static const unsigned long n = sizeof(a) / sizeof(double) - 1;
  static const double x = 2.;

#if CONFIG_USE_XKAAPI
  kaapi_init(&ac, &av);
#endif

  printf("%lf %lf %lf\n",
	 naive_seq(x, a, n),
	 horner_seq(x, a, n),
	 horner_par(x, a, n));

#if CONFIG_USE_XKAAPI
  kaapi_finalize();
#endif

  return 0;
}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <ve_offload.h>
#include "veo_time.h"

struct veo_proc_handle *proc;
struct veo_thr_ctxt *ctx;
uint64_t sym;
int nloop = 1000;

void *child(void *arg)
{
  int err = 0;
  struct veo_args *argp = veo_args_alloc();
  long ts, te;
  int mloop = 10*nloop/9;
  uint64_t reqs[mloop], res[mloop];

  //----------------------
  printf("Test 2: submit one call and wait for its result, %d times\n", nloop);
  ts = get_time_us();
  err = 0;
  for (int i=0; i<mloop; i++) {
    reqs[i] = veo_call_async(ctx, sym, argp);
    err += veo_call_wait_result(ctx, reqs[i], &res[i]);
  }
  te = get_time_us();
  printf("%d x (1 async call + 1 wait) took %fs, %fus/call\n\n",
         mloop, (double)(te-ts)/1.e6, (double)(te-ts)/mloop);
  if (err != 0)
    printf("cummulated err=%d !? Something's wrong.\n", err);

  veo_args_free(argp);

  return NULL;
}

int main(int argc, char *argv[])
{
  int err = 0;

  if (argc == 1) {
    printf("Usage:\n\t%s <nloop>\n", argv[0]);
    printf("Running for nloop=%d calls.\n", nloop);
  } else
    nloop = atoi(argv[1]);

  proc = veo_proc_create(-1);
  printf("proc = %p\n", (void *)proc);
  if (proc == NULL)
    return -1;
  uint64_t libh = veo_load_library(proc, "./libvehello.so");
  if (libh == 0)
    return -1;
  sym = veo_get_sym(proc, libh, "empty");
  if (sym == 0)
    return -1;
  ctx = veo_context_open(proc);

  //////
  pthread_t th;
  pthread_create (&th, NULL, child, NULL);
  //////
  
  long ts, te;
  struct veo_args *argp = veo_args_alloc();
  uint64_t reqs[nloop], res[nloop];

  //----------------------
  printf("Test 1: submit %d calls, then wait for %d results\n", nloop, nloop);
  ts = get_time_us();
  err = 0;
  for (int i=0; i<nloop; i++) {
    reqs[i] = veo_call_async(ctx, sym, argp);
  }
  for (int i=0; i<nloop; i++) {
    err += veo_call_wait_result(ctx, reqs[i], &res[i]);
  }
        
  te = get_time_us();
  printf("%d async calls + waits took %fs, %fus/call\n\n",
         nloop, (double)(te-ts)/1.e6, (double)(te-ts)/nloop);
  if (err != 0)
    printf("cummulated err=%d !? Something's wrong.\n", err);

  void *child_result;
  pthread_join (th, (void **)&child_result);

  err = veo_context_close(ctx);
  err = veo_proc_destroy(proc);
  return 0;
}

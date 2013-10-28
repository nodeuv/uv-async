#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include "async.h"

#define ENV_A 0x1
#define ENV_B 0x2
#define ENV_C 0x3

static uv_loop_t *loop;

static int jobs[4] = { 0, 0, 0, 0, };
static async_env_t *envs[4] = { };

static int loop_count = 0;
static int env_count = 0;


static void
idle (uv_idle_t *handle, int rc);

static void
on_prep (uv_prepare_t *handle, int rc);

static void
detail_job (char *name, async_work_data_t *data);

static void
job1 (async_work_data_t *data);

static void
job2 (async_work_data_t *data);

static void
job3 (async_work_data_t *data);

static void
job4 (async_work_data_t *data);

static void
spawn_job (async_work_data_t *data);

static void
pass_test (async_work_data_t *data);

int
main (void) {
  loop = uv_default_loop();
  uv_idle_t idler;
  uv_prepare_t prep;

  uv_idle_init(uv_default_loop(), &idler);
  uv_idle_start(&idler, idle);

  uv_prepare_init(uv_default_loop(), &prep);
  uv_prepare_start(&prep, on_prep);

  assert(loop);

  async(env_a, loop) {
    env_a->flags |= ENV_A;

    assert(env_a);
    assert(loop);
    assert(NULL == env_a->handle);
    assert(env_a->loop);
    assert(env_a->flags);

    envs[env_count++] = env_a;

    printf("queue job1 in env (a)\n");
    queue(env_a, job1);

    printf("wait 50ms for job2 in env (a)\n");
    wait(env_a, 50, job2);

    printf("wait 20ms for job3 in env (a)\n");
    wait(env_a, 20, job3);

    printf("queue job4 in env (a)\n");
    queue(env_a, job4);
  }

  async(env_b, loop) {
    env_b->flags |= ENV_B;

    assert(env_b);
    assert(loop);
    assert(NULL == env_b->handle);
    assert(env_b->loop);
    assert(env_b->flags);

    envs[env_count++] = env_b;

    printf("queue job2 in env (b)\n");
    queue(env_b, job2);

    printf("queue job1 in env (b)\n");
    queue(env_b, job1);
  }

  async(env_c, loop) {
    env_c->flags |= ENV_C;
    uv_pipe_t in, out;

    assert(env_c);
    assert(loop);
    assert(NULL == env_c->handle);
    assert(env_c->loop);
    assert(env_c->flags);

    envs[env_count++] = env_c;

    char *cmd[] = { "mkdir", "-p", "./tmp/test/dir" };
    aspawn(env_c, cmd, spawn_job);
  }

  async(env_d, loop) {
    assert(env_c);
    assert(loop);
    assert(NULL == env_c->handle);
    assert(env_c->loop);
    assert(env_c->flags);

    envs[env_count++] = env_d;

    int ms = 1000;

    printf("waiting %d ms... to check state\n", ms);
    wait(env_d, ms, pass_test);
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}

static void
detail_job (char *name, async_work_data_t *data) {
  printf("- '%s' called", name);

  if (ENV_A == (ENV_A & data->env->flags)) {
    printf(" in env (a)");
  } else if (ENV_B == (ENV_B & data->env->flags)) {
    printf(" in env (b)");
  } else if (ENV_C == (ENV_C & data->env->flags)) {
    printf(" in env (c)");
  }

  printf("\n");
}

static void
job1 (async_work_data_t *data) {
  jobs[0] = 1;
  detail_job("job1", data);
}

static void
job2 (async_work_data_t *data) {
  jobs[1] = 1;
  detail_job("job2", data);
}

static void
job3 (async_work_data_t *data) {
  jobs[2] = 1;
  detail_job("job3", data);
}

static void
job4 (async_work_data_t *data) {
  jobs[3] = 1;
  detail_job("job4", data);
}

static void
spawn_job (async_work_data_t *data) {
  detail_job("spawn", data);
  struct stat s;
  assert(0 == stat("./tmp", &s));
  assert(0 == stat("./tmp/test", &s));
  assert(0 == stat("./tmp/test/dir", &s));
}

static void
idle (uv_idle_t *handle, int rc) {

}

static void
on_prep (uv_prepare_t *handle, int rc) {

}

static void
pass_test (async_work_data_t *data) {
  int i = 0;
  printf("testing state...\n");
  assert(1 == jobs[0]);
  assert(1 == jobs[1]);
  assert(1 == jobs[2]);
  assert(1 == jobs[3]);
//  uv_stop(loop);
}

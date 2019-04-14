#ifndef INC_ENV_H
#define INC_ENV_H

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t envid_t;

#define LOG2NENV 10
#define NENV (1 << LOG2NENV)
#define ENVX(envid) ((envid) & (NENV-1))

// Values of env_status in struct Env
enum {
  ENV_FREE = 0,
  ENV_DYING,
  ENV_RUNNABLE,
  ENV_RUNNING,
  ENV_NOT_RUNNABLE
};

// Special environmemt type
enum EnvType {
  ENV_TYPE_USER = 0,
};

struct Env {
  struct Trapframe env_tf; // Saved registers
  struct Env *env_link; // Next free Env
  envid_t env_id; // Unique environment identifier
  envid_t env_parent_id; // env_id of the parent
  enum EnvType env_type; // Indicates special system environment
  unsigned env_status; // Status of the environment
  uint32_t env_runs; // Number of times environment has run
  pde_t *env_pgdir; // Kernel virtual address of page directory
};

#endif // INC_ENV_H

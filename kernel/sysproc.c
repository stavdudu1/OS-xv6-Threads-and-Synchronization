#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
      if(mykthread()->killed == 1){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_kthread_create(void)
{
  uint64 f, s;
  int sz;
  argaddr(0, &f);
  argaddr(1, &s);
  argint(2, &sz);

  return kthread_create((void *)f, s, sz);
}

uint64
sys_kthread_id(void)
{
  return kthread_id();
}

uint64
sys_kthread_kill(void)
{
  int ktid;
  argint(0, &ktid);

  return kthread_kill(ktid);
}

uint64
sys_kthread_exit(void)
{
  int st;
  argint(0, &st);
  kthread_exit(st);
  return 0;
}

uint64
sys_kthread_join(void)
{
  int ktid;
  uint64 st;
  argint(0, &ktid);
  argaddr(1, &st);

  return kthread_join(ktid,st);
}

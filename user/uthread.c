#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

// 定义上下文结构，保存 callee-save 寄存器
struct context {
  uint64 ra;
  uint64 sp;

  // callee-save 寄存器
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct     context context;   /* 保存的寄存器状态 */
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint64, uint64);
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  // 找到下一个可以运行的线程
  for(int i = 0; i < MAX_THREAD; i++){
    // 超出数组长度就重新从数组首地址开始
    if(t >= all_thread + MAX_THREAD)
      t = all_thread;
    if(t->state == RUNNABLE){
      next_thread = t;
      break;
    }
    t = t + 1;
  }

  if(next_thread == 0){
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }

  if(current_thread != next_thread){         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * 调用 thread_switch 进行实际的线程切换
     * 第一个参数是当前线程的 context 地址
     * 第二个参数是要切换到的线程的 context 地址
     */
    thread_switch((uint64)&t->context, (uint64)&current_thread->context);
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)())
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }

  // 检查是否找到空闲槽位
  if(t >= all_thread + MAX_THREAD){
    printf("thread_create: no free thread slots\n");
    return;  
  }

  t->state = RUNNABLE;
  // YOUR CODE HERE
  memset((void *)&t->stack, 0, STACK_SIZE);
  memset((void *)&t->context, 0, sizeof(struct context));
  // 设置栈指针到栈顶
  t->context.sp = (uint64)t->stack + STACK_SIZE;
  // 设置返回地址为要执行的函数
  // 当 thread_switch 返回时，会跳转到这个函数执行
  t->context.ra = (uint64)func;

  // 其他寄存器可以初始化为 0
  t->context.s0 = 0;
  t->context.s1 = 0;
  t->context.s2 = 0;
  t->context.s3 = 0;
  t->context.s4 = 0;
  t->context.s5 = 0;
  t->context.s6 = 0;
  t->context.s7 = 0;
  t->context.s8 = 0;
  t->context.s9 = 0;
  t->context.s10 = 0;
  t->context.s11 = 0;
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  thread_schedule();
  exit(0);
}

#ifndef KERN_MONITOR_H
#define KERN_MONITOR_H

struct Trapframe;

void monitor(struct Trapframe *tf);

int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);

#endif // KERN_MONITOR_H

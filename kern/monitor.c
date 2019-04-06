#include <kern/monitor.h>
#include <inc/stdio.h>
#include <inc/string.h>

struct Command {
  const char *name;
  const char *desc;
  int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
  {"help", "Display this list of commands", mon_help}
};

/**** Implementation of basic kernel monitor commands ****/

int mon_help(int argc, char **argv, struct Trapframe *tf) {
  for (int i=0; i<ARRAY_SIZE(commands); i++) {
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  }
  return 0;
}

/**** Kernel monitor command interpreter ****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int runcmd(char *buf, struct Trapframe *tf) {
  // Parse the command buffer into whitespace spearated arguments
  int argc = 0;
  char *argv[MAXARGS];
  argv[argc] = 0;
  while(1) {
    // Gobble whitespaces
    while(*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
    if (*buf == 0) break;
    
    // Save and scan past next arg
    if (argc == MAXARGS-1) {
      cprintf("Too many arguments (max %d)\n", MAXARGS);
      return 0;
    }
    argv[argc++] = buf;
    while(*buf && !strchr(WHITESPACE, *buf)) buf++;
  }
  argv[argc] = 0;
  
  // Lookup and invoke the command
  if (argc == 0) return 0;
  for (int i=0; i < ARRAY_SIZE(commands); i++) {
    if (strcmp(argv[0], commands[i].name) == 0) {
      return commands[i].func(argc, argv, tf);
    }
  }
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void monitor(struct Trapframe *tf) {
  char *buf;
  cprintf("Welcome to the JOS kernel monitor\n");
  cprintf("Type 'help' for a list of commands\n");
  while(1) {
    buf = readline("K> ");
    if (buf != NULL) {
      if (runcmd(buf, tf) < 0) break;
    }
  }
}
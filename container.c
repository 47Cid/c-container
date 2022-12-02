#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "cgroups.h"

static char child_stack[1024 * 1024];
static char* hostname = "container";

void set_env(){
  clearenv();
  setenv("TERM", "xterm-256color",0);
  setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin",0);
  sethostname(hostname,sizeof(hostname)+1); 
}

void set_root(const char* folder){
  chroot(folder);
  chdir("/");
}

int child(void *args){
  limitProcessCreation();
  printf("child pid: %d\n", getpid());
  unshare(CLONE_NEWNS);
  set_env();
  set_root("./root");
  mount("proc", "/proc", "proc", 0, 0); 

  system("ip link set veth1 up");
  char ip_addr_add[4096];
  snprintf(ip_addr_add, sizeof(ip_addr_add), "ip addr add 172.16.0.101/24 dev veth1");
  system(ip_addr_add);
  system("route add default gw 172.16.0.100 veth1");

  char **argv = (char **)args;
  execvp(argv[0], argv);
  return 0;
}

int main(int argc, char **argv){
  printf("parent pid: %d\n", getpid());

  system("ip link add veth0 type veth peer name veth1");
  system("ip link set veth0 up");
  system("brctl addif br0 veth0");

  int flags = CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET;
  int pid = clone(child, child_stack + sizeof(child_stack), flags | SIGCHLD, argv + 1);

  if (pid < 0) {
    fprintf(stderr, "could not clone process: %d\n", errno);
    return 1;
  }

  char ip_link_set[4096];
  snprintf(ip_link_set, sizeof(ip_link_set) - 1, "ip link set veth1 netns %d", pid);
  system(ip_link_set);

  waitpid(pid, NULL, 0);
  return 0;
}

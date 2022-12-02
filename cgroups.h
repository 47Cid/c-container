#define CGROUP_FOLDER "/sys/fs/cgroup/pids/container/"
#define concat(a,b) (a"" b)
#define MAX_PID 5

void write_rule(const char* path, const char* value) {
  int fp = open(path, O_WRONLY | O_APPEND );
  write(fp, value, strlen(value));
  close(fp);
}

void limitProcessCreation() {
  mkdir( CGROUP_FOLDER, S_IRUSR | S_IWUSR);  
  const char* pid  = getpid();

  write_rule(strcat(CGROUP_FOLDER, "cgroup.procs"), pid);
  write_rule(strcat(CGROUP_FOLDER, "notify_on_release"), "1");
  write_rule(strcat(CGROUP_FOLDER, "pids.max"), MAX_PID);

}

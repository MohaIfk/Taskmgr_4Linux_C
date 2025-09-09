#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>

const char *get_state_name(char state){
  char *state_str;

if (state == 'R')
    state_str = "Running";
else if (state == 'S')
    state_str = "Sleeping";
else if (state == 'D')
    state_str = "Uninterruptible Sleep";
else if (state == 'T')
    state_str = "Stopped";
else if (state == 't')
    state_str = "Tracing Stop";
else if (state == 'Z')
    state_str = "Zombie";
else if (state == 'X')
    state_str = "Dead";
else if (state == 'I')
    state_str = "Idle";
else
    state_str = "Unknown";

  return state_str;
}

void print_pid_info(const char *pid) {
    char path[256];
    FILE *fp;
    int pid_num;
    char comm[256], state;
    char buffer[4096];

    snprintf(path, sizeof(path), "/proc/%s/stat", pid);

    fp = fopen(path, "r");
    if (!fp) return;

    if (fgets(buffer, sizeof(buffer), fp)) {
    char *open = strchr(buffer, '(');
    char *close = NULL;

    if (!open) { fclose(fp); return; }  // check open

    // find the last ')' that is followed by a space
    for (char *p = open; *p != '\0'; p++) {
        if (*p == ')' && *(p + 1) == ' ')
            close = p;
    }

    if (!close) { fclose(fp); return; } // check close

    *close = '\0';
    sscanf(buffer, "%d", &pid_num);
    strncpy(comm, open + 1, sizeof(comm));
    comm[sizeof(comm) - 1] = '\0';
    sscanf(close + 2, "%c", &state);
    printf("PID: %-8d | Name: %-40s | State: %-20s\n", pid_num, comm, get_state_name(state));
}

    fclose(fp);
}

int main(){
  DIR *d;
  struct dirent *dir;
  regex_t regex;

  if (regcomp(&regex, "^[0-9]+$", REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
        return 1;
  }

  d=opendir("/proc");

  if (d==NULL) {
    printf("The Proc Folder Is Unreachable\n");
    return 1;
  }
while (1) {
    system("clear");
     while ((dir=readdir(d))!=NULL) {
    if (dir->d_type==DT_DIR)
      if(regexec(&regex,dir->d_name,0,NULL,0)==0)
        print_pid_info(dir->d_name);
    
  }
  sleep(1);
}
 
  
  closedir(d);
  regfree(&regex);
  return 0;
}

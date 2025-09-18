#include <stdio.h> #include <stdlib.h> #include <dirent.h> #include <string.h> #include <unistd.h> #include <ctype.h> #include <errno.h> #include <sys/types.h>

static const char *get_state_name(char state) { switch (state) { case 'R': return "Running"; case 'S': return "Sleeping"; case 'D': return "Uninterruptible Sleep"; case 'T': return "Stopped"; case 't': return "Tracing Stop"; case 'Z': return "Zombie"; case 'X': return "Dead"; case 'I': return "Idle"; default:  return "Unknown"; } }

/* fast check whether a string is a positive integer (PID) */ static int is_numeric_str(const char *s) { if (!s || !*s) return 0; for (const char *p = s; *p; ++p) if (!isdigit((unsigned char)*p)) return 0; return 1; }

/* Print PID info (PID, comm, state) reading /proc/<pid>/stat safely using getline */ static void print_pid_info(const char *pid_str) { char path[64]; FILE *fp = NULL; char *line = NULL; size_t linecap = 0; ssize_t linelen;

if (snprintf(path, sizeof(path), "/proc/%s/stat", pid_str) >= (int)sizeof(path))
    return; // should not happen for reasonable PIDs

fp = fopen(path, "r");
if (!fp) return; // process may have exited

linelen = getline(&line, &linecap, fp);
if (linelen <= 0) {
    free(line);
    fclose(fp);
    return;
}

/* Find the command between the first '(' and the last ')' */
char *lparen = strchr(line, '(');
char *rparen = strrchr(line, ')');
if (!lparen || !rparen || rparen < lparen) {
    free(line);
    fclose(fp);
    return;
}

/* Extract PID: it's the integer at the beginning of the line (before '(') */
int pid = 0;
{
    char tmp = *lparen;         /* temporarily terminate string to parse pid safely */
    *lparen = '\0';
    pid = (int)strtol(line, NULL, 10);
    *lparen = tmp;
}

/* Extract command name safely (handle arbitrary length inside parentheses) */
size_t comm_len = (size_t)(rparen - lparen - 1);
char *comm = (char *)malloc(comm_len + 1);
if (!comm) {
    free(line);
    fclose(fp);
    return;
}
memcpy(comm, lparen + 1, comm_len);
comm[comm_len] = '\0';

/* After ')' there are space-separated fields; the next non-space character is the state */
char *p = rparen + 1;
while (*p && isspace((unsigned char)*p)) ++p;
char state = (p && *p) ? *p : '?';

printf("PID: %-8d | Name: %-40s | State: %-20s\n", pid, comm, get_state_name(state));

free(comm);
free(line);
fclose(fp);

}

int main(void) { struct dirent **namelist = NULL; int n = 0;

/* Make stdout line-buffered so output appears promptly when printing to a terminal */
setvbuf(stdout, NULL, _IOLBF, 0);

for (;;) {
    /* Clear screen using ANSI escape sequences (faster and safer than system("clear")) */
    printf("\033[H\033[J");

    /* Read /proc entries in one shot; scandir is portable and returns sorted names */
    n = scandir("/proc", &namelist, NULL, alphasort);
    if (n < 0) {
        fprintf(stderr, "scandir(/proc) failed: %s\n", strerror(errno));
        sleep(1);
        continue;
    }

    for (int i = 0; i < n; ++i) {
        const char *name = namelist[i]->d_name;
        if (is_numeric_str(name)) {
            print_pid_info(name);
        }
        free(namelist[i]);
    }
    free(namelist);
    namelist = NULL;

    /* Sleep a second between updates */
    sleep(1);
}

return 0;

}
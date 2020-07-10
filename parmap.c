/*
 * parmap.c - Parallelized execution of a shell command
 *
 * Copyright (c) 2020 Joshua Krusell
 * License: MIT
 */

#define _DEFAULT_SOURCE

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "(test-build)"
#endif

#define USAGE "Usage: %s [OPTION]... <variable> <command>"

extern char **environ;
static const char *progname;

/* clang-format off */
static struct option longopts[] = {
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "delimiter", required_argument, NULL, 'd' },
  { "max_jobs", required_argument, NULL, 'm' },
  { NULL, no_argument, NULL, 0 }
};
/* clang-format on */

/* Maximum number of commands to run in parallel */
static long int max_jobs = 0;

/* Global count of currently spawned child processes */
static long int spawned_jobs = 0;

/* Default delimiters for parsing stdin input */
static char *delims = " \n\t";

/* Buffer holding parsed arguments from stdin */
static char *token;

/* FSM states for stdin parser */
enum ParseState { NORMAL, QUOTED, NORMAL_BACKSLASH, QUOTED_BACKSLASH };

void help(void);
void usage(void);
void version(void);
int cleanup(void);
void err_cleanup(int, char *, ...);
bool isdelim(int);
ssize_t parse_stdin(char *, size_t);
int waitall(bool, int *);

void help(void) {
  printf(USAGE "\n\n", progname);
  printf("Invokes a command in parallel with input from stdin\n\n"
         "Options:\n"
         "  -h, --help\t\t Help message\n"
         "  -v, --version\t\t Print version number\n"
         "  -d, --delimiter\t Placeholder text\n"
         "  -m, --max_jobs\t Maximum number of jobs to run in parallel\n");
}

void usage(void) {
  warnx(USAGE, progname);
}

void version(void) {
  printf("%s %s\n", progname, VERSION);
}

int cleanup(void) {
  int child_status = 0;

  free(token);
  return waitall(true, &child_status);
}

void err_cleanup(int errnum, char *fmt, ...) {
  if (!fmt)
    fmt = "Fatal error";

  cleanup();

  fprintf(stderr, "%s: ", progname);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if (errnum != 0)
    fprintf(stderr, ": %s", strerror(errnum));

  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

int waitall(bool reap_all, int *child_status) {
  pid_t pid;
  int wstatus, rv = 0, wflags = (reap_all || spawned_jobs == max_jobs) ? 0 : WNOHANG;
  *child_status = 0;

  // Follow the xargs POSIX spec, if a command returns 255 or is
  // terminated by a signal, stop processing input from stdin and exit
  // after waiting for the remaining spawned children. Unlike xargs
  // though, simply return 1 as the exit status for all errors.
  while ((pid = waitpid(-1, &wstatus, wflags)) > 0) {
    spawned_jobs--;

    if (WIFSIGNALED(wstatus)) {
      warnx("%d: terminated by signal %s", pid, strsignal(WTERMSIG(wstatus)));
      rv = *child_status = -1;
    } else if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) == 255) {
        warnx("%d: exited with status 255", pid);
        rv = *child_status = -1;
      } else if (WEXITSTATUS(wstatus) > 0) {
        *child_status = -1;
      }
    }

    if (!reap_all)
      break;
  }

  // Same as *BSD xargs, exit on error which might potentially leave
  // behind zombie children.
  if (pid == -1 && errno != ECHILD)
    err(EXIT_FAILURE, "waitpid");

  return rv;
}

bool isdelim(int ch) {
  char *p = delims;
  while (*p) {
    if (ch == *p++) {
      return true;
    }
  }

  return false;
}

ssize_t parse_stdin(char *token, size_t bufsize) {
  enum ParseState state = NORMAL;

  size_t len = 0;
  int ch, quotec;
  char *p = token;

  // Similar to GNU xargs, parse stdin using a finite state machine,
  // but permit escaping delimiter chars with single/double quotes.
  while ((ch = getchar()) != EOF) {
    switch (state) {
      case NORMAL_BACKSLASH:
        state = NORMAL;
        break;
      case QUOTED_BACKSLASH:
        state = QUOTED;
        break;
      case QUOTED:
        if (ch == '\\') {
          state = QUOTED_BACKSLASH;
          continue;
        }

        if (ch == quotec) {
          state = NORMAL;
          continue;
        }

        break;
      case NORMAL:
        if (isdelim(ch)) {
          *p = '\0';
          return len;
        }

        if (ch == '\\') {
          state = NORMAL_BACKSLASH;
          continue;
        }

        if (ch == '\'' || ch == '\"') {
          state = QUOTED;
          quotec = ch;
          continue;
        }

        break;
    }

    *p++ = ch;
    if ((len = p - token) == bufsize)
      err_cleanup(0, "Input token exceeds buffer size");
  }

  if (state == QUOTED)
    err_cleanup(0, "Missing closing %s-quote, aborting", (quotec == '\'') ? "single" : "double");

  *p = '\0';
  return len;
}

int main(int argc, char *argv[]) {
  progname = basename(argv[0]);

  int opt;
  char *end;

  while ((opt = getopt_long(argc, argv, "hvnd:m:", longopts, NULL)) != -1) {
    switch (opt) {
      case 'h':
        help();
        exit(EXIT_SUCCESS);
      case 'v':
        version();
        exit(EXIT_SUCCESS);
      case 'd':
        delims = optarg;
        break;
      case 'm':
        errno = 0;
        max_jobs = strtol(optarg, &end, 10);
        if (*end != '\0' || errno == ERANGE)
          errx(EXIT_FAILURE, "Invalid number '%s' for argument '-m'", optarg);
        break;
      default:
        usage();
        exit(EXIT_FAILURE);
    }
  }

  if (!(optind < argc - 1)) {
    usage();
    errx(EXIT_FAILURE, "Missing command argument");
  }

  char *variable = argv[optind], *cmd = argv[optind + 1];

  if (max_jobs < 1)
    max_jobs = sysconf(_SC_NPROCESSORS_ONLN);

  long int arg_max = sysconf(_SC_ARG_MAX);

  // Follow POSIX standard for xargs such that the combined
  // commandline and environment passed to exec* does not exceed
  // ARG_MAX - 2048.
  size_t bufsize = arg_max - 2048 - strlen(variable) - strlen(cmd) - 8;

  char **p = environ;
  while (*p)
    bufsize -= strlen(*p++) + 1;

  ssize_t len;
  if (!(token = malloc(bufsize)))
    err(EXIT_FAILURE, NULL);

  pid_t pid;
  int rv = 0, wait_status = 0, child_status = 0;

  while ((len = parse_stdin(token, bufsize)) > 0) {
    if ((setenv(variable, token, 1)) < 0)
      err_cleanup(errno, "setenv");

    if ((pid = fork()) == -1) {
      err_cleanup(errno, "fork");
    } else if (pid == 0) {
      int fd;

      // Prevent child process from possibly stealing parent stdin
      // input
      if ((fd = open("/dev/null", O_RDONLY)) == -1 || (dup2(fd, STDIN_FILENO)) == -1)
        err_cleanup(errno, "Failed to attach stdin to /dev/null");

      execl("/bin/sh", "sh", "-c", cmd, NULL);
      err(EXIT_FAILURE, "execl");
    }

    spawned_jobs++;
    wait_status = waitall(false, &child_status);
    rv |= child_status;

    if (wait_status < 0)
      break;
  }

  return cleanup() < 0 || rv != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

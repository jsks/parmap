/*
 * pmap.c - Parallelized of shell commands
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
static long int max_jobs = 0,
  spawned_jobs = 0;

static char *delims = " \n\t",
  *token;

enum ParseState {
  NORMAL,
  QUOTED,
  ESCAPED,
};

void help(void);
void usage(void);
void version(void);
void err_cleanup(int);
bool isdelim(int);
ssize_t parse_stdin(char *, size_t);
int waitall(bool);

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

void err_cleanup(int errnum) {
  free(token);
  waitall(true);

  if (errnum != 0) {
    errno = 0;
    char *msg = strerror(errnum);
    if (errno != 0)
      err(EXIT_FAILURE, NULL);

    fprintf(stderr, "%s: %s\n", progname, msg);
  }

  exit(EXIT_FAILURE);
}

int waitall(bool reap_all) {
  pid_t pid;
  int wstatus,
    rv = 0,
    wflags = (reap_all || spawned_jobs == max_jobs) ? 0 : WNOHANG;

  while ((pid = waitpid(-1, &wstatus, wflags)) > 0) {
    spawned_jobs--;

    if (WIFSIGNALED(wstatus)) {
      warnx("%d: terminated by %s", pid, strsignal(WTERMSIG(wstatus)));
      rv = -2;
    } else if (WIFEXITED(wstatus)) {
      if (WEXITSTATUS(wstatus) == 255) {
        warnx("%d: returned with status 255", pid);
        rv = -2;
      } else if (WEXITSTATUS(wstatus) > 0) {
        rv = -1;
      }
    }

    if (!reap_all)
      break;
  }

  if (pid == -1 && errno != ECHILD)
    err(EXIT_FAILURE, NULL);

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
  char *p;

  if (!(p = token))
    return -1;

  while ((ch = getchar()) != EOF) {
    switch (state) {
      case ESCAPED:
        state = NORMAL;
        break;
      case QUOTED:
        if (ch == '\\')
          continue;

        if (ch == quotec)
          state = NORMAL;

        break;
      case NORMAL:
        if (isdelim(ch)) {
          *p = '\0';
          return len;
        }

        if (ch == '\\') {
          state = ESCAPED;
          continue;
        }

        if (ch == '\'' || ch == '\"') {
          state = QUOTED;
          quotec = ch;
        }

        break;
    }

    *p++ = ch;
    if ((len = p - token) == bufsize) {
      warnx("Buffer size");
      return -1;
    }
  }

  if (state == QUOTED) {
    warnx("Missing closing %s-quote, aborting", (quotec == '\'') ? "single" : "double");
    return -1;
  }

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

  if (max_jobs < 1 && (max_jobs = sysconf(_SC_NPROCESSORS_ONLN)) == -1)
    err(EXIT_FAILURE, NULL);

  pid_t pid;
  int rv = 0;

  size_t bufsize = 1024;
  ssize_t len;
  token = malloc(bufsize);

  while ((len = parse_stdin(token, bufsize)) > 0) {
    if ((setenv(variable, token, 1)) < 0)
      err_cleanup(errno);

    if ((pid = fork()) == -1) {
      err_cleanup(errno);
    } else if (pid == 0) {
      int fd;
      if ((fd = open("/dev/null", O_RDONLY)) == -1 || (dup2(fd, STDIN_FILENO)) == -1)
        err(EXIT_FAILURE, NULL);

      execl("/bin/sh", "sh", "-c", cmd, NULL);
      _exit(EXIT_FAILURE);
    }

    spawned_jobs++;
    if ((rv = waitall(false), rv) == -2)
      break;
  }

  if (len == -1)
    err_cleanup(0);

  free(token);
  return (waitall(true) < 0 || rv != 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

---
title: PARMAP(1)
date: July 2020
---

# NAME

parmap - parallelized execution of a shell command

# SYNOPSIS

**parmap** [option]... *variable* *command*

# DESCRIPTION

**parmap** reads arguments delimited by whitespace characters from
standard input and invokes *command* in parallel for each argument.
For each invocation, *variable* is set to the corresponding parsed
argument and made available to *command* as an exported environment
variable.

*command* should consist of a single quoted string to prevent shell
expansions before execution. When invoked by **parmap** it will be
interpreted by **/bin/sh** and hence should conform to valid POSIX
shell syntax.

Delimiters for arguments read from standard input can be escaped using
either a backslash or single/double quotes and can be changed with the
commandline option **-d**.

Processing continues for each argument until one of three conditions:

- End of input
- A *command* invocation exits with a status of 255
- A *command* invocation is terminated by a signal

# OPTIONS

-d, \--delimiter *delim*
: Delimiter(s) for parsing standard input. A delimiter must be a 7-bit
  ascii character and cannot be the null byte. Multiple characters
  will be interpreted as separate delimiters. The default is
  whitespace characters.

-h, \--help
: Print help message and exit.

-m, \--max-jobs *number*
: Maximum number of child processes to run in parallel. If unspecified
  or set to less than or equal to zero, the default is the available
  number of CPU cores as determined by `_SC_NPROCESSORS_ONLN` (see
  **sysconf**(3)).

-v, \--version
: Print version number and exit.

# EXIT STATUS

If `parmap` encounters an internal error or if any of the child
processes return with a non-zero exit status or are terminated by a
signal, then `parmap` will exit with status 1. Otherwise, the exit
status will be 0 on success.

# EXAMPLES

Simple example using default options:

```sh
# Output order may vary
$ echo 1 2 3 4 | parmap x 'echo number: $x'
```

If using **zsh**, spaces in filenames can be escaped with quotes by
using a glob modifier. For example:

```sh
$ touch "example file.txt"
$ print -r *(:q) | parmap f 'echo file: $f'
```

# SEE ALSO

**find**(1), **xargs**(1)

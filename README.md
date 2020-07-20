parmap
---

`parmap` is a commandline utility for executing in parallel a command
for each argument read from standard input.

## Installation

The target platform for `parmap` is x86_64 GNU/Linux; however, the
code is fairly portable and the limited testsuite does pass when
compiled against musl and on NetBSD/amd64.

To compile simply invoke `make`. There are no external dependencies.

``` sh
$ make

# Optional, install in /usr/local
$ sudo make install
```

Building the debug target or invoking the integration tests with `make
test` require GNU Make. If compiling on *BSD substitute `make` for
`gmake`.

## Usage

Basic usage consists of specifying a variable name to contain the
parsed argument from standard input and a quoted shell command that
may reference the specified variable.

For example, using the default delimiters of whitespace characters:

``` sh
# Output order may vary
$ printf "1 2 3 4" | ./parmap x 'echo $(( $x + 1 ))'
2
3
4
5
```

Delimiters can be escaped using single/double quotes and
backslashes. For example:

``` sh
$ printf "'1 2' 3\ 4" | ./parmap x 'echo _$x'
_1 2
_3 4
```

The delimiter can be changed using the commandline option, `-d`. For
example:

``` sh
$ printf "1|2|3|4" | ./parmap -d '|' x 'echo _$x'
_1
_2
_3
_4
```

To limit the maximum number of parallel jobs use the commandline
option, `-m`. For example:

``` sh
# This will synchronously process the inputs from stdin by spawning
# at most 1 child process at a time.
$ printf "1 2 3 4" | ./parmap -m 1 x 'echo _$x'
_1
_2
_3
_4
```

For complete documentation refer to the provided manpage, `parmap.1`.

## License

MIT

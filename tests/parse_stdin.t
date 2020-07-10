Setup:
  $ alias parmap="$TESTDIR/../parmap"

Default delimiter should include blank space:
  $ print {1..4} | parmap -m 1 x 'echo _$x'
  _1
  _2
  _3
  _4

Default delimiter should include tab:
  $ print '1\t2\t3\t4' | parmap -m 1 x 'echo _$x'
  _1
  _2
  _3
  _4

Default delimiter should include newlines:
  $ print '1\n2\n3\n4\n' | parmap -m 1 x 'echo _$x'
  _1
  _2
  _3
  _4

Backslash should escape blank space:
  $ print -r '1\ 2 3 4' | parmap -m 1 x 'echo _$x'
  _1 2
  _3
  _4

Single quotes should escape blank space
  $ print "1 2 '3 4'" | parmap -m 1 x 'echo _$x'
  _1
  _2
  _3 4

Double quotes should escape blank space:
  $ print '"1 2" 3 4' | parmap -m 1 x 'echo _$x'
  _1 2
  _3
  _4

# Note: `echo -e` won't print a newline in an env variable
Single quotes should escape newline:
  $ print "'1\n2' 3 4" | parmap -m 1 x 'printf "_"; printf "%s\n" $x'
  _1
  2
  _3
  _4

Backslash should escape quotes:
  $ print -r "\'1 2\' 3 4" | parmap -m 1 x 'echo _$x'
  _'1
  _2'
  _3
  _4

Backslash needs to be escaped:
  $ print -r '1\\ 2 3 4' | parmap -m 1 x 'echo _$x'
  _1\
  _2
  _3
  _4

Backslash needs to be escaped within quotes:
  $ print -r '"1\\" 2 3 4' | parmap -m 1 x 'echo _$x'
  _1\
  _2
  _3
  _4

Quotes need to be escaped within quotes:
  $ print -r '"1\" 2" 3 4' | parmap -m 1 x 'echo _$x'
  _1" 2
  _3
  _4

Switching delimiter should still work:
  $ print '1|2|3|4' | parmap -d '|' -m 1 x 'echo _$x'
  _1
  _2
  _3
  _4

No delimiter in input should return only a single argument:
  $ print {1..4} | parmap -d '|' -m 1 x 'echo _$x'
  _1 2 3 4

Nulls present in input string should stop processing:
  $ print -N {1..4} | parmap -m 1 x 'echo _$x'
  _1

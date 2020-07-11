Setup:
  $ alias parmap="$TESTDIR/../parmap"

Unknown command should fail:
  $ print 1 2 3 4 | parmap -m 1 x 'sdf $x'
  sh: sdf: *not found (glob)
  sh: sdf: *not found (glob)
  sh: sdf: *not found (glob)
  sh: sdf: *not found (glob)
  [1]

Unmatched single quote should fail:
  $ print "'1 2" | parmap -m 1 x 'echo $x'
  parmap: Missing closing single-quote, aborting
  [1]

Unmatched double quote should fail:
  $ print '1 "2' | parmap -m 1 x 'echo $x'
  1
  parmap: Missing closing double-quote, aborting
  [1]

Invalid argument to `-m` should fail:
  $ print 1 2 | parmap -m 10a x 'echo $x'
  parmap: Invalid number '10a' for argument '-m'
  [1]

Unknown argument should fail:
  $ print 1 2 | parmap -X x 'echo $x'
  *parmap: *X* (glob)
  parmap: Usage: * (glob)
  [1]

Missing command argument should fail:
  $ print 1 2 | parmap x
  parmap: Usage: * (glob)
  parmap: Missing command argument
  [1]

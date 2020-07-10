Setup:
  $ alias parmap="$TESTDIR/../parmap"

Normal operation should return 0:
  $ print {1..4} | parmap x 'echo $x >/dev/null'; echo $?
  0

If a task errors, then the remaining tasks continue, but the final
exit status is non-zero:
  $ print {1..4} | parmap -m 1 x '[ $x == 2 ] && exit 127; echo $x'
  1
  3
  4
  [1]

If command returns 255, wait for spawned jobs and then immediately
quit:
  $ print {1..4} | parmap x -m 1 '[ $x == 1 ] && exit 255; echo $x'
  *: exited with status 255 (glob)
  [1]

If command is terminated by signal, wait for spawned jobs and then
immediately quit:
  $ print {1..4} | parmap -m 1 x '[ $x == 2 ] && kill -INT $$; echo $x'
  1
  *: terminated by signal * (glob)
  [1]

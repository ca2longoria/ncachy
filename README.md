# ncachy
Single-file unix-socket-file server/client key/val store script.  Paired with ccachy, written in C, for considerably faster client-side action.

## ccachy vs ncachy

**ccachy** mimics only the client-side behavior of **ncachy**.

Why does it exist?  Because it can be between 10-15 times faster than ncachy used in its client context on a single-core machine, and if it is to be used in long loops, it's needs to be at least that fast.

Its usage doesn't match **ncachy's** exactly, but it's close enough.

```bash
# Set, via argument
ccachy [@name] [key1=val1 [key2=val2 [key3=val3 ...]]]
# Get, via argument
ccachy [@name] [arg1 [arg2 [arg3 ...]]]

# stdin as an alternative to arguments
cachy [@name] <<EOF
key1=val1
key2=val2
key3=val3
EOF
```
^ Pretty much like that.

## Makefile

Run `make` for standard compilation of **ccachy**.
`make debug` will compile a version with colored debug output.

## Examples

For usage, run `ncachy --help`.  I haven't added usage output to **ccachy**, yet.

```bash
#!/bin/bash

# '-k' defaults to parent pid, so ncachy's socket file will  will be cleaned
# up when the script finishes.
SOK=`ncachy -d -k`

echo socket in use: $SOK
echo

A=1
ccachy a=1
for i in {1..5} | while read line; do
  echo $A
  let A=$A+1
  ccachy a+=1
done
echo
echo A vs a: $A, `ccachy a`

```

When running the above, the output looks like...
```
socket in use: /tmp/jeeze.sock

1
2
3
4
5

A vs a: 1, 6
```

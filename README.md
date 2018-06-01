# ncachy
Single-file unix-socket-file server/client key/val store script.  Paired with ccachy, written in C, for considerably faster client-side action.

## ccachy vs ncachy

**ccachy** mimics only the client-side behavior of **ncachy**.

Why does it exist?  Because it can be between 10-15 times faster than ncachy used in its client context on a single-core machine, and if it is to be used in long loops, it's needs to be at least that fast.

Its usage doesn't match **ncachy's** exactly, but it's close enough.

```
ccachy [@name] [arg1 [arg2 [arg3 ...]]]
<stdin> | cachy [@name]
```
^ Pretty much like that.

## Makefile

Run `make` for standard compilation of **ccachy**.
`make debug` will compile a version with colored debug output.


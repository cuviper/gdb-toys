# gdb-toys

*Experimental clients using the GDB remote protocol.*

These are a set of command line tools that talk to gdbserver to perform
debugging operations.

* **gdbi**: An interactive prompt for sending raw protocol commands to gdbserver
  and reading their replies.  The packets are automatically wrapped with begin
  and end markers, with checksum, and the replies are decoded from escape
  characters and run-length-encoding before printing.

* **gdbtrace**: A simple signal/syscall tracer via gdbserver.  It aims to be
  similar to what you would get from `strace`, though not decoded. (yet?)
  A gdbserver patch is required for syscall support:
  https://sourceware.org/ml/gdb-patches/2013-09/msg00992.html

## License

These programs are provided under GPLv3+ -- see COPYING.

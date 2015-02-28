# gdb-toys

*Experimental clients using the GDB remote protocol.*

These are a set of command line tools that talk to gdbserver to perform
debugging operations.

* **gdbi**: An interactive prompt for sending raw protocol commands to gdbserver
  and reading their replies.  The packets are automatically wrapped with begin
  and end markers, with checksum, and the replies are decoded from escape
  characters and run-length-encoding before printing.

* **gdbsig**: A simple signal tracer via gdbserver.  It aims to be similar to
  what you would get from `strace -e trace= cmd...`, eventually anyway.

## License

These programs are provided under GPLv3+ -- see COPYING.

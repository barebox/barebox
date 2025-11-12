Release v2025.12.0
==================

Shell
-----

* An optional parameter was added to the `-c` option of `dmesg` allowing
  configuration of the number of lines to remain in the log buffer after
  clearing. When no parameter is provided to `-c`, zero is assumed, and no
  lines are retained. Earlier versions always left 10 lines of logs remain in
  the log buffer.

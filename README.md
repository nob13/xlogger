xlogger
=======

xlogger is a minimalstic log file generator, which is somehow similar to logger(1) but doesn't use syslog.

It marks incoming data with the current date and some tag and writes it into a logging file.
If the file exists a new one will be created. If a given file size threshold is reached, a new file will be created.

For who is this?
For all these people who like to do logging via printf or NodeJS' console.log("bla") and who do not have
the time and the need to setup a logging framework.

Requirements
------------
- Linux, GCC

Compilation
-----------
Just enter make, xlogger will be in "xlogger".
If you want a debug variant, enter "make xlogger_dbg".

Usage
-----
Just pipe in your data, e.g.

    myprogram | /path/to/xlogger --out /tmp/logfile.log

Parameters
----------
    --help                   show this help
    --out [FILENAME]         sets output filename
    --tag [TAG]              sets a tag for the output file
    --size [SIZE]            go to next log file after a given size (<=0 means infinite)
    --deluxe                 enables some nice reformatting on newlines

Deluxe reformatting puts newline on a same prefix:
E.g.: 
    $./demo.py | ./xlogger --deluxe
    $more output.log

    10-11-2011 17:55:43  First logging
    10-11-2011 17:55:43  Second logging
    10-11-2011 17:55:44  Third logging with newlines 
                         yeah 
                     
    10-11-2011 17:55:44  Last logging
    ...

Bugs
----
- Note: xlogger does not always detect the end of a log output as the OS may do some buffering
and one write operation of the target program does not necessary lead to one read operation.
  This could be circumvented with support of "\n" separated log output, altough this support would
  break support for multi-line-comments.
- I usually don't do plain C. There may be some String issues.

License
-------
The code is licensed under the terms of the BSD license.

xlogger: xlogger.c
		gcc -DNDEBUG -Wall -std=c99 -O3 xlogger.c -o xlogger
# Debug Variant
xlogger_dbg: xlogger.c
		gcc  -g -Wall -std=c99 xlogger.c -o xlogger_dbg


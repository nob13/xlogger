#!/usr/bin/python -u
import time
print "First logging"
time.sleep (0.1)
print "Second logging"
time.sleep (0.1)
print "Third logging with newlines \nyeah \n"
time.sleep (0.1)
print "Last logging"

for i in range (50):
	print "A long line yeah num. ", i
	time.sleep (0.1);


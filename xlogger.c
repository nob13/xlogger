#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <time.h>
#include <errno.h>
#include <assert.h>

void printHelp (){
    const char * help = 
    "xlogger, a minimalistic log file generator\n"
    "------------------------------------------\n"
    "--help                   show this help\n"
    "--out [FILENAME]         sets output filename\n"
    "--tag [TAG]              sets a tag for the output file\n"
    "--size [SIZE]            go to next log file after a given size (<=0 means infinite)\n"
    "--deluxe                 enables some nice reformatting on newlines\n"
    ;
    printf (help);
}

int eq (const char * a, const char * b) {
    return strcmp (a,b) == 0;
}

// Disable this on debug
#ifndef NDEBUG
#define LOG(X) printf ("Log: %s\n", X);
#define LOGERRNO(X) printf ("Log: %s (%s)\n", X, strerror(errno));
#define LOG2(X,N) printf ("Log: %s %s\n", X, N);
#else
#define LOG(X)
#define LOGERRNO(X)
#define LOG2(X,N)
#endif

// Settings
const char * outputFile = "output.log";
const char * outputTag  = "";
int maxSize = 0;
enum WriteMode {
	WM_DEFAULT = 0,
	WM_DELUXE = 1
} writeMode = WM_DEFAULT;

typedef enum WorkError_t {
    WE_NOERR,  // No Error happened (end of file)           
    WE_EXISTS, // File exists already
    WE_WRITE,  // File write error
    WE_READ,   // Reading error
    WE_PERM,   // File permission err
    WE_SIZE,   // Maximum size reached
    WE_OTHER   // Some other error
} WorkError;

int min (int a, int b) { return a < b ? a : b; }

// Formats current time (not thread safe!)
const char * getTimeString () {
    time_t ct = time (NULL);
    static char buffer [256];
    struct tm * current = localtime(&ct);
    strftime (buffer, 256, "%d-%m-%Y %T", current);
    return buffer;
}

/// Caluclates the length of an encoding, if all newlines of the message are
/// indented to the lenght of the prefix
/// (Note: the last newline gets ignored or added if not exists)
int deluxeFormatLength (int prefixLength, const char * message) {
	int result = prefixLength;
	const char * last = 0;
	for (const char * p = message; *p != 0; p++) {
		last = p;
		const char * n = (p + 1);
		if (*p == '\n' && *n != 0) {
			result += prefixLength + 1;
		} else {
			result += 1;
		}
	}
	// Add an artificial newline to close message
	if (last && *last != '\n')
		result += 1;
	return result;
}

/// Does acutal deluxe format
void deluxeFormat (char * buffer, int bufferSize, const char * prefix, int prefixLength, const char * message) {
	int rest = bufferSize;
	assert (rest >= 0);
	memcpy (buffer, prefix, prefixLength);
	assert (rest >= 0);
	buffer+=prefixLength;
	rest  -=prefixLength;
	const char * last = 0;
	for (const char * p = message; *p != 0; p++) {
		const char * n = p + 1;
		last = p;
		*buffer = *p;
		rest--;
		buffer++;
		assert (rest >= 0);
		if (*p == '\n' && *n != 0) {
			memset (buffer, ' ', prefixLength);
			buffer += prefixLength;
			rest   -= prefixLength;
			assert (rest >= 0);
		}
	}
	// Add aritificial newline to close the message
	if (last && *last != '\n') {
		*buffer = '\n';
		rest--;
	}
	assert (rest == 0);
}

// Writes log message into the file descripter, formats it.
// Returns return value of write().
ssize_t formatAndWrite (int fd, char * message, int messageLength) {
        // Format it
        if (writeMode == WM_DEFAULT){
		    char outputBuffer [8192];
		    outputBuffer[8191] = 0;
		    const char * timeString = getTimeString ();
		    int endsWithNewLine = messageLength > 1 && message[messageLength - 1] == '\n';
		    if (endsWithNewLine) {
		        snprintf (outputBuffer, 8191, "%s %s %s", timeString, outputTag, message);
		    } else {
		        snprintf (outputBuffer, 8191, "%s %s %s\n", timeString, outputTag, message);
		    }
        	// Write it
        	size_t usedLength = strlen (outputBuffer);
        	return write (fd, outputBuffer, usedLength);
        } else if (writeMode == WM_DELUXE) {
        	const char * timeString = getTimeString();
        	char prefixBuffer [256];
        	prefixBuffer[255] = 0;
        	snprintf (prefixBuffer, 255, "%s %s ", timeString, outputTag); 
        	int prefixLength = strlen (prefixBuffer);
        	int needed = deluxeFormatLength (prefixLength, message);
        	char buffer[needed];
        	deluxeFormat (buffer, needed, prefixBuffer, prefixLength, message);
        	return write (fd, buffer, needed);
        } else {
        	assert (!"Unknown write mode");
        	return -1;
        }
}

// Continue working to a a file
WorkError continueLoggingTo (const char * fileOut) {
    LOG2 ("Trying ", fileOut)
    int fd = open (fileOut, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 00640);
    if (fd < 0) {
        if (errno == EEXIST) {
            LOG("File exists")
            return WE_EXISTS;
        }
        if (errno == EACCES) {
            LOG ("Access Error")
            return WE_PERM;
        }
        LOG ("Unknown Error")
        return WE_OTHER;
    }
    int currentSize = 0;
    while (1) {
        char buffer [4096];
        buffer[4095] = 0;
        int toRead = maxSize <= 0 ? 4095 : min (4095, (maxSize - currentSize));
        if (toRead <= 0) {
            LOG ("Size limit reached")
            close (fd);
            return WE_SIZE;
        }
        ssize_t amount = read (0, &buffer, toRead);
        #ifndef NDEBUG
        printf ("Read %ld bytes (toRead=%d)\n", amount, toRead);
        #endif
        if (amount < 0) {
            LOGERRNO ("Read error")
            close (fd);
            return WE_READ;
        }
        if (amount == 0) {
            LOG ("End of file")
            close (fd);
            return WE_NOERR;
        }
        buffer[amount] = 0;
        ssize_t writtenLength = formatAndWrite (fd, buffer, amount);
        
        if (writtenLength < 0) {
            LOGERRNO ("Write Error");
            close (fd);
            return WE_WRITE;
        }
        #ifndef NDEBUG
        printf ("Wrote %ld bytes\n", writtenLength);
        #endif
        currentSize += writtenLength;
    }
    // Shall not come here...
    return WE_NOERR;
}

int main (int argc, char * argv[]){	
    for (int i = 1; i < argc; i++) {
        const char * p = argv[i];
        const char * n = i + 1 < argc ? argv[i + 1] : 0;
        if (eq (p, "--help")) {
            printHelp();
            return 0;
        }
        if (eq (p, "--out") && n){
            outputFile = n;
            i++;
        }
        if (eq (p, "--tag") && n) {
            outputTag = n;
            i++;
        }
        if (eq (p, "--size") && n) {
            maxSize = atoi (n);
            i++;
        }
        if (eq (p, "--deluxe")){
        	writeMode = WM_DELUXE;
        }
    }
#ifndef NDEBUG
    printf ("Output file: %s Output Tag: %s Write Mode %d\n", outputFile, outputTag, writeMode);
#endif
    WorkError e = continueLoggingTo (outputFile);
    if (e == WE_EXISTS || e == WE_SIZE) {
        for (int i = 0; i < 8192; i++) {
            // Try to add something...
            int len = strlen (outputFile) + 5;
            char name [len];
            name[len-1] = 0;
            snprintf (name, len - 1, "%s.%d", outputFile, i);
            e = continueLoggingTo (name);
            if (e == WE_EXISTS || e == WE_SIZE)
              continue;
            if (e == WE_NOERR) {
                return 0;
            }
            fprintf (stderr, "Could not continue working, code = %d!\n", e);
            return 1;
        }
        fprintf (stderr, "Maximum log file number reached!\n");
        return 1;
    }
    if (e == WE_NOERR) {
    	return 0;
    }
    fprintf (stderr, "Could not continue work, code=%d!\n", e);
    return 1;
}


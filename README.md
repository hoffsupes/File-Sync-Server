# File-Sync-Server

*********************************************************************************
*********************************************************************************
*********************************************************************************
Simple File Sync **************************************************************** 
by               ****************************************************************
Gaurav Dass      ****************************************************************
dassg@rpi.edu    ****************************************************************
*********************************************************************************
*********************************************************************************
*********************************************************************************

This is a very simple file sync created for syncing files between the server (temporary directory in there) and the current directory of the client. This was a nice problem. The development was done primarily on Ubuntu 16.10 and testing for compilation was done on Ubuntu 14.04.

USAGE (IMPORTANT):

Compile the program by using the following flags, it is very important to do so:

-lssl -lcrypto -lm

FOR SERVER MODE:
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs server <port no>

eg.
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs server 67129


FOR CLIENT MODE:
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs client <port no>


eg.
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs client 67129


Together:
eg.
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs server 67129
rm fs; gcc -o fs -g file_sync.c -lssl -lcrypto -lm; ./fs client 67129

Use the same port number for both of them.

Please run the server BEFORE the client. 

Operation is of the form: Server Starts up and waits. A client connects, the server generates a .4220_file_list and sends it over to the client. The client receives the list, decodes it to get the list of files in the server and then acts accordingly. 

Client operation is of two forms:

A.) Traversing each server file looking for 

        a.) Server files which have a match with client
            i.)   Either are outdated, overwrite own version of file
            ii.)  Are newer version, overwrite server's version of file
            iii.) Client does not have this file, get from server this file

B.) Traversing each file of its own to determine which files server does not have, sending server these files

C.) Close connection -- Cleanly exit

I've tried to avoid the use of any extensive packets, I've relied on sending what could be interpreted as simple opcodes. The above situations translate to opcodes:

Aai    ->   2
Aaii   ->   4
Aaiii  ->   1
B      ->   3
C      ->   5

An intermediate opcode is 109, which is used to skip files when a perfect match occurs, namely, timestamp and hash are both equal. Opcodes are sent with the filenames, again, the communication model is kept as lightweight as possible. Along with the opcodes, the client also sends the filename and hash to the server.

After the client finishes up, it exits and the server goes back to listening.

The biggest problem would be the testing on this thing, the client was to be tested on the server which itself was to be built on the way. Along with that the file had to be saved across multiple directories to test for the sync. Each step required sureshot and precise implementation. To make sure that the server is in tandem with the client, I had to force synchronization between the server and the client. Synch points are implemented after the completion of each successful transfer. The client also skips over all the metadata, including its own executable and code file which it treats as the same (rejects them).

If you have any problems while testing this code, please contact me at dassg@rpi.edu.

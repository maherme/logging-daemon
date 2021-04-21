# logging-daemon

A program which is taking care of system logs on the Linux System.

## Functionality:

The main actions of this program are:
- Creating (owns) a unix socket /dev/log with a read and writes permissions for anybody.
- Reading logs from the socket and write them to the files provided on the command line.
- Printing logs to STDOUT.
- Detecting duplicate logs.
- Handling SIGINT therefore it prints the most frequent message before exit.
- Accepting -f option, so that it forks itself to the background.
- When using -f it writes /var/run/logging-daemon.pid file with its PID after forking and it will remove /var/run/logging-daemon.pid when it is killed.

## Quick Start:

Once you have cloned the project in your local directory execute:
```
$ cmake -H. -Bbuild
```
This command will configure the build environment and it will create the build folder where the executable files will be placed.

You can complie the code using:
```
$ cmake --build build
```
The executable file, named logging-daemon, will be placed in the build/ directory.

Before running the program you need to do:
```
# systemctl stop rsyslog
# systemctl stop systemd-journald
# unlink /dev/log
```
You can run the program as follow:
```
$ sudo ./logging-daemon -f /tmp/output1.log /tmp/output2.log /tmp/output3.log
```
You can log messages using logger command as follow:
```
$ logger hello
$ logger good bye
$ logger hello
$ logger hello again
$ logger hello
$ logger bye bye
```
You can stop the daemon using kill:
```
# kill -2 <pid>
```

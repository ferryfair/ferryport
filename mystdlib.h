/* 
 * File:   mystdlib.h
 * Author: newmek7
 *
 * Created on 26 March, 2013, 10:25 AM
 */

#ifndef MYSTDLIB_H
#define	MYSTDLIB_H
#include <sys/types.h>
#include <string>
#include <signal.h>

void initTermios(int echo);
void resetTermios(void);
char getch_(int echo);
char getch(void);
char getche(void);
int copyfile(std::string src, std::string dst);
std::string getCurrentDir();
std::string getMachineName();
int rmmydir(std::string dirn);
std::string inputPass();
std::string inputText();
std::string getStdoutFromCommand(std::string cmd);
std::string getTime();
std::string get_command_line(pid_t pid);
int poke(std::string ip);

class spawn {
private:
    int cpstdinp[2];
    int cpstdoutp[2];
    int cpstderrp[2];
    int childExitStatus;

public:
    pid_t cpid;
    int cpstdin;
    int cpstdout;
    int cpstderr;

    spawn();
    spawn(std::string command, bool daemon = false, void (*onStopHandler)() = NULL, bool freeChild = false, bool block = false);
    int getChildExitStatus();
    int pkill(int signal = SIGTERM);
};
#endif	/* MYSTDLIB_H */


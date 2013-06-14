#include "mystdlib.h"
#include "myconverters.h"
#include "debug.h"
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sys/stat.h> 
#include <ftw.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <vector>
#include <wait.h>
#include <malloc.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
static struct termios old, mnew;

/* Initialize new terminal i/o settings */
void initTermios(int echo) {
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    mnew = old; /* make new settings same as old settings */
    mnew.c_lflag &= ~ICANON; /* disable buffered i/o */
    mnew.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
    tcsetattr(0, TCSANOW, &mnew); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
    tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) {
    char ch;
    initTermios(echo);
    ch = getchar();
    resetTermios();
    return ch;
}

/* Read 1 character without echo */
char getch(void) {
    return getch_(0);
}

/* Read 1 character with echo */
char getche(void) {
    return getch_(1);
}

spawn::spawn() {

}

spawn::spawn(std::string command, bool daemon, void (*onStopHandler)(), bool freeChild, bool block) {
    pipe(this->cpstdinp);
    pipe(this->cpstdoutp);
    pipe(this->cpstderrp);
    std::vector<std::string> cmdv = explode(" ", command);
    char * args[cmdv.size() + 1];
    int i = 0;
    int a = 0;
    bool validcmd = true;
    std::string arg;
    for (i = 0; i < cmdv.size(); i++) {
        arg = std::string(cmdv[i]);
        if (cmdv[i][0] == '"') {
            arg=arg.substr(1);
            i++;
            while (i < cmdv.size() && cmdv[i][cmdv[i].length() - 1] != '"') {
                arg += " " + cmdv[i];
                i++;
            }
            if (i < cmdv.size() && cmdv[i][cmdv[i].length() - 1] == '"') {
                arg += " " + cmdv[i];
                arg=arg.substr(0,arg.length()-1);
                i++;
            } else {
                validcmd = false;
                i++;
                break;
            }
        } else if (cmdv[i][0] == '\'') {
            arg=arg.substr(1);
            i++;
            while (i < cmdv.size() && cmdv[i][cmdv[i].length() - 1] != '\'') {
                arg += " " + cmdv[i];
                i++;
            }
            if (i < cmdv.size() && cmdv[i][cmdv[i].length() - 1] == '\'') {
                arg += " " + cmdv[i];
                arg=arg.substr(0,arg.length()-1);
                i++;
            } else {
                validcmd = false;
                i++;
                break;
            }
        } else if (arg[arg.length() - 1] == '\\') {
            arg = arg.substr(0, arg.length() - 1) + " " + cmdv[++i];
        }
        char * buf = (char*) malloc(arg.length() + 1);
        arg.copy(buf, arg.length(), 0);
        buf[arg.length()] = '\0';
        args[a] = buf;
        ++a;
    }
    args[a] = NULL;
    if (validcmd) {
        this->cpid = fork();
        if (this->cpid == 0) {
            int fi;
            if (!freeChild) {
                prctl(PR_SET_PDEATHSIG, SIGKILL);
            }
            dup2(this->cpstdinp[0], 0);
            close(this->cpstdinp[1]);
            dup2(this->cpstdoutp[1], 1);
            close(this->cpstdoutp[0]);
            close(this->cpstdoutp[0]);
            dup2(this->cpstderrp[1], 2);
            close(this->cpstderrp[0]);
            if (daemon) {
                fi = open("/dev/null", O_APPEND | O_WRONLY);
                dup2(fi, 2);
            }
            execvp(args[0], args);
            if (daemon) {
                close(fi);
            }
            if (onStopHandler != NULL)onStopHandler();
            if (debug == 1) {
                close(this->cpstdoutp[1]);
                std::cout << "\n" + getTime() + "termination of pid: " + std::string(itoa(getpid())) + " command: " + command;
                fflush(stdout);
            }
        } else {
            close(this->cpstdinp[0]);
            this->cpstdin = this->cpstdinp[1];
            close(this->cpstdoutp[1]);
            this->cpstdout = this->cpstdoutp[0];
            close(this->cpstderrp[1]);
            this->cpstderr = this->cpstderrp[0];
            for (i = 0; i < a; i++) {
                free(args[a]);
            }
            if (block) {
                waitpid(this->cpid, &this->childExitStatus, 0);
            }
        }
    } else {

    }
}

int spawn::getChildExitStatus() {
    return this->childExitStatus;
}

int copyfile(std::string src, std::string dst) {
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(src.c_str(), O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

std::string getCurrentDir() {
    char cCurrentPath[FILENAME_MAX];
    if (!GetCurrentDir(cCurrentPath, sizeof (cCurrentPath))) {
        return std::string(itoa(errno));
    }
    cCurrentPath[sizeof (cCurrentPath) - 1] = '\0'; /* not really required */
    return std::string(cCurrentPath);
}

std::string getMachineName() {
    //struct addrinfo hints, *info, *p;
    //int gai_result;

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    //memset(&hints, 0, sizeof hints);
    //hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
    //hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_CANONNAME;

    //if ((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_result));
    //exit(1);
    //}
    //std::string mn = std::string(info->ai_canonname);
    //freeaddrinfo(info);
    return std::string(hostname);
}

static int jgjg(const char *fpath, const struct stat *sb, int tflag, struct FTW * ftwbuf) {
    unlink(fpath);
}

int rmmydir(std::string dirn) {
    //return nftw(dirn.c_str(), jgjg, 20, FTW_DEPTH | FTW_PHYS);
    std::string cmd = "rm -Rf " + dirn;
    system(cmd.c_str());
    return 1;
}

std::string inputText() {
    std::string buf;
    char ch;
    //while ((ch = getchar()) != '\n' && ch != EOF);
    ch = getch();
    while ((int) ch != 10) {
        if ((int) ch == '\b' || (int) ch == 127) {
            if (buf.size() > 0) {
                printf("\b \b");
                fflush(stdout);
                buf.erase(buf.size() - 1);
            }
        } else {
            buf.push_back(ch);
            std::cout << ch;
        }
        ch = getch();
    }
    return buf;
}

std::string inputPass() {
    char ch;
    std::string buf;
    //while ((ch = getchar()) != '\n' && ch != EOF);
    ch = getch();
    while ((int) ch != 10) {
        if ((int) ch == '\b' || (int) ch == 127) {
            if (buf.size() > 0) {
                printf("\b \b");
                fflush(stdout);
                buf.erase(buf.size() - 1);
            }
        } else {
            buf.push_back(ch);
            std::cout << '*';
        }
        ch = getch();
    }
    return buf;
}

std::string getStdoutFromCommand(std::string cmd) {
    std::string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1"); // Do we want STDERR?

    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    return data;
}

std::string getTime() {
    struct tm * timeinfo;
    char tb[20];
    time_t ct;
    time(&ct);
    timeinfo = localtime(&ct);
    strftime(tb, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(tb);
}

std::string get_command_line(pid_t pid) {
    FILE *f;
    char file[256], cmdline[256] = {0};
    sprintf(file, "/proc/%d/cmdline", pid);

    f = fopen(file, "r");
    if (f) {
        char *p = cmdline;
        fgets(cmdline, sizeof (cmdline) / sizeof (*cmdline), f);
        fclose(f);

        while (*p) {
            p += strlen(p);
            if (*(p + 1)) {
                *p = ' ';
            }
            p++;
        }
        return std::string(cmdline);
    } else {
        return std::string();
    }
}

int poke(std::string ip) {
    /*int mysocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct sockaddr_in sip;
    memset(&sip, '0', sizeof (sip));
    sip.sin_family = AF_INET;
    sip.sin_port = htons(80);
    if (inet_pton(AF_INET, ip.c_str(), &sip.sin_addr) <= 0) {
        return -1;
    }
    return connect(mysocket, (sockaddr*) & sip, sizeof (sip));*/
    if (debug == 1) {
        std::cout << "\n" + getTime() + " poking " + ip + "....";
        fflush(stdout);
    }
    spawn poke = spawn("ping -c 1 " + ip, false, NULL, false, true);
    int ces = poke.getChildExitStatus();
    if (debug == 1) {
        std::cout << "childExitStatus:" + std::string(itoa(ces)) + "\n";
        fflush(stdout);
    }
    return ces;
}
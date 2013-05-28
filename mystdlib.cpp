#include "mystdlib.h"
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
#include "myconverters.h"
#include <ftw.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <vector>

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

spawn::spawn(std::string command, bool daemon, void (*onStopHandler)(), bool freeChild) {
    pipe(this->cpstdinp);
    pipe(this->cpstdoutp);
    pipe(this->cpstderrp);
    this->cpid = fork();
    std::vector<std::string> cmdv = explode(" ", command);
    char * args[cmdv.size() + 1];
    int i = 0;
    for (i = 0; i < cmdv.size(); i++) {
        args[i] = (char*) cmdv[i].c_str();
    }
    args[i] = NULL;
    if (this->cpid == 0) {
        int fi;
        if (!freeChild) {
            prctl(PR_SET_PDEATHSIG, SIGKILL);
        }
        close(0);
        dup(this->cpstdinp[0]);
        close(this->cpstdinp[1]);
        close(1);
        dup(this->cpstdoutp[1]);
        close(this->cpstdoutp[0]);
        close(2);
        dup(this->cpstderrp[1]);
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
    } else {
        close(this->cpstdinp[0]);
        this->cpstdin = this->cpstdinp[1];
        close(this->cpstdoutp[1]);
        this->cpstdout = this->cpstdoutp[0];
        close(this->cpstderrp[1]);
        this->cpstderr = this->cpstderrp[0];
    }
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

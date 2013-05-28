#include "Multimedia.h"
#include "mystdlib.h"
#include "myconverters.h"
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iconv.h>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>

int videoSegmenter(std::string inputFile, int segmentLength, std::string outputFolder) {
    std::string ffcmd = "ffmpeg -i " + inputFile;
    std::string ffout = getStdoutFromCommand(ffcmd);
    int ffDoffset = ffout.find("Duration: ", 0);
    if (ffDoffset > 0) {
        outputFolder = outputFolder[outputFolder.length() - 1] == '/' ? outputFolder : outputFolder + "/";
        struct stat st;
        if (stat(outputFolder.c_str(), &st) != -1) {
            std::string ssi = std::string(itoa(segmentLength));
            std::string ts = ffout.substr(ffDoffset + 10, 11);
            int ei = inputFile.rfind(".", -1);
            std::string ifwoe = inputFile.substr(0, ei);
            std::string ife = inputFile.substr(ei);
            float its = timeToSec(ts);
            int sc = 1;
            std::string ifn = ifwoe.substr(ifwoe.rfind("/", -1) + 1);
            std::string ifm3u8 = outputFolder + ifn + ".m3u8";
            std::string m3u = "#EXTM3U"
                    "\n#EXT-X-TARGETDURATION:" + ssi;
            std::string cmd;
            std::string sn;
            spawn segmenter;
            while (its > 0) {
                sn = outputFolder + ifn + "-" + std::string(itoa(sc)) + ife;
                cmd = "ffmpeg -i " + inputFile + " -y -codec copy -ss " + std::string(itoa((sc - 1) * segmentLength)) + " -t " + ssi + sn;
                segmenter = spawn(cmd, false, NULL, true);
                waitpid(segmenter.cpid, NULL, WUNTRACED);
                m3u += "\n#EXTINF:" + (its < segmentLength ? std::string(itoa(its)) : ssi) + "," +
                        "\n" + ifn + "-" + std::string(itoa(sc)) + ife;
                its -= segmentLength;
                sc++;
            }
            m3u += "\n#EXT-X-ENDLIST";
            iconv_t cd = iconv_open("UTF-8", "");
            int fd = open(ifm3u8.c_str(), O_WRONLY | O_TRUNC | O_CREAT);
            char * src = (char *) m3u.c_str();
            char * inbuf = src;
            size_t inbytesleft = m3u.length();
            char dst[inbytesleft];
            char * outbuf = dst;
            size_t outbytesleft = inbytesleft;
            size_t s = iconv(cd, &inbuf, &inbytesleft, (char **) &outbuf, &outbytesleft);
            dst[m3u.length()] = '\0';
            write(fd, dst, strlen(dst));
            close(fd);
        } else {
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

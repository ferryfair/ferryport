/*
 * File:   main.cpp
 * Author: satya gowtham kudupudi
 *
 * Created on 15 March, 2013, 12:18 PM
 */

#include "Multimedia.h"
#include "myconverters.h"
#include "mystdlib.h"
#include "myxml.h"
#include "mycurl.h"
#include "debug.h"
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <getopt.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlstring.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/mman.h>
#include <dirent.h>
#include <vector>
#include <ostream>
#include <unistd.h>
#include <stdexcept>
#include <signal.h>
#include <sys/prctl.h>
#include <semaphore.h>

#define MAX_CAMS 10
#define APP_NAME "remotedevicecontroller"
using namespace std;

enum RecordState {
    RECORD_PREVIOUS_STATE, RECORD_STOP, RECORD_STREAM
};
string recordStateStr[] = {"RECORD_PREVIOUS_STATE", "RECORD_STOP", "RECORD_STREAM"};

enum camState {
    CAM_PREVIOUS_STATE, CAM_OFF, CAM_RECORD, CAM_STREAM, CAM_STREAM_N_RECORD, CAM_NEW_STATE
};
string camStateString [] = {"CAM_PREVIOUS_STATE", "CAM_OFF", "CAM_RECORD", "CAM_STREAM", "CAM_STREAM_N_RECORD", "CAM_NEW_STATE"};

sig_atomic_t child_exit_status;

string serverAddr;
string serverPort;
string streamAddr;
string streamPort;
string appName;
string xmlnamespace;
string systemId;
string securityKey;
string videoStreamingType;
string pollInterval;
string autoInsertCameras;
string configFile = "/etc/" + string(APP_NAME) + ".conf.xml";
string camFolder = "/dev/";
bool camcaptureCompression;
string recordResolution;
string streamResolution;
string recordfps;
string streamfps;
string mobileBroadbandCon;
string corpNWGW;
int manageNetwork = 0;
string gpsDevice;
string gpsSDeviceBaudrate;
int reconnectDuration;
int reconnectPollCount = 0;
int reconnectPollCountCopy;
string internetTestURL;
string recordsFolder = "/var/" + string(APP_NAME) + "records/";
string logFile = "/var/log/" + string(APP_NAME) + ".log";
string initFile = "/etc/init/" + string(APP_NAME) + ".conf";
string initOverrideFile = "/etc/init/" + string(APP_NAME) + ".override";
string initdFile = "/etc/init.d/" + string(APP_NAME);
string installationFolder = "/usr/local/bin/";
string binFile = installationFolder + string(APP_NAME);
string rootBinLnk = "/usr/bin/" + string(APP_NAME);
string srcFolder = "/usr/local/src/" + string(APP_NAME);
time_t gpsUpdatePeriod = 10;
bool pkilled = true;
int SOAPServiceReqCount = 0;
int ferr;

bool allCams = false;
string reqCam;
string reqSId;
bool newlyMarried = true;

camState ps = CAM_OFF;
camState cs = CAM_OFF;
time_t st;

pid_t rootProcess;
pid_t firstChild;
pid_t secondChild;
pid_t runningProcess;
string runMode = "normal";

time_t gpsReadStart;
time_t gpsReadEnd;
string gpsCoordinates;
pthread_t gpsUpdaterThread;

pthread_t nwMgrThread;
string modemInode = "/dev/ttyUSB0";
string currentIP;

time_t currentTime;


void run();
void stop();
string readConfigValue(string name);
void instReInstComCode(string sk);
void stopRunningProcess();
void* gpsLocationUpdater(void* arg);
void reinstallKey();
void* networkManager(void* arg);

class camService {
public:
    pid_t pid;
    int stdio[2];
    string cam;
    camState state;
    camState newState;
    string SId;
    bool disable;
    string recordPath;
    string streamPath;
};

class RecordsManager {
public:
    typedef int RecordIndex;

    class Record {
    public:
        string recordName[3];
        string recordFile;
        RecordState state;
        RecordState newState;
        string sId;
        pid_t spid;
        spawn recorder;

        class RecordFileNotFoundException {
        public:
            string notFoundFile;

            RecordFileNotFoundException(string f) {
                this->notFoundFile = f;
            }
        };

        Record(string* recordName) {
            struct stat s;
            string fL = recordsFolder + recordName[0] + "/" + recordName[1] + "/" + recordName[2];
            if (stat(fL.c_str(), &s) != -1) {
                this->recordName[0] = recordName[0];
                this->recordName[1] = recordName[1];
                this->recordName[2] = recordName[2];
                this->recordFile = fL;
                this->state = RECORD_STOP;
                this->newState = RECORD_PREVIOUS_STATE;
                this->spid = 0;
            } else {
                throw RecordFileNotFoundException(fL);
            }
        }
    };

private:
    static vector<Record> records;

    static void serviceStopHandler(pid_t pid) {

    }

public:

    static int setNewState(RecordIndex ri, RecordState rs) {
        try {
            if (records[ri].state != rs) {
                records[ri].newState = rs;
                return 1;
            } else {
                records[ri].newState = RECORD_PREVIOUS_STATE;
                return 0;
            }
        } catch (std::out_of_range e) {
            return -1;
        }
    }

    static int setSId(RecordIndex ri, string sid) {
        try {
            if (records[ri].sId.compare(sid) != 0) {
                records[ri].sId = string(sid);
                return 1;
            } else {
                return 0;
            }
        } catch (std::out_of_range e) {
            return -1;
        }
    }

    static RecordIndex addRecord(string * recordName) {
        if (getRecordIndex(recordName) == -1) {
            try {
                Record r = Record(recordName);
                RecordsManager::records.push_back(r);
                return RecordsManager::records.size() - 1;
            } catch (Record::RecordFileNotFoundException e) {
                return -1;
            }
        }
        return 0;
    }

    static Record* getRecord(string * recordName) {
        int i = 0;
        Record* sr;
        while (i < (int) RecordsManager::records.size()) {
            sr = &RecordsManager::records.at(i);
            if (sr->recordName[0].compare(recordName[0]) == 0) {
                if (sr->recordName[1].compare(recordName[1]) == 0) {
                    if (sr->recordName[2].compare(recordName[2]) == 0) {
                        return sr;
                    }
                }
            }
            i++;
        }
        return NULL;
    }

    static Record* getRecordByPID(pid_t pid) {
        int i = 0;
        Record* sr;
        while (i < records.size()) {
            sr = &records.at(i);
            if (sr->spid == pid) {
                return sr;
            }
            i++;
        }
        return NULL;
    }

    static int getRecordIndex(string* recordName) {
        int i = 0;
        Record* sr;
        while (i < (int) RecordsManager::records.size()) {
            sr = &RecordsManager::records.at(i);
            if (sr->recordName[0].compare(recordName[0]) == 0) {
                if (sr->recordName[1].compare(recordName[1]) == 0) {
                    if (sr->recordName[2].compare(recordName[2]) == 0) {
                        return i;
                    }
                }
            }
            i++;
        }
        return -1;
    }

    static int setRecordState(int rIndex) {
        struct stat rStat;
        char procF[20] = "/proc/";
        string sa = "rtmp://" + streamAddr + ":" + streamPort + "/oflaDemo/RecordedFiles" + records[rIndex].sId;
        string rfa = records[rIndex].recordFile;
        if (records[rIndex].newState == RECORD_STREAM) {
            stopRecord(rIndex);
            string cmd = "ffmpeg -re -i " + rfa + " -r " + streamfps + " -s " + streamResolution + " -f flv " + sa;
            records[rIndex].recorder = spawn(cmd, true, NULL, false);
            if ((debug & 1) == 1) {
                cout << "\n" + getTime() + " setRecordState: " + cmd + " :" + string(itoa(records[rIndex].recorder.cpid)) + "\n";
                fflush(stdout);
            }
            records[rIndex].spid = records[rIndex].recorder.cpid;
            records[rIndex].newState = RECORD_PREVIOUS_STATE;
            records[rIndex].state = RECORD_STREAM;
        } else if (records[rIndex].newState == RECORD_STOP) {
            stopRecord(rIndex);
            records[rIndex].newState = RECORD_PREVIOUS_STATE;
        }
        if (records[rIndex].spid == 0)return -1;
        return 1;
    }

    static int stopRecord(RecordIndex ri) {
        int i = 0;
        records[ri].state = RECORD_STOP;
        string proc = "/proc/" + string(itoa(records[ri].spid));
        struct stat ptr;
        if (stat(proc.c_str(), &ptr) != -1) {
            pkilled = false;
            i = kill(records[ri].spid, SIGTERM);
            while (!pkilled);
        }
        return i;
    }
};
vector<RecordsManager::Record> RecordsManager::records;

class csList {
private:
    static int s;
    static camService csl[];
    static int camCount;

    static int moveCamService(int srcIndex, int dstIndex) {
        if (csList::csl[srcIndex].cam.length() > 0) {
            csList::copyCamService(srcIndex, dstIndex);
            csList::cleanCamService(srcIndex);
            return dstIndex;
        } else {
            return -1;
        }
    }

    static int copyCamService(int srcIndex, int dstIndex) {
        if (csList::csl[srcIndex].cam.length() > 0) {
            csList::csl[dstIndex].cam = csList::csl[srcIndex].cam;
            csList::csl[dstIndex].pid = csList::csl[srcIndex].pid;
            csList::csl[dstIndex].SId = csList::csl[srcIndex].SId;
            csList::csl[dstIndex].recordPath = csList::csl[srcIndex].recordPath;
            csList::csl[dstIndex].state = csList::csl[srcIndex].state;
            csList::csl[dstIndex].newState = csList::csl[srcIndex].newState;
            csList::csl[dstIndex].disable = csList::csl[srcIndex].disable;
            return srcIndex;
        } else {
            return -1;
        }
    }

    static int cleanCamService(int srcIndex) {
        if (srcIndex < MAX_CAMS && srcIndex>-1) {
            csList::csl[srcIndex].cam = "";
            csList::csl[srcIndex].SId = "";
            csList::csl[srcIndex].disable = false;
            csList::csl[srcIndex].newState = CAM_PREVIOUS_STATE;
            csList::csl[srcIndex].pid = 0;
            csList::csl[srcIndex].recordPath = "";
            csList::csl[srcIndex].state = CAM_PREVIOUS_STATE;
            csList::csl[srcIndex].streamPath = "";
        }
    }
public:

    csList() {
        camCount = 0;
        s = 0;
    }

    static void initialize(int ss) {
        s = ss;
    }

    static void addCamService(string cam) {
        int i = 0;
        bool camAdded = false;
        while (csList::csl[i].cam.length() != 0) {
            if ((int) csList::csl[i].cam.find(cam, 0) == 0) {
                camAdded = true;
            }
            i++;
        }
        if (!camAdded) {
            csList::csl[i].cam = cam;
            csList::camCount++;
            csList::csl[i].state = CAM_OFF;
            csList::csl[i].newState = CAM_RECORD;
        }
    }

    static void removeCamService(string cam) {
        int csIndex;
        camService cs = getCamService(cam, &csIndex);
        if (csIndex != -1) {
            if (csList::camCount > 1) {
                csList::moveCamService(csList::camCount - 1, csIndex);
            } else {
                csList::csl[csIndex].cam = "";
            }
        }
        csList::camCount--;
    }

    static camService getCamService(string cam, int * index) {
        int i = 0;
        camService *nul = new camService;
        while (csl[i].cam.length() != 0) {
            if (csl[i].cam.compare(cam) == 0) {
                *index = i;
                return csl[i];
            }
            i++;
        }
        *index = -1;
        return *nul;
    }

    static int getCamCount() {
        return csList::camCount;
    }

    static bool camReattached() {
        int i = 0;
        string procF = "/proc/";
        string proc;
        struct stat ptr;
        while (i < csList::camCount) {
            if (!csList::csl[i].disable) {
                proc = procF + string(itoa(csl[i].pid));
                if (stat(proc.c_str(), &ptr) == -1) {
                    return true;
                }
            }
            i++;
        }
        return false;
    }

    static pid_t setCamState(string cam) {
        int csIndex;
        camService cs = csList::getCamService(cam, &csIndex);
        if (!cs.disable) {
            string dev = camFolder + cam;
            pid_t fcpid = cs.pid;
            camState ns = cs.state;
            spawn process;
            string cmd;
            if (cs.newState == CAM_RECORD) {
                cmd = "ffmpeg -f video4linux2 " + (camcaptureCompression ? string("-vcodec mjpeg ") : string("")) + "-r " + recordfps + " -s " + recordResolution + " -i " + dev + " " + cs.recordPath;
                csList::stopCam(cam);
                process = spawn(cmd, true, NULL, false);
                if ((debug & 1) == 1) {
                    cout << "\n" + getTime() + " setCamState: " + cmd + " :" + string(itoa(process.cpid)) + "\n";
                    fflush(stdout);
                }
                fcpid = process.cpid;
                ns = CAM_RECORD;
            } else if (cs.newState == CAM_STREAM) {
                cmd = "ffmpeg -f video4linux2 " + (camcaptureCompression ? string("-vcodec mjpeg ") : string("")) + "-r " + recordfps + " -s " + recordResolution + " -i " + dev + " -r " + streamfps + " -s " + streamResolution + " -f flv " + cs.streamPath;
                csList::stopCam(cam);
                process = spawn(cmd, true, NULL, false);
                if ((debug & 1) == 1) {
                    cout << "\n" + getTime() + " setCamState: " + cmd + " :" + string(itoa(process.cpid)) + "\n";
                    fflush(stdout);
                }
                fcpid = process.cpid;
                ns = CAM_STREAM;
            } else if (cs.newState == CAM_STREAM_N_RECORD) {
                cmd = "ffmpeg -f video4linux2 " + (camcaptureCompression ? string("-vcodec mjpeg ") : string("")) + "-r " + recordfps + " -s " + recordResolution + " -i " + dev + " -r " + streamfps + " -s " + streamResolution + " -f flv " + cs.streamPath + " " + cs.recordPath;
                csList::stopCam(cam);
                process = spawn(cmd, true, NULL, false);
                if ((debug & 1) == 1) {
                    cout << "\n" + getTime() + " setCamState: " + cmd + " :" + string(itoa(process.cpid)) + "\n";
                    fflush(stdout);
                }
                fcpid = process.cpid;
                ns = CAM_STREAM_N_RECORD;
            } else if (cs.newState == CAM_OFF) {
                kill(cs.pid, SIGTERM);
            }
            csList::csl[csIndex].pid = fcpid;
            csList::csl[csIndex].state = ns;
            csList::csl[csIndex].newState = CAM_PREVIOUS_STATE;
            return fcpid;
        } else {
            return -1;
        }
    }

    static int stopCam(string cam) {
        int i = 0;
        int csIndex;
        camService cs = csList::getCamService(cam, &csIndex);
        string proc = "/proc/" + string(itoa(cs.pid));
        struct stat ptr;
        if (stat(proc.c_str(), &ptr) != -1) {
            pkilled = false;
            i = kill(cs.pid, SIGKILL);
            while (!pkilled);
        }
        cs.state = CAM_OFF;
        return i;
    }

    static void setCams(string * cams) {
        int i = 0;
        int j = 0;
        bool camFound = false;
        string procf = "/proc/";
        string proc;
        struct stat ptr;
        while (i < csList::camCount) {
            j = 0;
            while (cams[j].length() != 0) {
                if (csList::csl[i].cam.compare(cams[j]) == 0) {
                    camFound = true;
                    break;
                }
                j++;
            }
            if (camFound) {
                proc = procf + string(itoa(csList::csl[i].pid));
                if (stat(proc.c_str(), &ptr) == -1) {
                    camFound = false;
                    csList::removeCamService(string(cams[j]));
                } else {
                    i++;
                }
            } else {
                csList::removeCamService(string(cams[j]));
            }
        }
        i = 0;
        while (cams[i].length() > 0) {
            csList::addCamService(string(cams[i]));
            i++;
        }
    }

    static int setPid(string cam, pid_t pid) {
        int csIndex;
        csList::getCamService(cam, &csIndex);
        csList::csl[csIndex].pid = pid;
        return csIndex;
    }

    static int setSId(string cam, string SId) {
        int csIndex;
        csList::getCamService(cam, &csIndex);
        if (csIndex != -1) {
            csList::csl[csIndex].SId = SId;
            return csIndex;
        } else {
            return -1;
        }
    }

    static int setNewCamState(string cam, camState ns) {
        int csIndex;
        getCamService(cam, &csIndex);
        if (csIndex != -1) {
            if (csList::csl[csIndex].state != ns) {
                csList::csl[csIndex].newState = ns;
                return 1;
            } else {
                return 0;
            }
        } else {
            return -1;
        }
    }

    static void setRecordPath(string cam, string recordPath) {
        int csIndex;
        getCamService(cam, &csIndex);
        csList::csl[csIndex].recordPath = recordPath;
    }

    static void setStreamPath(string cam, string streamPath) {
        int csIndex;
        getCamService(cam, &csIndex);
        csList::csl[csIndex].streamPath = streamPath;
    }

    static int setStateAllCams(camState state) {
        int i = 0;
        while (i < csList::camCount) {
            if (csList::csl[i].state != state) {
                csList::csl[i].newState = state;
            }
            i++;
        }
        return 0;
    }

    static void getCams(string * cams) {
        int i = 0;
        while (i < csList::camCount) {
            cams[i] = string(csList::csl[i].cam);
            i++;
        }
    }

    static string getCamsWithStateString() {
        string strCameras = "";
        int i = 0;
        while (i < csList::camCount) {
            strCameras += csList::csl[i].cam + ":" + camStateString[csList::csl[i].state] + ((i + 1 == csList::camCount) ? "" : "^");
            i++;
        }
        return strCameras;
    }

};
int csList::s = 0;
camService csList::csl[MAX_CAMS];
int csList::camCount;

void uninstall() {
    stopRunningProcess();
    cout << "\nDeleting " + initFile;
    unlink(initFile.c_str());
    cout << "\nDeleting " + initOverrideFile;
    unlink(initOverrideFile.c_str());
    cout << "\nDeleting " + initdFile;
    unlink(initdFile.c_str());
    cout << "\nDeleting  " + configFile;
    unlink(configFile.c_str());
    cout << "\nDeleting " + binFile;
    unlink(binFile.c_str());
    cout << "\nDeleting " + logFile;
    unlink(logFile.c_str());
    cout << "\nDeleting " + rootBinLnk;
    unlink(rootBinLnk.c_str());
    string input;
decision:
    cout << "\nDo you want to remove recorded files?(yes/no):";
    input = inputText();
    if (input.compare("yes") == 0) {
        rmmydir(recordsFolder);
    } else if (input.compare("no") == 0) {
        cout << "\nPlease find the records at " + recordsFolder;
    } else {
        goto decision;
    }
    cout << "\n" + string(APP_NAME) + " uninstalled successfully :D\n";
}

string generateSecurityKey() {
    return systemId;
}

void readConfig() {
    xmlInitParser();
    struct stat st;
    xmlDoc * xd;
    if (stat(configFile.c_str(), &st) != -1) {
        xd = xmlParseFile(configFile.c_str());
    } else {
        xd = xmlParseFile("config.xml");
    }
    xmlXPathContext *xc = xmlXPathNewContext(xd);
    xmlXPathObject* xo = xmlXPathEvalExpression((xmlChar*) "/config/server-addr", xc);
    xmlNode* node;
    node = xo->nodesetval->nodeTab[0];
    serverAddr = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/server-port", xc);
    node = xo->nodesetval->nodeTab[0];
    serverPort = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/stream-addr", xc);
    node = xo->nodesetval->nodeTab[0];
    streamAddr = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/stream-port", xc);
    node = xo->nodesetval->nodeTab[0];
    streamPort = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/app-name", xc);
    node = xo->nodesetval->nodeTab[0];
    appName = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/namespace", xc);
    node = xo->nodesetval->nodeTab[0];
    xmlnamespace = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/debug", xc);
    node = xo->nodesetval->nodeTab[0];
    debug = atoi((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/camcapture-compression", xc);
    node = xo->nodesetval->nodeTab[0];
    camcaptureCompression = (string((char*) xmlNodeGetContent(node)).compare("true") == 0);
    xo = xmlXPathEvalExpression((xmlChar*) "/config/record-fps", xc);
    node = xo->nodesetval->nodeTab[0];
    recordfps = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/record-resolution", xc);
    node = xo->nodesetval->nodeTab[0];
    recordResolution = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/stream-fps", xc);
    node = xo->nodesetval->nodeTab[0];
    streamfps = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/stream-resolution", xc);
    node = xo->nodesetval->nodeTab[0];
    streamResolution = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/manage-network", xc);
    node = xo->nodesetval->nodeTab[0];
    manageNetwork = atoi((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/reconnect-duration", xc);
    node = xo->nodesetval->nodeTab[0];
    reconnectDuration = atoi((char*) xmlNodeGetContent(node))*60;
    xo = xmlXPathEvalExpression((xmlChar*) "/config/reconnect-poll-count", xc);
    node = xo->nodesetval->nodeTab[0];
    reconnectPollCount = atoi((char*) xmlNodeGetContent(node));
    reconnectPollCountCopy = reconnectPollCount;
    xo = xmlXPathEvalExpression((xmlChar*) "/config/mobile-broadband-connection", xc);
    node = xo->nodesetval->nodeTab[0];
    mobileBroadbandCon = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/internet-test-url", xc);
    node = xo->nodesetval->nodeTab[0];
    internetTestURL = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/corporate-network-gateway", xc);
    node = xo->nodesetval->nodeTab[0];
    corpNWGW = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/gps-device", xc);
    node = xo->nodesetval->nodeTab[0];
    gpsDevice = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/gps-sdevice-baudrate", xc);
    node = xo->nodesetval->nodeTab[0];
    gpsSDeviceBaudrate = string((char*) xmlNodeGetContent(node));
    xo = xmlXPathEvalExpression((xmlChar*) "/config/system-id", xc);
    if (xo->nodesetval->nodeNr > 0) {
        node = xo->nodesetval->nodeTab[0];
        systemId = string((char*) xmlNodeGetContent(node));
        xo = xmlXPathEvalExpression((xmlChar*) "/config/videoStreamingType", xc);
        node = xo->nodesetval->nodeTab[0];
        videoStreamingType = string((char*) xmlNodeGetContent(node));
        xo = xmlXPathEvalExpression((xmlChar*) "/config/pollInterval", xc);
        node = xo->nodesetval->nodeTab[0];
        pollInterval = string((char*) xmlNodeGetContent(node));
        xo = xmlXPathEvalExpression((xmlChar*) "/config/autoInsertCameras", xc);
        node = xo->nodesetval->nodeTab[0];
        autoInsertCameras = string((char*) xmlNodeGetContent(node));
    }
    securityKey = generateSecurityKey();
    xmlCleanupParser();
}

void writeConfigValue(string name, string value) {
    xmlInitParser();
    xmlDoc * xd;
    struct stat st;
    if (stat(configFile.c_str(), &st) != -1) {
        xd = xmlParseFile(configFile.c_str());
    } else {
        xd = xmlParseFile("config.xml");
    }
    xmlXPathContext* xc = xmlXPathNewContext(xd);
    xmlXPathObject* rxo = xmlXPathEvalExpression((xmlChar*) "/config", xc);
    xmlNode* rn;
    rn = rxo->nodesetval->nodeTab[0];
    string xpath = "/config/" + name;
    xmlXPathObject *xo = xmlXPathEvalExpression((xmlChar*) xpath.c_str(), xc);
    int nn = xo->nodesetval->nodeNr;
    if (nn > 0) {
        xmlNode* node = xo->nodesetval->nodeTab[0];
        xmlNodeSetContent(node, (xmlChar*) value.c_str());
    } else {
        xmlNewTextChild(rn, NULL, (xmlChar*) name.c_str(), (xmlChar*) value.c_str());
    }
    xmlChar* s;
    int size;
    xmlDocDumpMemory(xd, &s, &size);
    FILE *fp = fopen(configFile.c_str(), "w");
    fwrite((char*) s, 1, size, fp);
    fclose(fp);
}

string readConfigValue(string name) {
    xmlInitParser();
    xmlDoc * xd;
    struct stat st;
    if (stat(configFile.c_str(), &st) != -1) {
        xd = xmlParseFile(configFile.c_str());
    } else {
        xd = xmlParseFile("config.xml");
    }
    xmlXPathContext* xc = xmlXPathNewContext(xd);
    string xpath = "/config/" + name;
    xmlXPathObject* xo = xmlXPathEvalExpression((xmlChar*) xpath.c_str(), xc);
    int nn = xo->nodesetval->nodeNr;
    if (nn > 0) {
        xmlNode* node = xo->nodesetval->nodeTab[0];
        return string((char*) xmlNodeGetContent(node));
    } else {
        return "";
    }
    xmlCleanupParser();
}

void getCameras() {
    DIR *dpdf;
    struct dirent *epdf;
    dpdf = opendir("/dev/");
    string fn;
    int i = 0;
    i = 0;
    string cams[MAX_CAMS];
    while (epdf = readdir(dpdf)) {
        fn = string(epdf->d_name);
        if (fn.find("video", 0) == 0) {
            cams[i] = fn;
            i++;
        }
    }
    closedir(dpdf);
    csList::setCams(cams);
}

void record() {
    time_t rawtime;
    struct tm * timeinfo;
    char fn [80];
    char dn [80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(fn, 80, "%Y-%m-%d_%H:%M:%S.flv", timeinfo);
    strftime(dn, 80, "%Y-%m-%d", timeinfo);
    string path = "/var/" + string(APP_NAME) + "records/" + string(dn) + "/";
    struct stat st = {0};
    if (stat(path.c_str(), &st) == -1) {
        mkdir(path.c_str(), 0774);
    }
    struct stat fileAtt;
    int i = 0;
    string vd[MAX_CAMS];
    csList::getCams(vd);
    string fold;
    string dev;
    string fname;
    string sa;
    i = 0;
    int k = 0;
    int csIndex;
    camService cs;
    while (vd[i].length() != 0) {
        k = 0;
        if (!(cs = csList::getCamService(vd[i], &csIndex)).disable && (csIndex != -1) && cs.newState != CAM_PREVIOUS_STATE) {
            fold = path + vd[i];
            if (stat(fold.c_str(), &fileAtt) == -1) {
                mkdir(fold.c_str(), 0774);
            }
            dev = "/dev/" + vd[i];
            fname = fold + "/" + fn;
            sa = "rtmp://" + streamAddr + ":" + streamPort + "/oflaDemo/" + cs.SId;
            csList::setRecordPath(vd[i], fname);
            csList::setStreamPath(vd[i], sa);
            csList::setCamState(vd[i]);
        }
        i++;
    }
    allCams = false;
}

string getStrRecordedFiles() {
    DIR *dpdf;
    DIR *dateFolder;
    DIR *vidFolder;
    struct dirent *epdf;
    struct dirent *dfd;
    struct dirent *vfd;
    dpdf = opendir(recordsFolder.c_str());
    string fn;
    string dn;
    string vn;
    int i = 0;
    i = 0;
    string rfs = "";
    vector<string> sf;
    vector<string> vrfs;
    while (epdf = readdir(dpdf)) {
        fn = string(epdf->d_name);
        if (fn.compare(".") != 0 && fn.compare("..") != 0) {
            string afn = recordsFolder + fn + "/";
            if ((dateFolder = opendir(afn.c_str())) != NULL) {
                while (dfd = readdir(dateFolder)) {
                    dn = string(dfd->d_name);
                    if (dn.compare(".") != 0 && dn.compare("..") != 0) {
                        string adn = afn + dn + "/";
                        if ((vidFolder = opendir(adn.c_str())) != NULL) {
                            while (vfd = readdir(vidFolder)) {
                                vn = string(vfd->d_name);
                                if (vn.compare(".") != 0 && vn.compare("..") != 0) {
                                    sf.clear();
                                    sf.push_back(fn);
                                    sf.push_back(dn);
                                    sf.push_back(vn);
                                    vrfs.push_back(implode("%", sf));
                                }
                            }
                        }
                    }
                }
            };
        }
    }
    rfs = implode("^", vrfs);
    return rfs;
}

string reqSOAPService(string service, xmlChar* content) {
    xmlInitParser();

    xmlDoc* xd = xmlParseDoc((xmlChar*) "<?xml version=\"1.0\" encoding=\"utf-8\"?><soap12:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\"><soap12:Body><content></content></soap12:Body></soap12:Envelope>");
    xmlXPathContext *xpathCtx = xmlXPathNewContext(xd);
    xmlXPathRegisterNs(xpathCtx, (xmlChar*) "soap12", (xmlChar*) "http://www.w3.org/2003/05/soap-envelope");
    xmlXPathRegisterNs(xpathCtx, (xmlChar*) "xsd", (xmlChar*) "http://www.w3.org/2001/XMLSchema");
    xmlXPathRegisterNs(xpathCtx, (xmlChar*) "xsi", (xmlChar*) "http://www.w3.org/2001/XMLSchema-instance");
    xmlXPathRegisterNs(xpathCtx, (xmlChar*) "n", (xmlChar*) xmlnamespace.c_str());
    xmlXPathObject * accNameXpathObj = xmlXPathEvalExpression((xmlChar*) "/soap12:Envelope/soap12:Body/content", xpathCtx);

    xmlNode *node = accNameXpathObj->nodesetval->nodeTab[0];
    xmlDoc* cxd = xmlParseDoc(content);
    xmlNodePtr xnp = xmlDocGetRootElement(cxd);
    xmlReplaceNode(node, xnp);
    xmlChar* s;
    int size;
    xmlDocDumpMemory(xd, &s, &size);
    xmlCleanupParser();
    SOAPServiceReqCount++;
    string res = SOAPReq(serverAddr, serverPort, "/" + appName + "/CamCaptureService.asmx", xmlnamespace + "/" + service, string((char*) s), false);
    return res;
}

camState camStateChange() {
    getCameras();
    ps = cs;
    cs = csList::camReattached() ? CAM_NEW_STATE : CAM_PREVIOUS_STATE;
    string strCameras = csList::getCamsWithStateString();
    string strGPS = "NOGPSDEVICE";
    time(&currentTime);
    if (currentTime - gpsReadStart < gpsUpdatePeriod + 2) {
        strGPS = gpsCoordinates;
    }
    fflush(stdout);
    string IP = GetPrimaryIp();
    if (IP.compare(currentIP) != 0) {
        currentIP = IP;
        csList::setStateAllCams(CAM_RECORD);
        record();
    } else if (IP.length() == 0) {
        cerr << "\n" + getTime() + " CONNECTION ERROR.\n";
        csList::setStateAllCams(CAM_RECORD);
        cs = CAM_NEW_STATE;
        return cs;
    }
    string strNetwork = "ip:" + currentIP + ",signalstrength:-1";
    string content = "<GetDataChangeBySystemId xmlns=\"" + xmlnamespace + "\"><SystemName>" + getMachineName() + "</SystemName><SecurityKey>" + securityKey + "</SecurityKey><Cameras>" + strCameras + "</Cameras><GPS>" + strGPS + "</GPS><network>" + strNetwork + "</network></GetDataChangeBySystemId>";
    if ((debug & 1) == 1) {
        cout << "\n" + getTime() + " SOAPRequest " + string(itoa(SOAPServiceReqCount)) + ": " + content + "\n";
        fflush(stdout);
    }
    string response = reqSOAPService("GetDataChangeBySystemId", (xmlChar*) content.c_str());
    if (response.compare("CONNECTION ERROR") == 0) {
        cerr << "\n" + getTime() + " CONNECTION ERROR.\n";
        csList::setStateAllCams(CAM_RECORD);
        cs = CAM_NEW_STATE;
        return cs;
    }
    if ((debug & 1) == 1) {
        cout << "\n" + getTime() + " SOAPResponse: " + response + "\n";
        fflush(stdout);
    }
    xmlChar *res = (xmlChar*) response.c_str();
    xmlDoc *xd = xmlParseDoc(res);
    xmlXPathContext *xpathCtx = xmlXPathNewContext(xd);
    xmlXPathObject * xpathObj = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='GetDataChangeBySystemIdResult']", xpathCtx);
    xmlNode *node = xpathObj->nodesetval->nodeTab[0];
    if (xpathObj->nodesetval->nodeNr > 0) {
        node = node->children;
        string resCon;
        while (node != NULL) {
            resCon = string((char*) xmlNodeGetContent(node));
            if ((int) resCon.find("StartVideoRecord^", 0) >= 0) {
                cs = CAM_RECORD;
            } else if ((int) resCon.find("StopVideoRecord^", 0) >= 0) {
                cs = CAM_OFF;
            } else if ((int) resCon.find("StartVideoStream^", 0) >= 0) {
                cs = CAM_OFF;
            } else if ((int) resCon.find("StopVideoStream^", 0) >= 0) {
                vector<string> devs = explode("^", resCon);
                unsigned int devCount = devs.size();
                int i = 1;
                for (i = 1; i < devCount; i++) {
                    if (csList::setNewCamState(devs[i], CAM_RECORD) == 1) {
                        cs = CAM_NEW_STATE;
                    };
                }
            } else if ((int) resCon.find("StartSoundRecord^", 0) >= 0) {
            } else if ((int) resCon.find("StopSoundRecord^", 0) >= 0) {
            } else if ((int) resCon.find("StartSoundStream^", 0) >= 0) {
            } else if ((int) resCon.find("StopSoundStream^", 0) >= 0) {
            } else if ((int) resCon.find("DeleteSystem", 0) >= 0) {
                uninstall();
            } else if ((int) resCon.find("ViewAllRecordedFiles", 0) >= 0) {
                string strRecordedFiles = getStrRecordedFiles();
                content = "<InsertRecordedFiles xmlns=\"" + xmlnamespace + "\"><SystemSecurityKey>" + securityKey + "</SystemSecurityKey><strRecordedFiles>" + strRecordedFiles + "</strRecordedFiles></InsertRecordedFiles>";
                response = reqSOAPService("InsertRecordedFiles", (xmlChar*) content.c_str());
            } else if ((int) resCon.find("ViewSingleRecordedFile", 0) >= 0) {
                string toStreamVidFilesStr = resCon.substr(23);
                vector<string> toStreamVidFilesVector = explode("^", toStreamVidFilesStr);
                vector<string>toStreamVidFileAttrVect;
                int i = 0;
                for (int i = 0; i < toStreamVidFilesVector.size(); i++) {
                    toStreamVidFileAttrVect.clear();
                    toStreamVidFileAttrVect = explode("%", toStreamVidFilesVector.at(i));
                    string recordName[3] = {toStreamVidFileAttrVect.at(2), toStreamVidFileAttrVect.at(3), toStreamVidFileAttrVect.at(4)};
                    if (toStreamVidFileAttrVect.at(5).compare("True") == 0) {
                        RecordsManager::RecordIndex ri = RecordsManager::addRecord(recordName);
                        if (ri != -1) {
                            RecordsManager::setNewState(ri, RECORD_STREAM);
                            RecordsManager::setSId(ri, toStreamVidFileAttrVect.at(0));
                            RecordsManager::setRecordState(ri);
                        }
                    } else {
                        RecordsManager::RecordIndex ri = RecordsManager::getRecordIndex(recordName);
                        if (ri != -1) {
                            RecordsManager::setNewState(ri, RECORD_STOP);
                            RecordsManager::setRecordState(ri);
                        }
                    }
                }
            } else if ((int) resCon.find("ViewAll^") >= 0) {
                cs = CAM_NEW_STATE;
                string camsi = resCon.substr(8);
                vector<string> a = explode("^", camsi);
                int camCount = a.size();
                vector<string> b;
                int i;
                for (i = 0; i < camCount; i++) {
                    b.empty();
                    b = explode(":", a[i]);
                    csList::setSId(b[0], b[1]);
                }
                csList::setStateAllCams(CAM_STREAM_N_RECORD);
            } else if ((int) resCon.find("View^") >= 0) {
                int sIsind = resCon.find(":", 0) - 5;
                reqCam = resCon.substr(5, sIsind);
                reqSId = resCon.substr(sIsind + 6);
                csList::setSId(reqCam, reqSId);
                if (csList::setNewCamState(reqCam, CAM_STREAM_N_RECORD) == 1) {
                    cs = CAM_NEW_STATE;
                };
            }
            node = node->next;
        }
    }
    xmlCleanupParser();
    fflush(stdout);
    return cs;
}

void print_usage(FILE* stream, int exit_code, char* program_name) {
    fprintf(stream, "Usage: %s <option> [<parameter>]\n", program_name);
    string doc = "-c --configure Configures " + string(APP_NAME) + ""
            "\n-d --update Updates " + string(APP_NAME) + ""
            "\n-h --help Display this usage information."
            "\n-i --install Installs " + string(APP_NAME) + "."
            "\n-k --keyInstall Installs " + string(APP_NAME) + " with key given by user."
            "\n-r --reinstall Reinstall the " + string(APP_NAME) + ""
            "\n-s --start=\033[4mTYPE\033[0m Runs client. If \033[4mTYPE\033[0m is 'daemon' " + string(APP_NAME) + " runs as daemon. If \033[4mTYPE\033[0m is 'normal' " + string(APP_NAME) + " runs normally."
            "\n-u --uninstall Uninstalls " + string(APP_NAME) + "."
            "\n-x --stop Terminates " + string(APP_NAME) + "."
            "\n---------------------------"
            "\nHave a nice day :)\n\n";
    fprintf(stream, (const char*) doc.c_str());
    exit(exit_code);
}

void run() {
    secondChild = getpid();
    readConfig();
    if (runMode.compare("daemon") == 0) {
        if ((debug & 1) == 1) {
            dup2(ferr, 1);
            stdoutfd = ferr;
        } else {
            close(1);
        }
    }
    if (securityKey.length() == 0) {
        cout << "\nPlease install or re-install " + string(APP_NAME) + ".";
    } else {
        if (geteuid() != 0) {
            cout << "\nPlease login as root are sudo user.\n";
        } else {
            pthread_create(&gpsUpdaterThread, NULL, &gpsLocationUpdater, NULL);
            if (manageNetwork == 1) {
                pthread_create(&nwMgrThread, NULL, &networkManager, NULL);
            }
            writeConfigValue("pid", string(itoa(rootProcess)));
            csList::initialize(10);
            int ecode;
            time(&st);
            time_t ct;
            int pis = atoi(pollInterval.c_str()) / 1000;
            int pst = pis;
            camState csc;
            camState ps;
            while (true) {
                time(&ct);
                if (ct - st > 900) {
                    csc = ps;
                    st = ct;
                    pst = 0;
                    csList::setStateAllCams(CAM_RECORD);
                    cs = CAM_NEW_STATE;
                } else {
                    if (newlyMarried) {
                        getCameras();
                        csc = CAM_NEW_STATE;
                        cs = csc;
                        csList::setStateAllCams(CAM_RECORD);
                        newlyMarried = false;
                        allCams = true;
                        pst = 0;
                    } else {
                        csc = camStateChange();
                        pst = pis;
                    }
                }
                switch (csc) {
                    case CAM_NEW_STATE:
                        ps = csc;
                        record();
                        break;
                    case CAM_OFF:
                        ps = csc;
                        ecode = system("pkill -9 ffmpeg");
                        break;
                    default:
                        break;
                }
                sleep(pst);
            }
        }
    }
}

void install() {
    if (geteuid() != 0) {
        cout << "\nPlease login as root are sudo user.\n";
    } else {
        string cmd = "" + string(APP_NAME) + " -u";
        system(cmd.c_str());
        readConfig();
        int ret = system("which ffmpeg > /dev/null");
        if (ret == 0) {
            cout << "\n Do you want to install " + string(APP_NAME) + " with existing key(y) or with a new key(n)? ";
            string yn = inputText();
            if (yn.compare("n") == 0) {
                cout << "Installation process for MEKCamController\nPlease give in prompted information...";
                cout << "\nusername: ";
                string username = inputText();
                cout << "\npassword: ";
                string password = inputPass();
                string content = "<VendorLoginForClientComponent xmlns='" + xmlnamespace + "'><AccountName></AccountName><Username>" + username + "</Username><Password>" + password + "</Password></VendorLoginForClientComponent>";
                string res = reqSOAPService("VendorLoginForClientComponent", (xmlChar*) content.c_str());
                xmlInitParser();
                xmlDoc* xd2 = xmlParseDoc((xmlChar*) res.c_str());
                xmlXPathContext *xpc2 = xmlXPathNewContext(xd2);
                xmlXPathObject * tableXpathObj = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='Table']", xpc2);
                xmlNode *node4;
                int nn = tableXpathObj->nodesetval->nodeNr;
                if (nn > 0) {
                    cout << "\nSelect one of the systems:\n----------------------------\n";
                    cout << "SNO\tBranchId\tBranchName\tSystemId\tSystemName";
                    fflush(stdout);
                    int i;
                    char* bn[nn];
                    for (i = 0; i < tableXpathObj->nodesetval->nodeNr; i++) {
                        node4 = tableXpathObj->nodesetval->nodeTab[i];
                        cout << ("\n" + string(itoa(i + 1)) + ".\t");
                        fflush(stdout);
                        cout << (string((char*) xmlNodeGetContent(node4->children)) + "\t\t" + string((char*) xmlNodeGetContent(node4->children->next)) + "\t" + string(bn[i] = (char*) xmlNodeGetContent(node4->children->next->next)) + "\t\t" + string((char*) xmlNodeGetContent(node4->children->next->next->next)));
                        fflush(stdout);
                    }
                    xmlCleanupParser();
                    cout << "\nSelect a system(SNO): ";
                    cin >> i;
                    if (i > 0 && i <= nn) {
                        i -= 1;
                        instReInstComCode(bn[i]);
                    } else {
                        cout << "\nSNO out of range.";
                        cout << "\nSelect a system(SNO): ";
                        cin >> i;
                    }
                } else {
                    cout << "\nNo systems are allocated to this user.\n";
                }
                xmlCleanupParser();
            } else {
                reinstallKey();
            }
        } else {
            cout << "\nffmpeg is not installed or not allowed to run as root. Please install ffmpeg and allow it to run as root.";
        }
    }
}

void reinstallKey() {
    string cmd = "" + string(APP_NAME) + " -u";
    system(cmd.c_str());
    readConfig();
    string sk;
    cout << "\nInstalling " + string(APP_NAME) + "...";
    cout << "\nSecurity key: ";
    sk = inputText();
    instReInstComCode(sk);
}

void reinstall() {
    struct stat st;
    if (stat(configFile.c_str(), &st) == -1) {
        cout << "Configuration file not found. Install " + string(APP_NAME) + ".";
    } else {
        string sk = readConfigValue("system-id");
        string cmd = string(APP_NAME) + " -u";
        system(cmd.c_str());
        readConfig();
        instReInstComCode(sk);
    }
}

void instReInstComCode(string sk) {
    string content = (string) "<UpdateSystemInstallStatus xmlns='" + xmlnamespace + "'><SecurityKey>" + sk + "</SecurityKey><InstallStatus>1</InstallStatus></UpdateSystemInstallStatus>";
    string res = reqSOAPService("UpdateSystemInstallStatus", (xmlChar*) content.c_str());
    if ((int) res.find(">1<", 0) != -1) {
        cout << "Writing required files to file system.";
        if (0 == copyfile("init.conf", initFile))
            cout << "\n/etc " + initFile + " is created.";
        if (0 == copyfile("init.d", initdFile))
            cout << "\n/etc " + initdFile + " is created.";
        if (0 == copyfile("config.xml", configFile))
            cout << "\n/etc " + configFile + " is created.";
        if (0 == copyfile(string(APP_NAME), binFile)) {
            string modCmd = "chmod u+x " + binFile;
            system(modCmd.c_str());
        }
        cout << "\nEnabling " + string(APP_NAME) + " to run as root...";
        string slc = "ln -s " + binFile + " " + rootBinLnk;
        system(slc.c_str());
        cout << "\n" + rootBinLnk + " is created.";
        if (0 == copyfile("error.log", logFile))
            cout << "\n" + logFile + " is created.";
        struct stat st = {0};
        if (stat(recordsFolder.c_str(), &st) == -1) {
            if (0 == mkdir(recordsFolder.c_str(), 0774))cout << "\n" + recordsFolder + " is created";
        }
        writeConfigValue("system-id", sk);
        securityKey = sk;
        content = "<AddSystem xmlns=\"" + xmlnamespace + "\"><SystemSecurityKey>" + securityKey + "</SystemSecurityKey><SystemName>" + getMachineName() + "</SystemName><IPAddress /></AddSystem>";
        res = reqSOAPService("AddSystem", (xmlChar*) content.c_str());
        xmlInitParser();
        xmlDoc* xd = xmlParseDoc((xmlChar*) res.c_str());
        xmlXPathContext *xc = xmlXPathNewContext(xd);
        xmlXPathObject* xo = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='AddSystemResult']", xc);
        xmlNode* node;
        if (xo->nodesetval->nodeNr > 0) {
            node = xo->nodesetval->nodeTab[0];
            string resultmsg = string((char*) xmlNodeGetContent(node->children));
            if (resultmsg.compare("System Updated Successfully.") == 0) {
                videoStreamingType = string((char*) xmlNodeGetContent(node->children->next));
                autoInsertCameras = string((char*) xmlNodeGetContent(node->children->next->next));
                pollInterval = string((char*) xmlNodeGetContent(node->children->next->next->next));
                writeConfigValue("videoStreamingType", videoStreamingType);
                writeConfigValue("autoInsertCameras", autoInsertCameras);
                writeConfigValue("pollInterval", pollInterval);
                xmlCleanupParser();
                cout << "\n" + string(APP_NAME) + " installed successfully :D"
                        "\nDo u wanna start " + string(APP_NAME) + " now? [Y/n]: ";
                string input = inputText();
                cout << "\n";
                if (input.length() == 0 || tolower(input).compare("y") == 0) {
                    string cmd = "start " + string(APP_NAME);
                    system(cmd.c_str());
                } else if (tolower(input).compare("n") == 0) {
                    cout << string(APP_NAME) + " will start at next system startup. To change configuration run '" + string(APP_NAME) + " -c'\n";
                }
            } else {
                cout << "\nSorry some one booked the system while u r choosing! Try again.\n";
            }
        } else {
            cout << "\n" + res;
            cout << "\nQuandary :( Please contact administrator.\n";
        }
    } else {
        cout << "\n" + res;
        cout << "\nQuandary :( Please contact administrator.\n";
    }
}

void configure() {
    readConfig();
    cout << "\nCurrent " + string(APP_NAME) + " configuration:\n----------------------------";
    cout << "\napp-name:\t" + appName;
    cout << "\nserver-addr:\t" + serverAddr;
    cout << "\nserver-port:\t" + serverPort;
    cout << "\nstream-addr:\t" + streamAddr;
    cout << "\nstream-port:\t" + streamPort;
    cout << "\nnamespace:\t" + xmlnamespace;
    cout << "\ncamcapture-compression:\t" + readConfigValue("camcapture-compression");
    cout << "\nrecord-fps:\t" + recordfps;
    cout << "\nrecord-resolution:\t" + recordResolution;
    cout << "\nstream-fps:\t" + streamfps;
    cout << "\nstream-resolution:\t" + streamResolution;
    cout << "\nbootup:\t" + readConfigValue("bootup");
    cout << "\nsystem-id:\t" + systemId;
    cout << "\nvideoStreamingType:\t" + videoStreamingType;
    cout << "\nautoInsertCameras:\t" + autoInsertCameras;
    cout << "\npollInterval:\t" + pollInterval;
    cout << "\nmanage-network:\t" + string(itoa(manageNetwork));
    cout << "\nreconnect-poll-count:\t" + string(itoa(reconnectPollCount));
    cout << "\ninternet-test-url:\t" + internetTestURL;
    cout << "\nmobile-broadband-connection:\t" + mobileBroadbandCon;
    cout << "\ncorporate-network-gateway:\t" + corpNWGW;
    cout << "\ngps-device:\t" + gpsDevice;
    cout << "\ngps-sdevice-baudrate:\t" + gpsSDeviceBaudrate;
    cout << "\ndebug:\t" + string(itoa(debug));
    cout << "\n-------------------------\nSet or Add configuration property\n";
    string pn;
    string val;
    while (true) {
        cout << "\nProperty name:";
        pn = inputText();
        if (pn.length() != 0) {
            cout << "\nValue:";
            val = inputText();
            if (pn.compare("bootup") == 0) {
                if (geteuid() != 0) {
                    cout << "\nPlease login as root are sudo user.\n";
                } else {
                    struct stat st;
                    if (val.compare("true") == 0) {
                        if (stat(initFile.c_str(), &st) != -1) {
                            cout << "\nAllowing to run " + string(APP_NAME) + " at startup...";
                            if (stat(initOverrideFile.c_str(), &st) != -1) {
                                string rmfcmd = "rm " + initOverrideFile;
                                system(rmfcmd.c_str());
                            }
                            writeConfigValue(pn, val);
                            cout << "ok\n";
                        } else {
                            cout << "\nInstall " + string(APP_NAME) + " in first.\n";
                        }
                    } else if (val.compare("false") == 0) {
                        if (stat(initFile.c_str(), &st) != 1) {
                            cout << "\nDisabling " + string(APP_NAME) + " to run at startup...";
                            if (stat(initOverrideFile.c_str(), &st) == -1) {
                                int fd = open(initOverrideFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
                                string buf = "manual";
                                write(fd, buf.c_str(), buf.length());
                                close(fd);
                            }
                            writeConfigValue(pn, val);
                            cout << "ok\n";
                        } else {
                            cout << "\nInstall " + string(APP_NAME) + " in first.\n";
                        }
                    }
                }
            } else {
                writeConfigValue(pn, val);
            }
        } else {
            cout << "\n";
            fflush(stdout);
            break;
        }
    }
}

void secondFork() {
    secondChild = fork();
    if (secondChild != 0) {
        int status;
        wait(&status);
        secondFork();
    } else {
        secondChild = getpid();
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        run();
    }
}

void firstFork() {
    firstChild = fork();
    if (firstChild != 0) {
        int status;
        wait(&status);
        firstFork();
    } else {
        firstChild = getpid();
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        secondFork();
    }
}

int log(string prefix, string msg) {
    int fd = open(logFile.c_str(), O_WRONLY | O_APPEND);
    struct tm * timeinfo;
    char tb[20];
    time(&currentTime);
    timeinfo = localtime(&currentTime);
    strftime(tb, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
    string ts = string(tb);
    string log = ts + " " + prefix + ": " + msg;
    write(fd, log.c_str(), log.length());
    close(fd);
}

void signalHandler(int signal_number) {
    if (signal_number == SIGUSR1) {
        int fd;
        void* file_memory;
        string fn = "/var/" + string(APP_NAME) + ".data";
        fd = open(fn.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
        file_memory = mmap(0, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        munmap(file_memory, 1024);
    }
    if (signal_number == SIGTERM || signal_number == SIGINT) {
        if (getpid() == rootProcess) {
            cerr << "\n" + getTime() + " " + string(APP_NAME) + " terminated by " + string(itoa(signal_number)) + " number.\n";
        }
    }
    if (signal_number == SIGCHLD) {
        if (runMode.compare("daemon") == 0) {
            if (secondChild == getpid()) {

            } else if (firstChild == getpid()) {
                cerr << "\n" + getTime() + " derror: secondchild exited.\n";
            } else if (rootProcess == getpid()) {
                cerr << "\n" + getTime() + " derror: first child exited.\n";
            }
        }
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, NULL, WNOHANG | WCONTINUED)) > 0) {
            if ((debug & 1) == 1) {
                cout << "\n" + getTime() + " child exited. pid:" + std::string(itoa(pid)) + "\n";
                fflush(stdout);
            }
        }
        child_exit_status = status;
    }
    pkilled = true;
    fflush(stdout);
    fflush(stderr);
}

void stopRunningProcess() {
    if (runningProcess > 0) {
        cout << "\nStopping current process.....";
        if (kill(runningProcess, SIGTERM) != -1) {
            cout << "OK\n";
        } else {
            cout << "FAILED\n";
        }
    }
}

void update() {
    struct stat st = {0};
    string cmd;
    if (stat(srcFolder.c_str(), &st) == -1) {
        chdir("/usr/local/src");
        cmd = "git clone git://github.com/bulbmaker/" + string(APP_NAME) + ".git";
        system(cmd.c_str());
    } else {
        chdir(srcFolder.c_str());
        cout << "\nGit: pulling from online repostiory...";
        fflush(stdout);
        cmd = "git pull";
        system(cmd.c_str());
    }
    chdir(srcFolder.c_str());
    cout << "\nbuilding " + string(APP_NAME) + "...";
    cmd = "make clean";
    system(cmd.c_str());
    cmd = "make";
    system(cmd.c_str());
    cout << "Spawning " + string(APP_NAME) + "...";
    cmd = "./" + string(APP_NAME) + " -r";
    spawn(cmd, false, NULL, true);
}

void* gpsLocationUpdater(void* arg) {
    char buf[48];
    char c;
    int i = 0;
    string cmd;
    if (gpsSDeviceBaudrate.length() > 0) {
        cmd = "stty -F " + gpsDevice + " " + gpsSDeviceBaudrate;
        spawn gpsdbrsetter = spawn(cmd, false, NULL, false, true);
        if ((debug & 1) == 1) {
            cout << "\n" + getTime() + " setting baudrate:cmd:" + cmd + "\n";
        }
    }
    FILE *f = fopen(gpsDevice.c_str(), "r");
    if ((debug & 1) == 1) {
        cout << "\n" + getTime() + " reading gpsdevice:" + gpsDevice + "\n";
    }
    while (f) {
        c = (char) getc(f);
        if (c == '@') {
            time(&gpsReadStart);
            i = 0;
            c = getc(f);
            while (c != '#') {
                buf[i] = c;
                i++;
                c = getc(f);
            }
            time(&gpsReadEnd);
            buf[i] = '\0';
            gpsCoordinates = gpsDevice + ":" + string(buf);
        }
    }
    if ((debug & 1) == 1) {
        cout << "\n" + getTime() + " no gps device found. gpsLocationUpdater exiting.\n";
    }
}

void test() {
    char ipAddr[16];
    cout << GetPrimaryIp();
    fflush(stdout);
}

void* networkManager(void* arg) {
    if ((debug & 1) == 1) {
        cout << "\n" + getTime() + " network manager started.\n";
        fflush(stdout);
    }
    time_t presentCheckTime;
    time_t previousCheckTime;
    time_t waitInterval = reconnectDuration;
    spawn *wvdial;
    while (true) {
        previousCheckTime = presentCheckTime;
        time(&presentCheckTime);
        sleep((int) waitInterval);
        if (waitInterval = presentCheckTime - previousCheckTime >= reconnectDuration) {
            waitInterval = reconnectDuration;
            if (poke(internetTestURL) != 0) {
                sleep(5);
                if (mobileBroadbandCon.length() > 0) {
                    if (mobileBroadbandCon.compare("wvdial") == 0) {
                        if (wvdial) {
                            kill(wvdial->cpid, SIGTERM);
                            if ((debug & 1) == 1) {
                                char wvdialerr[100];
                                read(wvdial->cpstderr, wvdialerr, 100);
                                cout << "\n" + getTime() + " networkManager: wvdial->exitcode=" + string(itoa(wvdial->getChildExitStatus())) + ",wvdialerr->error=" + string(wvdialerr) + ". sleeping 10 seconds...\n";
                                fflush(stdout);
                            }
                            waitpid(wvdial->cpid, NULL, WUNTRACED);
                            delete wvdial;
                            wvdial = new spawn("wvdial", true, NULL, true, false);
                        } else {
                            wvdial = new spawn("wvdial", true, NULL, true, false);
                            if ((debug & 1) == 1) {
                                cout << "\n" + getTime() + " networkManager: wvdialed for the 1st time.\n";
                                fflush(stdout);
                            }
                        }
                    } else {
                        spawn *ifup = new spawn("nmcli con up id " + mobileBroadbandCon + " --timeout 30", false, NULL, false, true);
                        if (ifup->getChildExitStatus() != 0) {
                            if ((debug & 1) == 1) {
                                char ifuperr[100];
                                read(ifup->cpstderr, ifuperr, 100);
                                cout << "\n" + getTime() + " networkManager: ifup->exitcode=" + string(itoa(ifup->getChildExitStatus())) + ",ifup->error=" + string(ifuperr) + ". sleeping 10 seconds...\n";
                                fflush(stdout);
                            }
                            sleep(30);
                            spawn *ifup2 = new spawn("nmcli con up id " + mobileBroadbandCon, false, NULL, false, true);
                            if (ifup2->getChildExitStatus() != 0) {
                                if ((debug & 1) == 1) {
                                    cout << "\n" + getTime() + " nmcli stderror:";
                                    char buf[100];
                                    read(ifup->cpstderr, buf, 100);
                                    printf("%s", buf);
                                    cout << ". Exitcode=" + string(itoa(ifup2->getChildExitStatus())) + ".\n";
                                    fflush(stdout);
                                }
                                if ((debug & 1) == 1) {
                                    cout << "\n" + getTime() + " networkManager: disabling wwan." + "\n";
                                    fflush(stdout);
                                }
                                spawn *disableCon = new spawn("nmcli nm wwan off", false, NULL, false, true);
                                sleep(10);
                                if ((debug & 1) == 1) {
                                    cout << "\n" + getTime() + " networkManager: enabling wwan." + "\n";
                                    fflush(stdout);
                                }
                                spawn *enableCon = new spawn("nmcli nm wwan on", false, NULL, false, true);
                                delete enableCon;
                                delete disableCon;
                            } else {
                                if ((debug & 1) == 1) {
                                    cout << "\n" + getTime() + " nmcli:";
                                    char buf[200];
                                    read(ifup->cpstdout, buf, 200);
                                    printf("%s", buf);
                                    cout << "\n";
                                    fflush(stdout);
                                }
                            }
                            delete ifup2;
                        } else {
                            if ((debug & 1) == 1) {
                                cout << "\n" + getTime() + " networkManager: ifup: connected." + "\n";
                                fflush(stdout);
                            }
                        }
                        delete ifup;
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    ferr = open(logFile.c_str(), O_WRONLY | O_APPEND);
    stdinfd = dup(0);
    stdoutfd = dup(1);
    stderrfd = dup(2);
    runningProcess = atoi(readConfigValue("pid").c_str());
    struct sigaction signalaction_struct;
    memset(&signalaction_struct, 0, sizeof (signalaction_struct));
    signalaction_struct.sa_handler = &signalHandler;
    sigaction(SIGCHLD, &signalaction_struct, NULL);
    sigaction(SIGUSR1, &signalaction_struct, NULL);
    int next_option;
    const char* const short_options = "dhs:ircuxkp:t";
    string opt;
    const struct option long_options[] = {
        {"update", 0, NULL, 'd'},
        {"help", 0, NULL, 'h'},
        {"start", 1, NULL, 's'},
        {"install", 0, NULL, 'i'},
        {"reinstall", 0, NULL, 'r'},
        {"configure", 0, NULL, 'c'},
        {"uninstall", 0, NULL, 'u'},
        {"stop", 0, NULL, 'x'},
        {"keyInstall", 0, NULL, 'k'},
        {"test", 0, NULL, 't'},
        {NULL, 0, NULL, 0}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
        switch (next_option) {
            case 'h':
                print_usage(stdout, 0, argv[0]);
                break;
            case 's':
                stopRunningProcess();
                rootProcess = getpid();
                opt = string(optarg);
                if (opt.compare("daemon") == 0) {
                    runMode = opt;
                    close(1);
                    dup2(ferr, 2);
                    stderrfd = ferr;
                    firstFork();
                } else if (opt.compare("normal") == 0) {
                    run();
                }
                break;
            case 'i':
                stopRunningProcess();
                install();
                break;
            case 'r':
                reinstall();
                break;
            case 'c':
                configure();
                break;
            case 'u':
                uninstall();
                break;
            case 'x':
                stopRunningProcess();
                break;
            case 'd':
                update();
                break;
            case 'k':
                reinstallKey();
                break;
            case 't':
                test();
                break;
            case '?':
                print_usage(stderr, 1, argv[0]);
                break;
            case -1:
                print_usage(stderr, 1, argv[0]);
                break;
        }
        break;
    } while (next_option != -1);
    return 0;
}

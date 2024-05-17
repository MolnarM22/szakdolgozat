#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include "gps.hpp"

#define DEBUG 1
#define DEBUG_GPS_GPGGA 1

using namespace std;

mutex gpsDataMutex; // Mutex a GPS adatok védelmére

GPSDataClass gpsdata; // Globális változó a GPS adatok tárolására

/**
 * Egy stringet darabol fel a megadott elválasztó alapján.
 *
 * @param s A bemeneti string.
 * @param delimiter Az elválasztó karakter.
 * @return A darabolt stringek listája.
 */
vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    stringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token); 
    }
    return tokens;
}

/**
 * Visszaadja az aktuális GPS adatokat.
 *
 * @return Az aktuális GPS adatokat tartalmazó objektum.
 */
GPSDataClass returnGPSData() {
    static GPSDataClass gpsdata_prev;
    if (gpsDataMutex.try_lock()) {
        gpsdata_prev = gpsdata; 
        gpsDataMutex.unlock();
    } else {
        cout << "Nem sikerült megszerezni a GPS adatokat" << endl;
        return gpsdata_prev;
    }
    return gpsdata;
}

/**
 * Feldolgozza a nyers GPS adatokat és frissíti a globális GPS adat változót.
 *
 * @param rawGPSData A nyers GPS adatok string formátumban.
 */
void processGPSData(string rawGPSData) {
    gpsDataMutex.lock(); 
    string delimiter = "\n";
    string delimiter1 = "$GPGGA";
    try {
        string GPGGA = rawGPSData.substr(rawGPSData.find(delimiter1));
        string line = GPGGA.substr(0, GPGGA.find(delimiter));
        vector<string> parts = split(line, ','); 

        try {
            gpsdata.type = parts[0];
            gpsdata.time = parts[1];
            gpsdata.lat = parts[2];
            gpsdata.lat_dir = parts[3];
            gpsdata.lon = parts[4];
            gpsdata.lon_dir = parts[5];
            gpsdata.quality = parts[6];
            gpsdata.sats = parts[7];
            gpsdata.hdop = parts[8];
            gpsdata.alt = parts[9];
            gpsdata.aunits = parts[10];
            gpsdata.undulation = parts[11];
            gpsdata.uunits = parts[12];
            gpsdata.age = parts[13];
            gpsdata.checksum = parts[14];
        } catch (...) {
            cout << "Valami hiba történt" << endl;
        }
    } catch(...) {
        cout << "Nem jó a GPS jel" << endl;
    }
    gpsDataMutex.unlock(); 
}

struct termios tty;
struct termios tty_old;
int serial_port;

/**
 * Inicializálja a GPS-t a soros porton keresztül.
 *
 * @return True, ha a GPS inicializálása sikeres, egyébként false.
 */
bool gpsInit() {
    serial_port = open("/dev/ttyUSB0", O_RDWR);

    if (serial_port == -1) {
        cerr << "Hiba történt a soros port megnyitásakor." << endl;
        return false;
    }

    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_port, &tty) != 0) {
        cerr << "Hiba történt a soros port beállításainak lekérdezése közben." << endl;
        close(serial_port);
        return false;
    }

    tty_old = tty;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 15;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        cerr << "Hiba történt a soros port beállításainak beállításakor." << endl;
        close(serial_port);
        return false;
    }
    return true;
}

/**
 * A GPS kommunikáció.
 *
 * @return 0 ha minden jó.
 */
int gpsCom() {
    if (DEBUG_GPS_GPGGA) {
        while (true) {
            // Teszt adat, ha nincs valós GPS adat elérhető
            string test = "$GPGLL,4604.8462,N,01812.8055,E,192020.000,A,A*5D\n$GPGSA,A,3,21,02,28,17,,,,,,,,,7.54,3.51,6.68*06\n$GPGSV,2,1,05,02,68,152,18,21,51,144,18,17,37,291,30,28,27,071,16*76\n$GPGSV,2,2,05,04,,,20*7A\n$GPRMC,192020.000,A,4604.8462,N,01812.8055,E,0.33,33.30,060424,,,A*5D\n$GPVTG,33.30,T,,M,0.33,N,0.61,K,A*09\n$GPGGA,192021.000,4604.8460,N,01812.8055,E,1,4,1.74,125.1,M,40.3,M,,*5D\n";
            processGPSData(test); 
            sleep(1); 
            printf("Fut");
        }
    }
    char read_buffer[1024];
    int bytes_read = 0;
    string rawData;

    while (true) {
        bytes_read = read(serial_port, &read_buffer, sizeof(read_buffer)); 

        if (bytes_read > 0) {
            if (DEBUG == 1) {
                string s = string(read_buffer, bytes_read);
                rawData.append(s);
            }
        }

        if (rawData.length() > 365) {
            processGPSData(rawData);
            rawData.erase();
        }
    }

    tcsetattr(serial_port, TCSANOW, &tty_old); 
    close(serial_port); 
    return 0;
}


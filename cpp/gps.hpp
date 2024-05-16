#ifndef _gps_h
#define _gps_h
#include <string>

using namespace std;


class GPSDataClass {
  public: string type;
  string time;
  string lat;
  string lat_dir;
  string lon;
  string lon_dir;
  string quality;
  string sats;
  string hdop;
  string alt;
  string aunits;
  string undulation;
  string uunits;
  string age;
  string checksum;
};

extern GPSDataClass gpsdata;
GPSDataClass returnGPSData();

int gpsCom();
bool gpsInit();
#endif

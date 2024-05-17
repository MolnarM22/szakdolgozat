#ifndef _gps_h
#define _gps_h
#include <string>

using namespace std;

/**
 * A GPS adatokat tároló osztály.
 */
class GPSDataClass {
public:
    string type;      /*!< Az üzenet típusa. */
    string time;      /*!< Az idő adata. symbol: hhmmss.ss*/
    string lat;       /*!< A szélességi fok. */
    string lat_dir;   /*!< A szélességi fok iránya (N/S). */
    string lon;       /*!< A hosszúsági fok. */
    string lon_dir;   /*!< A hosszúsági fok iránya (E/W). */
    string quality;   /*!< A minőség mutatója. symbol: */
    string sats;      /*!< Látott műholdak száma. symbol: x*/
    string hdop;      /*!< Vízszintes pontosság (HDOP). symbol: x.x*/
    string alt;       /*!< Tengerszint feletti magasság. symbol: x.x*/
    string aunits;    /*!< Magasság mértékegysége. symbol: M*/
    string undulation;/*!< Geoid unduláció. symbol: x.x*/
    string uunits;    /*!< Unduláció mértékegysége. symbol: M */
    string age;       /*!< Adat kora (másodpercben). Maximum: 99 lehet*/
    string checksum;  /*!< Az üzenet ellenőrző összege. pl: *xx */
};

extern GPSDataClass gpsdata; /*!< Globális változó a GPS adatok tárolására. */

/**
 * Visszaadja az aktuális GPS adatokat.
 *
 * @return Az aktuális GPS adatokat tartalmazó objektum.
 */
GPSDataClass returnGPSData();

/**
 * A GPS kommunikáció.
 *
 * @return 0 ha minden jó.
 */
int gpsCom();

/**
 * Inicializálja a GPS-t a soros porton keresztül.
 *
 * @return True, ha a GPS init sikeres, egyébként false.
 */
bool gpsInit();

#endif


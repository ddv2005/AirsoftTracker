/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *	Adam M Rivera
 *	With direction from: Andrew Tridgell, Jason Short, Justin Beech
 *
 *	Adapted from: http://www.societyofrobots.com/robotforum/index.php?topic=11855.0
 *	Scott Ferguson
 *	scottfromscott@gmail.com
 *
 */
#include "AP_Declination.h"

#include <cmath>
#include <inttypes.h>

/*
  calculate magnetic field intensity and orientation
*/
bool AP_Declination::get_mag_field_ef(float latitude_deg, float longitude_deg, float &declination_deg)
{
    bool valid_input_data = true;

    /* round down to nearest sampling resolution */
    int32_t min_lat = static_cast<int32_t>(static_cast<int32_t>(latitude_deg / SAMPLING_RES) * SAMPLING_RES);
    int32_t min_lon = static_cast<int32_t>(static_cast<int32_t>(longitude_deg / SAMPLING_RES) * SAMPLING_RES);

    /* for the rare case of hitting the bounds exactly
     * the rounding logic wouldn't fit, so enforce it.
     */

    /* limit to table bounds - required for maxima even when table spans full globe range */
    if (latitude_deg <= SAMPLING_MIN_LAT) {
        min_lat = static_cast<int32_t>(SAMPLING_MIN_LAT);
        valid_input_data = false;
    }

    if (latitude_deg >= SAMPLING_MAX_LAT) {
        min_lat = static_cast<int32_t>(static_cast<int32_t>(latitude_deg / SAMPLING_RES) * SAMPLING_RES - SAMPLING_RES);
        valid_input_data = false;
    }

    if (longitude_deg <= SAMPLING_MIN_LON) {
        min_lon = static_cast<int32_t>(SAMPLING_MIN_LON);
        valid_input_data = false;
    }

    if (longitude_deg >= SAMPLING_MAX_LON) {
        min_lon = static_cast<int32_t>(static_cast<int32_t>(longitude_deg / SAMPLING_RES) * SAMPLING_RES - SAMPLING_RES);
        valid_input_data = false;
    }
    if(valid_input_data)
    {
        /* find index of nearest low sampling point */
        uint32_t min_lat_index = static_cast<uint32_t>((-(SAMPLING_MIN_LAT) + min_lat)  / SAMPLING_RES);
        uint32_t min_lon_index = static_cast<uint32_t>((-(SAMPLING_MIN_LON) + min_lon) / SAMPLING_RES);

        /* calculate declination */

        float data_sw = declination_table[min_lat_index][min_lon_index];
        float data_se = declination_table[min_lat_index][min_lon_index + 1];;
        float data_ne = declination_table[min_lat_index + 1][min_lon_index + 1];
        float data_nw = declination_table[min_lat_index + 1][min_lon_index];

        /* perform bilinear interpolation on the four grid corners */

        float data_min = ((longitude_deg - min_lon) / SAMPLING_RES) * (data_se - data_sw) + data_sw;
        float data_max = ((longitude_deg - min_lon) / SAMPLING_RES) * (data_ne - data_nw) + data_nw;

        declination_deg = ((latitude_deg - min_lat) / SAMPLING_RES) * (data_max - data_min) + data_min;
    }
    return valid_input_data;
}


/*
 calculate magnetic field intensity and orientation
*/
float AP_Declination::get_declination(float latitude_deg, float longitude_deg)
{
    float declination_deg=0;

    if(get_mag_field_ef(latitude_deg, longitude_deg, declination_deg))
        return declination_deg;
    else
        return 0;
}


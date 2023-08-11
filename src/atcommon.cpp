#include "atcommon.h"
#include <math.h>
#include <Arduino.h>

void convertDegreeToMeter(double lng, double lat, double &x,double &y)
{
    x = DEGREE_TO_METER_X(lng);
    y = DEGREE_TO_METER_Y(lat);
}

void convertMeterToDegree(double x, double y, double &lng,double &lat)
{
    lng = DEGREE_TO_METER_REVERSE_X(x);
    lat = DEGREE_TO_METER_REVERSE_Y(y);
}

double distanceXYBetween(double x1, double y1, double x2, double y2)
{
    return sqrt(pow(x2 - x1, 2) +pow(y2 - y1, 2));
}

double courseXYTo(double x1, double y1, double x2, double y2)
{
     double result = 270-degrees(atan2(y1 - y2, x1 - x2));
     if(result<0)
        result += 360;
    return result;
}

double distanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795;
}

double courseTo(double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

#pragma once

#define Pi M_PI

#define DEGREE_TO_METER_X(X) X*111319.490778
#define DEGREE_TO_METER_Y(Y) (log(tan((90.0 + (Y)) * M_PI / 360.0)) / (M_PI / 180.0))*111319.490778
#define DEGREE_TO_METER_REVERSE_Y(Y) (atan(pow(M_E, ((Y)/111319.490778)*M_PI/180.0))*360.0/M_PI-90.0)
#define DEGREE_TO_METER_REVERSE_X(X) X/111319.490778
void convertDegreeToMeter(double lng, double lat, double &x,double &y);
void convertMeterToDegree(double x, double y, double &lng,double &lat);
double distanceXYBetween(double x1, double y1, double x2, double y2);
double courseXYTo(double x1, double y1, double x2, double y2);
double distanceBetween(double lat1, double long1, double lat2, double long2);
double courseTo(double lat1, double long1, double lat2, double long2);

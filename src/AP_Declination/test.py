#!/usr/bin/env python3
'''
generate field tables from IGRF12. Note that this requires python3
'''

import igrf as igrf
import numpy as np
import datetime
from pathlib import Path
from pymavlink.rotmat import Vector3, Matrix3
import math

import argparse
parser = argparse.ArgumentParser(description='generate mag tables')
parser.add_argument('--sampling-res', type=int, default=5, help='sampling resolution, degrees')
parser.add_argument('--check-error', action='store_true', help='check max error')
parser.add_argument('--filename', type=str, default='tables.cpp', help='tables file')

args = parser.parse_args()

if not Path("AP_Declination.h").is_file():
    raise OSError("Please run this tool from the AP_Declination directory")


def write_table(f,name, table):
    '''write one table'''
    f.write("const float AP_Declination::%s[%u][%u] = {\n" %
                (name, NUM_LAT, NUM_LON))
    for i in range(NUM_LAT):
        f.write("    {")
        for j in range(NUM_LON):
            f.write("%.5ff" % table[i][j])
            if j != NUM_LON-1:
                f.write(",")
        f.write("}")
        if i != NUM_LAT-1:
            f.write(",")
        f.write("\n")
    f.write("};\n\n")

date = datetime.datetime.now()

SAMPLING_RES = args.sampling_res
SAMPLING_MIN_LAT = -90
SAMPLING_MAX_LAT = 90
SAMPLING_MIN_LON = -180
SAMPLING_MAX_LON = 180

lats = np.arange(SAMPLING_MIN_LAT, SAMPLING_MAX_LAT+SAMPLING_RES, SAMPLING_RES)
lons = np.arange(SAMPLING_MIN_LON, SAMPLING_MAX_LON+SAMPLING_RES, SAMPLING_RES)

NUM_LAT = lats.size
NUM_LON = lons.size

intensity_table = np.empty((NUM_LAT, NUM_LON))
inclination_table = np.empty((NUM_LAT, NUM_LON))
declination_table = np.empty((NUM_LAT, NUM_LON))

max_error = 0
max_error_pos = None
max_error_field = None

def get_igrf(lat, lon):
    '''return field as [declination_deg, inclination_deg, intensity_gauss]'''
    mag = igrf.igrf(date, glat=lat, glon=lon, alt_km=0., isv=0, itype=1)
    intensity = float(mag.total/1e5)
    inclination = float(mag.incl)
    declination = float(mag.decl)
    return [declination, inclination, intensity]


print("%f" % get_igrf(40.5795,-74.1502)[0])

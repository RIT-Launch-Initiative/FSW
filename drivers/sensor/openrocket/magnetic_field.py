from collections import namedtuple
# https://www.ngdc.noaa.gov/IAGA/vmod/igrf.html

mean_sea_level_km = 6371.2


Vec = namedtuple("Vec", ['x', 'y', 'z'])


def evaluate_xyz(r, latitude, longitude, t):
    return


def rotate(x, y, z, azimuth, altitude):

    # t in fractional years


def find_coefs(t: Float):
    t_after = t - 1900

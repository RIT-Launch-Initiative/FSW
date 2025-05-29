import marimo

__generated_with = "0.13.11"
app = marimo.App(width="medium")


@app.cell
def _():
    import marimo as mo
    return (mo,)


@app.cell
def _():
    from math import copysign, floor
    import pandas as pd
    import requests
    from importlib.abc import InspectLoader
    return InspectLoader, copysign, requests


@app.cell
def _(InspectLoader, requests):
    aaron_src = requests.get('https://raw.githubusercontent.com/RIT-Launch-Initiative/FSW/37800516edb6366e4db49044bc765761343bec6d/tools/fdp.py').content

    aaron = InspectLoader.source_to_code(aaron_src)

    return


@app.cell
def _(mo):
    mo.md(r"""# Configuration Values""")
    return


@app.cell
def _(mo):
    launchpad_lat_ui, launchpad_long_ui = mo.ui.number(value=43.1566, step=0.00001, label = "Latitude"), mo.ui.number(value=-77.6088, step=0.00001, label="Longitude")



    filepath = ''
    mo.vstack([mo.md("## Launchsite Location"),launchpad_lat_ui, launchpad_long_ui])
    return launchpad_lat_ui, launchpad_long_ui


@app.cell
def _(launchpad_lat_ui, launchpad_long_ui):
    launchpad_lat, launchpad_long = launchpad_lat_ui.value, launchpad_long_ui.value
    return launchpad_lat, launchpad_long


@app.cell
def _(copysign, launchpad_lat, launchpad_long):

    def recover_latlong(lat_frac, lon_frac):
        u16max = 65536
        lat_dec = lat_frac / u16max
        lon_dec = lon_frac / u16max

        lat = int(launchpad_lat) + copysign(lat_dec, launchpad_lat)
        lon = int(launchpad_long) + copysign(lon_dec, launchpad_long)
        return lat, lon
    return (recover_latlong,)


@app.cell
def _(launchpad_long):
    int(launchpad_long)
    return


@app.cell
def _(recover_latlong):
    recover_latlong(0, 65)
    return


@app.cell
def _():
    def BIT_MASK(n):
        return (1<<n)-1

    mask32 = BIT_MASK(32)

    alt_mask = BIT_MASK(14)
    speed_mask = BIT_MASK(10)
    state_mask = BIT_MASK(3)
    status_mask = BIT_MASK(2)
    qual_mask = BIT_MASK(3)

    sats_mask = BIT_MASK(4)
    millis_mask = BIT_MASK(27)

    def unpack_gps(u64):
        hi = (u64>>32) & mask32    

        alt = (hi >> 18) & alt_mask
        speed = (hi >> 8) & speed_mask
        state = (hi >> 5) & state_mask
        status= (hi >> 3) & status_mask
        quality = hi & qual_mask

        lo = u64 & mask32

        sats = (lo >> 28) & sats_mask
        millis = (lo >> 1) & millis_mask

        return alt, speed, state, status, quality, sats, millis
    return (unpack_gps,)


@app.cell
def _(unpack_gps):
    print(unpack_gps(369308414869516080))
    return


if __name__ == "__main__":
    app.run()

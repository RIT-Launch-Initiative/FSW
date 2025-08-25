import marimo

__generated_with = "0.13.11"
app = marimo.App(width="medium")


@app.cell
def _(i):
    i
    return


if __name__ == "__main__":
    app.run()

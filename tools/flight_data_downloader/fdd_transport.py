import os

class FDDTransport:
    __slots__ = ["output_folder"]

    def __init__(self):
        self.output_folder = os.getcwd()

    def set_output_folder(self, folder: str):
        self.output_folder = folder

    def _get_file(self, file: str) -> str:
        raise NotImplementedError("FDDTransport is an abstract class")

    def tree(self):
        print("\n" + self._get_file("tree"))

    def download(self, file: str):
        file_name = file.split("/")[-1]
        out_path = os.path.join(self.output_folder, file_name)

        with open(out_path, "w") as fout:
            fout.write(self._get_file(file))

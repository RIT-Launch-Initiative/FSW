import os


class FDDTransport:
    current_output_folder = os.getcwd()

    def __init__(self):
        print(self.current_output_folder)
        pass

    def _get_file(self, file: str) -> bytes:
        raise NotImplementedError("FDDTransport is an abstract class")

    def tree(self):
        print("\n" + self._get_file("tree"))

    def download(self, file: str):
        file_name = file.split("/")[-1]
        out_path = os.path.join(self.current_output_folder, file_name)

        with open(out_path, "w") as fout:
            fout.write(self._get_file(file))

    def set_output_folder(self, folder: str):
        self.current_output_folder = os.getcwd() + "/" + folder

        if not os.path.exists(folder):
            os.makedirs(folder)

    def set_attribute(self, args: list):
        raise NotImplementedError("FDDTransport is an abstract class")

import os


current_output_folder = os.getcwd()

class FDDTransport:

    def __init__(self):
        pass

    def _get_file(self, file: str) -> bytes:
        raise NotImplementedError("FDDTransport is an abstract class")

    def tree(self):
        print("\n" + self._get_file("tree"))

    def download(self, file: str):
        file_name = file.split("/")[-1]
        out_path = os.path.join(current_output_folder, file_name)

        with open(out_path, "w") as fout:
            contents = self._get_file(file)
            if contents is not None:
                fout.write(contents)
            else:
                print("Error downloading file")

    def set_output_folder(self, folder: str):
        global current_output_folder
        current_output_folder = os.getcwd() + "/" + folder

        if not os.path.exists(folder):
            os.makedirs(folder)

    def set_attribute(self, attribute, args: list):
        raise NotImplementedError("FDDTransport is an abstract class")

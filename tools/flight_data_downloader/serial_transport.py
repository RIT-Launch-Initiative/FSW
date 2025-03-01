from fdd_transport import FDDTransport

class SerialTransport(FDDTransport):
    def __init__(self):
        super().__init__()

    def _get_file(self, file: str) -> bytes:
        raise NotImplementedError("FDDTransport is an abstract class")

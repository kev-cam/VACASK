# Define our own exception
class RpnToVaError(Exception):
    def __init__(self, message, pos=None):
        if pos is not None:
            message = "At position "+str(pos)+": "+message
        super().__init__(message)

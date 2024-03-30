
class Element:
    def __init__(self, **kwargs):
        self.magnitud = kwargs.get("magnitud", None)
        self.ordinal = kwargs.get("ordinal", None)
        self.typeIo = kwargs.get("typeIo", None)
    def __str__(self):
        return f"{self.magnitud}-{self.ordinal}-{self.typeIo}"

class Client:
    def __init__(self, **kwargs):
        self.name = kwargs.get("Name", None)
        self.situation = kwargs.get("Situation", None)
        #max 10 elements
        self.elements = kwargs.get("Elements", [])
        self.mac = kwargs.get("MAC", None)
        self.localTCP = kwargs.get("Local-TCP", None)
        self.server = kwargs.get("Server", None)
        self.srvUDP = kwargs.get("Srv-UDP", None)
    
    def __str__(self):
        return f"""
            Name={self.name},
            Situation={self.situation},
            Elements={";".join([str(x) for x in self.elements])},
            MAC={self.mac},
            Local-TCP={self.localTCP},
            Server={self.server},
            Srv-UDP={self.srvUDP})
            """



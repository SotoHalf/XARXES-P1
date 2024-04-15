from enum import Enum
import common
import socket

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
        self.localTCP = int(kwargs.get("Local-TCP", None))
        self.server = kwargs.get("Server", None)
        self.srvUDP = int(kwargs.get("Srv-UDP", None))
        self.state = common.CLIENT_STATES_REVERSE.get("NOT_SUBSCRIBED",0xa0)

    def get_current_state(self):
        return common.CLIENT_STATES.get(self.state, self.state)

    def set_current_state(self,value):
        self.state = common.CLIENT_STATES_REVERSE.get(value, self.state)

    def get_name(self):
        return self.name
    
    def get_situation(self):
        return self.situation

    def get_elements(self):
        return ";".join([str(x) for x in self.elements])

    def get_mac(self):
        return self.mac

    def get_localTCP(self):
        return self.localTCP
    
    def __str__(self):
        return f"""
            Name={self.name},
            Situation={self.situation},
            Elements={self.get_elements()},
            MAC={self.mac},
            Local-TCP={self.localTCP},
            Server={self.server},
            Srv-UDP={self.srvUDP})
            
            State={self.state}
            """

# SOCKET PACKETS

class SocketType(Enum):
    UDP = 1
    TCP = 2

class SocketSetup:

    def __init__(self, sock_type: SocketType, client: Client):
        self.sock = None
        self.sock_type = sock_type
        self.destination = client.server

        if sock_type == SocketType.UDP:
            self.port = client.srvUDP
            self.setup_udp()
        elif sock_type == SocketType.TCP:
            self.port = client.localTCP
            self.setup_tcp()
        else:
            self.port = None

    def setup_udp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def sendto(self, s):
        print(s.encode())
        print((self.destination, self.port))
        self.sock.sendto(s.encode(),(self.destination, self.port))
    
    def setup_tcp(self):
        pass

    def close(self):
        self.sock.close()


class PDU: #UDP DATAGRAM
    def __init__(self, **kwargs):
        self.typePdu = common.PACKAGE_TYPE_SUB_REVERSE.get(kwargs.get("typePdu", None),0x00),
        self.typePduReversed = common.PACKAGE_TYPE_SUB.get(self.typePdu, "SUBS_REQ")

        if self.typePduReversed == "SUBS_REQ":
            self.setup_subs_req(kwargs)
        elif self.typePduReversed == "SUBS_ACK":
            pass
        elif self.typePduReversed == "SUBS_REJ":
            pass
        elif self.typePduReversed == "SUBS_INFO":
            self.setup_subs_info(kwargs)
        elif self.typePduReversed == "INFO_ACK":
            pass
        elif self.typePduReversed == "SUBS_NACK":
            pass

    def setup_subs_req(self,**kwargs):

        client = kwargs.get("client", None)

        self.mac = client.get_mac()
        self.random_num = "00000000" #doc says as random num "00000000"
        self.data = f"{client.get_name()},{client.get_situation()}"

    def setup_subs_info(self,**kwargs):

        client = kwargs.get("client", None)
        pdu_req = kwargs.get("pdu_req", None)


        self.mac = client.get_mac()
        self.random_num = pdu_req.random_num
        self.data = f"{client.get_tcp()},{client.get_elements()}"



    def __str__(self):
        return f"{self.typePduReversed}"

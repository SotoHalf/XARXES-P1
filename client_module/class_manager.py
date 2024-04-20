from enum import Enum
import common
import socket

class Element:
    def __init__(self, **kwargs):
        self.magnitud = kwargs.get("magnitud", None)
        self.ordinal = kwargs.get("ordinal", None)
        self.typeIo = kwargs.get("typeIo", None)
        self.value = None
    
    def set_value(self, value):
        self.value = value

    def get_stat(self):
        return f"{self.magnitud}-{self.ordinal}-{self.typeIo}\t {self.value}"

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
        self.state = common.CLIENT_STATES_REVERSE.get("NOT_SUBSCRIBED",0xa1)
        #server data
        self.random_num = "00000000"
        self.server_mac = None
        self.server_udp_port = None
        self.server_tcp_port = None

    def set_server_data(self, random_num:str, server_mac:str, server_udp_port:int):
        self.random_num = random_num
        self.server_mac = server_mac
        self.server_udp_port = server_udp_port
        #self.srvUDP = server_udp_port

    def check_info_ack_data(self, random_num:str, server_mac:str, server_tcp_port:int):
        if random_num != self.random_num: return False
        if server_mac != self.server_mac: return False
        self.server_tcp_port = server_tcp_port
        #self.localTCP = server_tcp_port
        return True

    def get_current_state(self):
        return common.CLIENT_STATES.get(self.state, self.state)

    def set_current_state(self,value):
        self.state = common.CLIENT_STATES_REVERSE.get(value, self.state)

    def get_server_mac(self):
        return self.server_mac

    def get_srvUDP(self):
        return self.srvUDP
    
    def get_localTCP(self):
        return self.localTCP
    
    def get_name(self):
        return self.name
    
    def get_situation(self):
        return self.situation

    def get_elements(self):
        return ";".join([str(x) for x in self.elements])

    def get_mac(self):
        return self.mac

    def get_random_num(self):
        return self.random_num
    
    def set_random_num(self, num):
        self.random_num = num
    

    def get_stat(self):
        elements_str = "\n".join([x.get_stat() for x in self.elements])
        return f"""
******************** DADES CONTROLADOR *********************
MAC: {self.get_mac()}, Nom: {self.get_name()}, Situació: {self.get_situation()} 

Estat: {self.get_current_state()}

    Dispos.    valor
    -------    ------
{elements_str}

************************************************************
"""
    
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
    
class PDU: #UDP DATAGRAM
    def __init__(self, **kwargs):

        self.mac = None
        self.random_num = None
        self.data = None
        self.reason = None
        self.port_tcp = None
        self.port_udp = None

                       
        self.typePdu = common.PACKAGE_TYPE_UDP_REVERSE.get(kwargs.get("typePdu", None),0x05)
        self.typePduReversed = common.PACKAGE_TYPE_UDP.get(self.typePdu, "SUBS_NACK")

        if self.typePduReversed == "SUBS_REQ":
            self.setup_subs_req(**kwargs)
        elif self.typePduReversed == "SUBS_INFO":
            self.setup_subs_info(**kwargs)
        elif self.typePduReversed == "SUBS_ACK":
            self.setup_subs_ack(**kwargs)
        elif self.typePduReversed == "SUBS_REJ":
            self.setup_subs_rej(**kwargs)
        elif self.typePduReversed == "INFO_ACK":
            self.setup_info_ack(**kwargs)
        elif self.typePduReversed == "SUBS_NACK":
            self.setup_subs_nack(**kwargs)
            #HELLO
        elif self.typePduReversed == "HELLO":
            self.setup_subs_hello(**kwargs)
        elif self.typePduReversed == "HELLO_REJ":
            self.setup_subs_hello_rej(**kwargs)

        
    def pdu_from_datagram(data_recived):

        if data_recived:
            #test
            print(data_recived)
            type_packet = common.PACKAGE_TYPE_UDP.get(data_recived[0],None)
            print(type_packet)
            if type_packet:
                return PDU(**{
                    "typePdu": type_packet,
                    "packet" : data_recived
                })
            
        return None
    
    def setup_subs_hello_rej(self,**kwargs):
        self.reason = "DATA DO NOT MATCH"
        client = kwargs.get("client", None)
        if client:
            self.mac = client.get_server_mac()
            self.random_num = client.get_random_num()
        else:
            self.mac = ''
            self.random_num = ''

    def setup_subs_hello(self,**kwargs):
        client = kwargs.get("client", None)
        if client:
            self.mac = client.get_server_mac()
            self.random_num = client.get_random_num()
            self.data = f"{client.get_name()},{client.get_situation()}"
        else:
            packet = kwargs.get("packet", None)
            self.mac = PDU.decode_bytes(packet,1,13) 
            self.random_num = PDU.decode_bytes(packet,13,22)
            self.data = PDU.decode_bytes(packet,22,102)   #Doc says Ip and then at the bottom port UDP


    def setup_subs_ack(self,**kwargs):
        packet = kwargs.get("packet", None)
        self.mac = PDU.decode_bytes(packet,1,13) 
        self.random_num = PDU.decode_bytes(packet,13,22)
        self.port_udp = PDU.decode_bytes(packet,22,102)   #Doc says Ip and then at the bottom port UDP
        
    def setup_subs_rej(self,**kwargs):
        packet = kwargs.get("packet", None)
        self.mac = PDU.decode_bytes(packet,1,13) 
        self.random_num = PDU.decode_bytes(packet,13,22)
        self.reason = PDU.decode_bytes(packet,22,102)

    def setup_info_ack(self,**kwargs):
        packet = kwargs.get("packet", None)
        self.mac = PDU.decode_bytes(packet,1,13) 
        self.random_num = PDU.decode_bytes(packet,14,22)
        self.port_tcp = PDU.decode_bytes(packet,22,102)
        
    def setup_subs_nack(self,**kwargs):
        self.reason = "ERROR NACK"

    def setup_subs_req(self,**kwargs):
        client = kwargs.get("client", None)
        if client:
            self.mac = client.get_mac()
            self.random_num = client.get_random_num()
            self.data = f"{client.get_name()},{client.get_situation()}"

    def setup_subs_info(self,**kwargs):

        client = kwargs.get("client", None)
        if client:
            self.mac = client.get_mac()
            self.random_num = client.get_random_num() #Get random from Client -> SUBS_REQ
            self.data = f"{client.get_localTCP()},{client.get_elements()}"

    def get_packet(self):
        #Create packet data to send
        if self.typePduReversed in ["SUBS_REQ","SUBS_INFO","HELLO"]:
            type_packet = self.typePdu.to_bytes(1, byteorder='big')
            packet = type_packet + PDU.encode_bytes(self.mac,13) + PDU.encode_bytes(self.random_num,9) + PDU.encode_bytes(self.data,80)
            return packet
        elif self.typePduReversed in ["HELLO_REJ"]:
            type_packet = self.typePdu.to_bytes(1, byteorder='big')
            packet = type_packet + PDU.encode_bytes(self.mac,13) + PDU.encode_bytes(self.random_num,9) + PDU.encode_bytes(self.reason,80)
            return packet
        
        return bytes([1])
    
    def get_attrs(self):
        if self.typePduReversed in ["SUBS_REQ","SUBS_INFO","HELLO"]:
            return {
                "pdu_type" : self.typePduReversed,
                "mac" : self.mac,
                "random_num" : self.random_num,
                "data" : self.data
            }
        elif self.typePduReversed == "SUBS_ACK":
            return {
                "pdu_type" : self.typePduReversed,
                "mac" : self.mac,
                "random_num" : self.random_num,
                "port_udp" : self.port_udp
            }
        elif self.typePduReversed == "INFO_ACK":
            return {
                "pdu_type" : self.typePduReversed,
                "mac" : self.mac,
                "random_num" : self.random_num,
                "port_tcp" : self.port_tcp
            }
        elif self.typePduReversed == "SUBS_REJ":
            return {
                "pdu_type" : self.typePduReversed,
                "mac" : self.mac,
                "random_num" : self.random_num,
                "reason" : self.reason
            }
        elif self.typePduReversed == "SUBS_NACK":
            return {
                "pdu_type" : self.typePduReversed,
                "reason" : self.reason
            }
        elif self.typePduReversed == "HELLO_REJ":
            return {
                "pdu_type" : self.typePduReversed,
                "reason" : self.reason
            }

        return {}

    #set the correct format of a package, max n fill left with missing n
    def encode_bytes(s,n):
        return s.encode()[:n]#.ljust(n, b'\x00')

    def decode_bytes(s,ini,fin):
        return s[ini:fin].decode('utf-8').rstrip('\x00')
    
    def check_hello_recived(client:Client, data_recived, data_sended):
        if not data_recived: return False
        if data_recived.get('mac',None) != client.get_server_mac(): return False
        if data_recived.get('random_num',None) != client.get_random_num(): return False
        if data_recived.get('data',None) != data_sended.get('data',None): return False
        return True

    def __str__(self):
        return f"{self.typePduReversed}"

class SocketType(Enum):
    UDP = 1
    TCP = 2

class SocketSetup:

    BufferSizeRecv = 8192

    def __init__(self, sock_type: SocketType, client: Client):
        self.sock = None
        self.sock_type = sock_type
        self.destination = client.server
        self.connected = False

        if self.sock_type == SocketType.UDP:
            self.port = client.get_srvUDP()
            self.setup_udp()
        elif self.sock_type == SocketType.TCP:
            self.port = client.get_localTCP()
            self.setup_tcp()
        else:
            self.port = None

    def get_connected(self):
        return self.connected
    
    def get_port(self):
        return self.port

    def setup_udp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.connected = True
    
    def setup_tcp(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.destination, self.port)) 
            self.connected = True
        except ConnectionRefusedError:
            self.connected = False
            

    def set_port(self,port:int):
        self.port = port

    def sendto(self, pdu: PDU, timeout = 1):
        if self.connected:
            self.sock.settimeout(timeout)
            packet = pdu.get_packet()

            if self.sock_type == SocketType.UDP:
                print(f"port enviat {self.port}")
                print(f"{packet}")
                self.sock.sendto(packet, (self.destination, self.port))
            elif self.sock_type == SocketType.TCP:
                self.sock.sendall(packet)
        else:
            if self.sock_type == SocketType.UDP:
                self.setup_udp()
            elif self.sock_type == SocketType.TCP:
                self.setup_udp()


    def recvData(self):
        recived_data = None
        try:
            if self.sock_type == SocketType.UDP:
                recived_data = self.sock.recv(SocketSetup.BufferSizeRecv)
            elif self.sock_type == SocketType.TCP:
                recived_data = self.sock.recv(SocketSetup.BufferSizeRecv)
        except socket.timeout:
            pass

        return recived_data
    
    def close(self):
        self.sock.close()



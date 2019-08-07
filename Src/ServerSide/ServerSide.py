import socket
import threading
import time
import subprocess
import queue
import hashlib
import random

ClientList = list()
FingerprintsDict = dict()
FingerprintsDict["INVALID"] = ""

class MessageClass:
    sender = str()
    message = bytes()
    timestamp = float()
    def __init__(self, sender, message, timestamp):
        self.sender = sender
        self.message = message
        self.timestamp = timestamp
    def GetSender(self):
        return sender
    def GetMessage(self):
        return message
    def GetTimestamp(self):
        return timestamp

class ClientClass:
    socket = socket.socket()
    nickname = str()
    fingerprint = bytes()
    jointimestamp = float()
    roomname = str()
    def __init__(self, socket, nickname, fingerprint, jointimestamp, roomname):
        self.socket = socket
        self.nickname = nickname
        self.fingerprint = fingerprint
        self.roomname = roomname
    def GetSocket(self):
        return socket
    def GetNickname(self):
        return nickname
    def GetFingerprint(self):
        return fingerprint
    def GetTimestamp(self):
        return jointimestamp
    def GetRoomName(self):
        return roomname


def CreateTORString(directory, HSP, socks = 0):
    return """SocksPort {}
HiddenServiceDir {}
HiddenServicePort {} {}:{}""".format(socks, directory, HSP[0], HSP[1][0], HSP[1][1])



def RunTOR(Port_Loopback = "7410", Port_Listening = "80", directory="/OR"):
    torrc_string = CreateTORString(socks = 0, directory = directory, HSP = (Port_Listening, ("127.0.0.1", Port_Loopback)))
    torrc_file = open("Tor/torrc", 'w')
    torrc_file.write( torrc_string )
    torrc_file.close()
    Process_TOR = subprocess.Popen("Tor/tor.exe -f Tor/torrc", stdout = subprocess.PIPE)
    #StartTime = time.time()
    while True:
        for output in Process_TOR.stdout:
            if b"100%: Done" in output:
                URL = open(directory + "/hostname", 'r').read()
                return URL
            
            elif b"[err]" in output:
                print("Tor Client exited with Error: {}".format(output.decode()))
            else:
                print("Tor Client output: {}".format(output.decode()))
    
def GetChunks(sock):
    Buffer = bytes()
    Result = str()
    while True:
        Buffer += sock.recv(256-len(Buffer))

        if(len(Buffer)<256):
            continue
        else:
            Result += Buffer[:255].decode()
            if(Buffer[256] == 0x01):
                Buffer = bytes()
                continue
            elif(Buffer[256] == 0x00):
                return Result
            else:
                return 0

def MessageHandler():       #TODO find a way to make sure that the ClientList is being updated consistantly
    global ClientList
    MessageBuffer = queue.Queue()
    while True:
        for client in ClientList:
            while ( MessageBuffer.queue[0].GetSender() == client.GetNickname() ):
                MessageBuffer.get()
            PushQueueToClient(client, MessageBuffer)
            RetrieveMessage(client, MessageBuffer)

def RetrieveMessage(Client, MessageQueue):
    message = MessageClass( Client.GetNickname(), ReceiveThroughChunks(Client, 256), time.time())
    MessageQueue.put(message)
    
    
def PushQueueToClient(Client, MessageQueue):
    for message in MessageQueue.queue():
        if message.GetTimestamp() >= Client.GetTimestamp():
            Buffer = message.GetMessage().encode() + b'\x00' +  message.GetSender().encode()
            SendThroughChunks(Client.GetSocket(), Buffer, 256)
        
    

def SendThroughChunks(socket, message, ChunkSize):
    bytesLeft = len(message)
    while (bytesLeft > 0):
        if (bytesLeft <= (ChunkSize-1)):     #Regard that 1 byte is always reserved (the last byte)
            buffer = message + b'\x00'
            socket.send(buffer)     #TODO make sure that all 256 bytes were successfuly sent over

        else:
            buffer = message + b'\x01'
            socket.send(buffer)
            message = message[256:]
            bytesLeft -= 255

def ReceiveThroughChunks(socket, ChunkSize):
    message = bytes()
    while True:
        buffer = socket.recv(ChunkSize) #TODO make sure that the exact amount is received
        
        if buffer[-1] not in (b'\x01' , b'\x00'):
            return None
        elif buffer[-1] == b'\x00':
            message += buffer[:255]
            return message
        else: #buffer[-1] == b'\x01'
            message += buffer[:255]
            continue
    


def KickClients(InputType, condition):
    global ClientList
    if InputType == "0":
        client = FindByNickname(condition)
        if not client:
            return False
        client.GetSocket().close()
        ClientList.remove(client)
        return True

    elif InputType == "1":
        clients = FindByRoomName(condition)
        for client in clients:
            client.GetSocket().close()
            ClientList.remove(client)
        return True
            

    else:
        return False


def FindByNickname(nickname):           #TODO reprogram the following two functions as individual threads in order to minimize the search time                                  
    global ClientList                                #OR simply create two dedicated lists :D
    for client in ClientList:
        if client.GetNickname() == nickname:
            return client
    return False

def FindByRoomName(roomname):
    global ClientList
    for client in ClientList:
        if client.GetRoomName() == roomname:
            yield client




def GetRoomName(Fingerprint):
    return FingerprintsDict.get(Fingerprint)


def  VarifyFingerprint(Fingerprint):
    global FingerprintsDict
    return FingerprintsDict.get(Fingerprint)

def ValidateNickname(nickname):
    if (not FindByNickname(nickname)) and nickname:
        return True
    else:
        return False

def GrantGreets(sock, fingerprint):
    sock.send("O".encode() + GetRoomName(fingerprint).encode() + bytes(1))
    return


def DenyGreets(sock):
    sock.send("N".encode() + bytes(255))
    return



def ClientGreeting(sock):
    Buffer = bytes()
    Result = str()
    while True:
        Buffer += sock.recv(256-len(Buffer))
        if(len(Buffer)<256):
            continue
        else:
            break
    if(Buffer[:64].find(0x00) < 0):    #Message is corrupted
        DenyGreets(sock)
        return
    nickname = Buffer[:Buffer.find(0x00)]
    fingerprint = Buffer[64:255]
    if(ValidateNickname(nickname) and VarifyFingerprint(fingerprint)):
        GrantGreets(sock, fingerprint)
        global FingerprintsDict
        ClientList.append( ClientClass(sock, nickname.decode(), fingerprint, time.time(), FingerprintsDict.get(fingerprint) ) )
        return
    else:
        DenyGreets(sock)
        return

def ListenerThread():
    sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    sock.bind(("127.0.0.1",7410))
    sock.listen()
    while True:
        a,b = sock.accept()
        threading._start_new_thread(ClientGreeting, tuple(a))

def ManagementThread():
    while True:
        command = input("1.Managing rooms\n2.Managing Users\n3.Statistics\n")
        if command not in ('1','2','3'):
            print("Invalid Option")
            continue
        elif command == '1':
            global FingerprintsDict
            for key in FingerprintsDict.keys():
                print("{} : {}".format(key, FingerprintsDict.get(key)))
            command = input("1.Add Room\n2.Remove Room\n3.Change Room Key\n")
            if command not in ('1','2','3'):
                print("Invalid Option")
                continue
            elif command == '1':
                RoomName = input("Room Name?\t")    #TODO mention the maximum room name size
                if FingerprintsDict.get(RoomName):
                    print("Room already exists")
                    continue
                Fingerprint = ""
                while Fingerprint in FingerprintsDict.values(): #We can replace it with a do while loop in C
                    salt = str( int( random.random() * (10 ** 3) ) )
                    Fingerprint = hashlib.sha512( RoomName.encode() + chr(salt[0]) + chr(salt[1]) + chr(salt[2]) ).hexdigest()
                FingerprintsDict[RoomName] = Fingerprint
                print("Room was created successfully\nFingerprint : {}".format(Fingerprint))
                continue
            elif command == '2':
                RoomName = input("Room Name?\t")
                if RoomName not in FingerprintsDict.keys():
                    print("Such a room does not exist")
                    continue
                KickClients(1, FingerprintsDict.get(RoomName) )      #I must create a list for each room indicating its members. That way
                                                                                                #it'll be easier to kick related clients
                FingerprintsDict.pop(RoomName)
            elif command == '3':
                RoomName = input("Room Name?\t")
                if RoomName not in FingerprintsDict.keys():
                    print("Such a room does not exist")
                    continue
                Fingerprint = FingerprintsDict.get(RoomName)
                while Fingerprint in FingerprintsDict.values(): #We can replace it with a do while loop in C
                    salt = str( int( random.random() * (10 ** 3) ) )
                    Fingerprint = hashlib.sha512( RoomName.encode() + chr(salt[0]) + chr(salt[1]) + chr(salt[2]) ).hexdigest()
                FingerprintsDict[RoomName] = Fingerprint

        elif command == '2':
            global ClientList
            for client in ClientList:
                print( "{}.{} Joined {} at {} with the fingerprint {} which is still {}".format(ClientList.index(client), client.GetNickname(), client.GetRoomName(),
                                                                                            client.GetTimestamp(), client.GetNickname(), client.GetFingerprint(),
                                                                                             (lambda x: "Valid" if x else "Invalid")(VarifyFingerprint(client.GetFingerprint()) ) ) ) #Just a simple Lambda
            command = input("1.Kick User\n")
            if command not in ("1"):
                print("Invalid Option")
                continue
            elif command == "1":
                ClientIndex = input("Enter the client index")
                try:
                    ClientIndex = int(ClientIndex)
                    command = input("Confirm the order. Client nickname : {}".format( ClientList[ClientIndex].GetNickname() ) )
                    if command.lower() == "y":
                        KickClients(0, ClientList[ClientIndex].GetNickname() )
                except:
                    print("Invalid user index")
                    continue
                    

        elif command == '3':
            print("To be implemented in future")
        

if __name__=="__main__":
    
    RunTOR()
    threading._start_new_thread(MessageHandler, tuple())
    threading._start_new_thread(ListenerThread, tuple())
    ManagementThread()

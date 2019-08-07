#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <netinet/in.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


/**********Data Types**********/

enum SocketFamily {IPv4=AF_INET, IPv6= AF_INET6};
enum SocketType {TCP= SOCK_STREAM, UDP= SOCK_DGRAM};

struct SocksSock { SOCKET SOCK; struct sockaddr SocksServer; WSADATA wsadata; };
struct ClientSession { struct SocksSock* SocksConn; char* ServerDomain; char* Fingerprint; char* RoomName; }; //TODO put a size attribute for each of these strings and avoid using \0 bytes as "end of string"
struct AddressFormat { char* IP; SHORT PORT; enum SocketFamily Family; };


/**********Socket Layer Functions**********/

int _Initialize(WSADATA wsadata); //Returns 0 For no errors
int _ConnectSimplified(SOCKET sock, struct AddressFormat* address);
int _Connect(SOCKET sock, struct sockaddr* addr);
int _SendData(SOCKET sock, char* BUFFER, int BUFFER_SIZE, int flags); //Returns 0 For no errors
int _RecvData(SOCKET sock, char* BUFFER, int SIZE, int flags); //Returns 0 For no errors
int _FetchData(SOCKET sock, char** BUFFER, int* SIZE, int flags);
void _CleanSock(WSADATA* wsadata, SOCKET sock);


/**********Session Layer Functions**********/

struct SocksSock* EstablishSocks(struct sockaddr* SocksAddress, int ADDRSIZE, char* DOMAIN, int DOMAINSIZE, char* PORT); //Returns 0 For no errors & PORT is 2bytes long
int SendWSocks(struct SocksSock* sock, char* BUFFER, int SIZE);
int FetchDataWSocks(struct SocksSock* sock, char* BUFFER, int SIZE);


/**********Presentation Layer Functions**********/

struct ClientSession* InitializeServerConn(struct SocksSock* socks, char* DOMAIN, int DOMAINSIZE, char* Fingerprint); // Returns NULL if there was connection issue
int MessageHandler(struct ClientSession* session, BOOL SendOrNot, char* PotentialMessage, int PotentialMessageSize, char** ReceivedMessage, int* ReceivedMessageSize); //Returns: 0 For nothing, 1 for message received, 2 for message sent and 3 for both
void RoomHandler(struct ClientSession session);	//Runs MessageHandler() on backend and manages the frontend


/**********Memory Management Functions**********/

void* AllocateMemory(int SIZE); //This func is implemented here to make it possible for future advancements
void FreeMemory(void* POINTER, int SIZE, BOOL IsPointer); //To both freeing the allocated memory and deleting the existing data. In case of SIZE=0 the data on memory is left untouched


/**********Privacy Related Functions**********/

void die();


/**********Multi Processing Functions**********/

int EstablishTOR(int PORT);
int GetTORState(int ProcessHandle);
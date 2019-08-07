#include "WinsockSocks.h"




int __cdecl main(int argc, char** argv)
{
	struct sockaddr_in sss;
	sss.sin_family = AF_INET;
	sss.sin_port = htons(9050);
	
	if (inet_pton(AF_INET, "127.0.0.1", &sss.sin_addr) <= 0)
	{
		printf("Brah");
			return 1;
	}
	
	struct SocksSock *abc= EstablishSocks((struct sockaddr*)&sss, sizeof(sss), "icanhazip.com", 13, "\0P");

	if (abc == NULL)
	{
		printf("Crap\n");
		return -1;
	}
	else
	{
		printf("GOOD");
		
		/*
		int stat = SendWSocks(abc, "GET / HTTP/1.1\r\nHost: icanhazip.com\r\n\r\n", 40);
		if (stat != 0) {
			printf("Couldnt send");
		}
		char* B = AllocateMemory(2048);
		stat = FetchDataWSocks(abc, B, 2048);
		if (stat<0){
			printf("Couldnt recv");
			return 1;
		}
		printf("%s",B);*/
	}

	struct ClientSession *Session = InitializeServerConn(abc, argv[1], 16, argv[2]);

	if (Session == NULL)
	{
		printf("Crap2\n");
		return -1;
	}
	else
	{
		printf("GOOD2");
		
	}

	RoomHandler(*Session);




	return 0;
}

void* AllocateMemory(int SIZE)
{
	return malloc(SIZE);
}

void FreeMemory(void* POINTER, int SIZE, BOOL IsPointer)
{
	if (SIZE) {
		memset(POINTER, 0, SIZE);
	}
	if (IsPointer) {
		free(POINTER);
	}
}


int GenerateTorrc(int PORT)
{
	char *torrc = AllocateMemory(16);
	sprintf(torrc, "SocksPort %d", PORT);
	return torrc;
}

int EstablishTOR(int PORT)
{
	int stat = 0;
	char* torrcString;
	FILE *TorFILE;
	FILE *TorHandler;

	TorFILE = fopen("Tor/tor.exe", 'r');
	if(TorFILE == NULL)		//Checking if tor.exe Exists
	{
		return -1;
	}

	torrcString = GenerateTorrc(PORT);
	TorFILE = fopen("Tor/torrc", 'w');
	if(TorFILE == NULL)
	{
		return -1;
	}
	fputs(torrcString, TorFILE);
	fclose(TorFILE);


	TorHandler = _popen("Tor/tor.exe -f Tor/torrc", 'r');
	if(TorHandler == NULL)		//Checking if the tor instance was initiated
	{
		return -1;
	}

	char* Buffer = AllocateMemory(2048);
	while(1)		//TODO setup a time out for this loop
	{
		fgets(Buffer, 2047, TorHandler);
		if(strstr(Buffer, "100%%") != NULL)		//Tor is running
		{
			return 0;
		}
		else if(strstr(Buffer, "[err]") != NULL)
		{
			return 1;
		}
		else continue;
	}
	


}


int _Initialize(WSADATA wsadata)	//TODO Fix this shit
{
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		return 1;		//Failed to initialize WSA
	}
	else {
		return 0;		//WSA was successfully initialized
	}
}

int _ConnectSimplified(SOCKET sock, struct AddressFormat* address_ip)
{
	int status = 0;
	int ADDRSIZE = sizeof(struct sockaddr_in);
	struct sockaddr_in* addr = (struct sockaddr_in*)AllocateMemory(ADDRSIZE);
	addr->sin_family = address_ip->Family;
	addr->sin_port = address_ip->PORT;
	status = inet_pton(address_ip->Family, address_ip->IP, &(addr->sin_addr));
	if (status <= 0) {
		FreeMemory(addr, ADDRSIZE, 1);
		return 2; //Invalid Address
	}
	status = connect(sock, (struct sockaddr*)addr, ADDRSIZE);
	if(status==0){
		return 0; //Connection successful
	}
	else {
		return 1; //Connection Error
	}
}


int _Connect(SOCKET sock, struct sockaddr* addr) //TODO set sockets to be non-blocking
{
	int status = 0;
	int ADDRSIZE = sizeof(struct sockaddr_in);
	status = connect(sock, addr, ADDRSIZE);
	if (status == 0) {
		return 0; //Connection successful
	}
	else {
		return 1; //Connection Error
	}
}

int _RecvData(SOCKET sock, char* BUFFER, int SIZE, int flags)
{
	return recv(sock, BUFFER, SIZE, flags);
}


int _SendData(SOCKET sock, char* BUFFER, int BUFFER_SIZE, int Flags) //TODO cleanup BytesSent Variable
{
	int BytesSent = 0;
	BytesSent = send(sock, BUFFER, BUFFER_SIZE, Flags);
	if (BytesSent == SOCKET_ERROR) {
		return SOCKET_ERROR; //Error occured
	}
	else if (BytesSent < BUFFER_SIZE) {	//To make sure that the whole buffer is sent
		return _SendData(sock, BUFFER + BytesSent, BUFFER_SIZE - BytesSent, Flags);
	}
	else {
		return 0;
	}
}

void _CleanSock(WSADATA* wsadata, SOCKET sock)
{
	closesocket(sock);
	WSACleanup();
	FreeMemory(wsadata, sizeof(WSADATA), 1);
	FreeMemory(&sock, sizeof(SOCKET), 0);
}

struct SocksSock* EstablishSocks(struct sockaddr* SocksAddress, int ADDRSIZE, char* DOMAIN, int DOMAINSIZE, char* PORT)
{

	char *BUFFER = NULL, *Temp = NULL;
	int status = 0;
	SOCKET *sock = AllocateMemory(sizeof(SOCKET));

	WSADATA wsa;
	status = _Initialize(wsa);
	if (status != 0) {
		printf("wsa Creation");
		return NULL;
	}
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0) {
		printf("Sock Creation");
		return NULL;
	}

	/*status = ioctlsocket(*sock, FIONBIO, &(1)); //Setting non-blocking socket io
	if (status != NO_ERROR) {
		printf("Sock options");
		return NULL;
	}*/

	status = _Connect(*sock, SocksAddress);
	if (status != 0) {
		printf("Connetion");
		return NULL;
	}

	/*Stage 1*/
	BUFFER = AllocateMemory(3);
	BUFFER = (char[]){ 0x05	//Socks Version
					, 0x01	//Number of supported auth methods
					, 0x00	//Auth method (no authentication)
			};

	status = _SendData(*sock, BUFFER, 3, 0);
	if (status != 0) {
		return NULL;
	}
	FreeMemory(BUFFER, sizeof(*BUFFER), 1);

	/*Stage 2*/
	BUFFER = AllocateMemory(2);
	status = _RecvData(*sock, BUFFER, 2, 0);
	if (status <= 0) {
		printf("recv1");
		return NULL;
	}
	FreeMemory(BUFFER, sizeof(*BUFFER), 1);

	/*Stage 3*/
	BUFFER = AllocateMemory(7 + DOMAINSIZE);
	Temp = AllocateMemory(5);
	Temp = (char[]) { 0x05	//Socks Version
					, 0x01	//Command Code (TCP stream)
					, 0x00	//Reserved
					, 0x03	//Address Type (Domain)
					, DOMAINSIZE	//Domain size
	};
	memcpy(BUFFER, Temp, 5);
	memcpy(BUFFER + 5, DOMAIN, DOMAINSIZE);
	BUFFER[5 + DOMAINSIZE] = PORT[0];	//Port number is 2 bytes long
	BUFFER[6 + DOMAINSIZE] = PORT[1];	//Port number (0x50 = 80)
	for(int i=0;i<7 + DOMAINSIZE;i++)printf("%d ",BUFFER[i]);

	status = _SendData(*sock, BUFFER, 7 + DOMAINSIZE, 0);
	if (status != 0) {
		return NULL;
	}
	FreeMemory(BUFFER, sizeof(*BUFFER), 1);
	FreeMemory(Temp, sizeof(*Temp), 1);

	/*Stage 4*/
	BUFFER = AllocateMemory(10);
	status = _RecvData(*sock, BUFFER, 10, 0);
	if (status <= 0) {
		return NULL;
	}
	if (BUFFER[1] != 0) {
		printf("Server Issue\n%d",BUFFER[1]);
		return NULL;
	}
	FreeMemory(BUFFER, sizeof(*BUFFER), 1);

	struct SocksSock* Socks = AllocateMemory(sizeof(struct SocksSock));
	Socks->SOCK = *sock;
	FreeMemory(sock, sizeof(*sock), 1);
	Socks->SocksServer = *SocksAddress;
	return Socks;

	//TODO checking whether AllocateMemory returns NULL or not


}

int SendWSocks(struct SocksSock* sock, char* BUFFER, int SIZE)
{
	int status = _SendData(sock->SOCK, BUFFER, SIZE, 0);
	if (status != 0) {
		return -1;
	}
	else{
		return 0;
	}
}

int FetchDataWSocks(struct SocksSock* sock, char* BUFFER, int SIZE)
{
	int status = _RecvData(sock->SOCK, BUFFER, SIZE, 0);
	if (status < 0) {
		return -1;
	}
	else {
		return status; //Returns number of characters received
	}

}


struct ClientSession* InitializeServerConn(struct SocksSock* socks, char* DOMAIN, int DOMAINSIZE, char* Fingerprint)
{
	char* FingerPrintBuffer = AllocateMemory(191);
	char* Nickname = AllocateMemory(64);
	printf("What Should you be called?(63 characters max)\t");
	scanf("%s", Nickname); //TODO find a better way to do this
	memcpy(FingerPrintBuffer, Fingerprint, 191);
	
	
	{
		char* BUFFER = AllocateMemory(256);
		memset(BUFFER, '\0', 256);

		memcpy(BUFFER, Nickname, strlen(Nickname));
		BUFFER[strlen(Nickname)] = '\0';

		memcpy(BUFFER + 64, FingerPrintBuffer, strlen(FingerPrintBuffer));

		BUFFER[255] = 0x00; //End of request

		SendWSocks(socks, BUFFER, 256);

		FreeMemory(BUFFER, 256, 1);
	}

	{
		int stat = 0, rcvd = 0;
		char* BUFFER = AllocateMemory(256);
		
		while (1)
		{
			stat = FetchDataWSocks(socks, BUFFER + rcvd, 256 - rcvd);
			if (stat > 0){	rcvd += stat;	}
			if (stat < 0){	return NULL;	}
			
			else if (rcvd < 256){	continue;	}
			else
			{
				break;			//At current state, the introduction request can't be bigger than a single chunk(256 bytes)
								//TODO remove this limit
			}
		}

		if (BUFFER[0] == 'N') {	//Server has refused to serve. Dying...
			return NULL;	//TODO make these returns more meaningful so i later i can check if it was server issue or network error
		}
		else if (BUFFER[0] == 'O')	//Server has accepted the connection. client and server should coordinate with each other here
		{
			char* ServerDomain = AllocateMemory(DOMAINSIZE); //These two lines are here because i wasnt sure about the function parameters(i mean DOMAIN) lifespan
			memcpy(ServerDomain, DOMAIN, DOMAINSIZE);

			char* RoomName = AllocateMemory(strlen(BUFFER + 1)); //The room name comes right after the first character('O' i mean)
			memcpy(RoomName, BUFFER + 1, strlen(BUFFER + 1));

			struct ClientSession* initializedConn = AllocateMemory(sizeof(struct ClientSession));
			initializedConn->SocksConn = socks;
			initializedConn->ServerDomain = ServerDomain;
			initializedConn->Fingerprint = FingerPrintBuffer;
			initializedConn->RoomName = RoomName;
		
			FreeMemory(BUFFER, 256, 1);
			
			return initializedConn;
		}

	


		else {
			return NULL;
		}
	}
}

int MessageHandler(struct ClientSession* session, BOOL SendOrNot, char* PotentialMessage, int PotentialMessageSize, char** ReceivedMessage, int* ReceivedMessageSize)
{
	char* BUFFER = NULL;
	int stat = 0;

	if (SendOrNot && PotentialMessageSize > 0)
	{ 											//The current version sends unencrypted message to TOR through local loopback and leaves encryption to it
												//but another more reliable method is required on further developments
		int CharsLeftToSend = PotentialMessageSize;
		int CharsSent = 0; //This variable is provided for more simplification
		BUFFER = AllocateMemory(256);
		
		while(CharsLeftToSend!=0)
		{
			memset(BUFFER, '\0', 256);
			if(CharsLeftToSend<=255)
			{
				memcpy(BUFFER, CharsSent + PotentialMessage, CharsLeftToSend);
				BUFFER[255] = 0x00; //The message was completely sent
				stat = SendWSocks(session->SocksConn, BUFFER, 256);
				if(stat != 0)
				{
					printf("Error occured while sending\n");
				}
				else
				{
					CharsSent += CharsLeftToSend;
					CharsLeftToSend -= CharsLeftToSend; //Decreasing number of characters queued to send from the whole left message
				}
			}
			else
			{
				memcpy(BUFFER, CharsSent + PotentialMessage, 255);
				BUFFER[255] = 0x01; //The message was partialy sent
				stat = SendWSocks(session->SocksConn, BUFFER, 256);
				if(stat != 0)
				{
					printf("Error occured while sending\n");
				}
				else
				{
					CharsSent += 255;
					CharsLeftToSend -= 255; //Decreasing number of characters queued to send from the whole left message

				}

			}
		}

		FreeMemory(BUFFER, sizeof(*BUFFER), 1);

	}



	if( ReceivedMessage != NULL && ReceivedMessageSize != NULL )	//TODO send a 
	{	//Here comes the message receiver
		stat = 0;

		int BytesRecvd = 0;
		int BytesWritten = 0;
		char *FinalMessage = AllocateMemory(256);
		char *TEMP1 = NULL, *TEMP2 = NULL;
		memset(FinalMessage, '\0', 1024);
		
		BUFFER = AllocateMemory(256);
		while(1)
		{
			memset(BUFFER, '\0', 256);

			stat = FetchDataWSocks(session->SocksConn, BUFFER, 256 - BytesRecvd);

			if(stat > 0)BytesRecvd += stat;

			if(stat<0)
			{
				printf("Error occured while receiving\n");
				continue;
			}
			
			else if(stat < 256 && BytesRecvd < 256)
			{											//CHALLENGE if the message was incompletely received (in a chunk < 256 bytes) we need to receive the rest
														//and link it to them but how? or how to avoid it? (SOLVED)
				
				memcpy(FinalMessage + BytesWritten, BUFFER, stat);
				BytesWritten += stat;					//There is a way to avoid BytesWritten variable and use BytesRecvd but that just makes things more complicated
														//TODO Anyway i'm looking forward to refactore it in future		
				
				continue;
			}

			else
			{
				memcpy(FinalMessage + BytesWritten, BUFFER, stat);	//All 256 bytes were received and copied to FinalMessage
				BytesWritten += stat;

				BytesRecvd = 0;

				if(FinalMessage[ sizeof(*FinalMessage)-1 ] == 0x01)			//Message has a following chunk
				{
					TEMP1 = AllocateMemory(sizeof(*FinalMessage) + 256);		//The following 5 lines allocate another bigger portion of memory, copy the previously
					memcpy(TEMP1, FinalMessage, sizeof(*FinalMessage));		//received chunks on it and reasign FinalMessage to point to it.
					TEMP2 = FinalMessage;
					FinalMessage = TEMP1;
					TEMP1 = NULL;
					FreeMemory(TEMP2, sizeof(*TEMP2), 1);

				}

				else if(FinalMessage[ sizeof(*FinalMessage)-1 ] == 0x00)		//Message was completely received
				{
					TEMP1 = AllocateMemory( sizeof(*FinalMessage) - sizeof(*FinalMessage)/256 );
					for(int counter = 0; counter < sizeof(*FinalMessage)/256; counter++)		//This loop deletes meaningless parts of received chunks from FinalMessage
					{
						memcpy(TEMP1 + counter*255, FinalMessage + 256*counter, 255);
					}

					TEMP2 = FinalMessage;									//Again, it's just swapping FinalMessage with TEMP1 and then freeing old FinalMessage
					FinalMessage = TEMP1;
					TEMP1 = NULL;
					FreeMemory(TEMP2, sizeof(*TEMP2), 1);


					*ReceivedMessage = FinalMessage;						//TODO Note that the function doesn't remove the empty part of the last chunk, so we
					*ReceivedMessageSize = sizeof(*FinalMessage);			//probably need to remove the rudundant part manually

					FreeMemory(BUFFER, sizeof(*BUFFER), 1);
					FreeMemory(FinalMessage, sizeof(*FinalMessage), 1);


					return 0;
				}

				else														//Corrupted message was received
				{
					FreeMemory(BUFFER, sizeof(*BUFFER), 1);
					FreeMemory(FinalMessage, sizeof(*FinalMessage), 1);
					return -1;
				}
			}
		}
	}

	else
	{
		return -1; 															//No option was provided at all
	}
	
}


void RoomHandler(struct ClientSession session)
{
	char* Message;
	char* ReceivedMessage;
	char* NicknameDelimiter;
	char Command;
	int stat = 0, SizeOfRcvd;
	while(1)
	{
		scanf("%c", &Command);

		switch(Command)
		{
			case 'S':
				scanf("%s", Message);
				MessageHandler(&session, 1, Message, strlen(Message), NULL, NULL);
			
			default:
				stat = MessageHandler(&session, 0, NULL, 0, &ReceivedMessage, &SizeOfRcvd);
				if(stat == -1)
				{
					printf("Message was received corruptedly\n");
				}
				else
				{
					NicknameDelimiter = strchr(ReceivedMessage, (int)'\0');
					printf("<%s>:%s\n", ReceivedMessage, NicknameDelimiter + 1);
				}

		}


	}

}
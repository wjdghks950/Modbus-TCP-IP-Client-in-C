#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IPADDR 30
#define RESPLEN 100

int receiveParams(int* startAddr, int* numCoils);
int readCoils(int sockfd, char* request, char* response, size_t len);
void PrintHexa(char* buff, size_t len);
int writeMultipleCoils(int sockfd, char* request, char* response, size_t len);
int readHoldingRegisters(int sockfd, char* request, char* response, size_t len);
int writeMultipleRegisters(int sockfd, char* request, char* response, size_t len);

int main(int argc, char* argv[])
{
    if (argc < 2){
        fprintf(stderr, "[ Provide Modbus-TCP_server_IP_address ]\n");
        exit(1);
    }
    int sockfd = 0;
    int portNum = 502; // Modbus server port number
    int rconnect = 0;
    int funcNum = 0;
    int startAddr = 0;
    int numCoils = 0;
    int len = 0;
    int i = 0;
    int numBytes = 0;

    int* values = NULL;
    char* request = NULL;
    char* response = (char*)malloc(RESPLEN);
    char serverip[IPADDR] = {0};
    memset(serverip, 0, IPADDR);
    strncpy(serverip, argv[1], strlen(argv[1]));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNum);
    inet_aton(serverip, &(serv_addr.sin_addr));

    unsigned char slave_addr;
    unsigned char func_code;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){ // Create a socket
        perror("socket error\n");
        exit(1);
    }
    else{
        printf("Socket created...\n");
    }
    rconnect = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if( rconnect != 0 ){
        perror("connection to the server: failed\n");
        exit(0);
    }
    else{
        printf("Connected to server at [ %s | port: %d ]\n", serverip, portNum);
        printf("---------------------------------\n");
        printf("\n[ Parameters ]\n");
        printf("Start Address: [ %d ]\n", startAddr);
        printf("Number of coils to read: [ %d ]\n", numCoils);
        printf("IP address of slave: [ %s ]\n", serverip);
        printf("---------------------------------\n");
        while(1){
            funcNum = receiveParams(&startAddr, &numCoils);
            switch (funcNum)
            {
                case 1:
                    len = sizeof(char) * 12;
                    request = (char*)malloc(len); //Modbus TCP ADU = 12 Bytes
                    memset(request, 0, len);
                    printf("Read coil...\n");
                    slave_addr = 0x01;
                    func_code = 0x01;
                    request[1] = 0x00;
                    request[5] = 0x06; // Length from here to the end of the frame(Unit ID~end of Data)
                    memcpy(&request[6], &slave_addr, sizeof(unsigned char));
                    memcpy(&request[7], &func_code, sizeof(unsigned char));
                    memcpy(&request[9], &startAddr, sizeof(unsigned char));
                    memcpy(&request[11], &numCoils, sizeof(unsigned char));
                    readCoils(sockfd, request, response, len);
                    break;
                case 2:
                    printf("Write multiple coils...\n");
                    i = 0, numBytes = 0;
                    int modulus = numCoils / 8;
                    numBytes = (int)ceil((float)numCoils / 8.0);
                    len = sizeof(char) * (12 + 1 + numBytes); // number of data bytes + numBytes
                    request = (char*)malloc(len);
                    memset(request, 0, len);
                    
                    int total_bit_len = 8 * numBytes;
                    int availBits = 8 - (total_bit_len - numCoils); // number of space holders

                    values = (int*)malloc(sizeof(int)*numBytes);
                    memset(values, 0, sizeof(int)*numBytes);
                    printf("numBytes: %d\n", numBytes);
                    for (i=0; i<numBytes; i++){
                        if (modulus != 0){
                            if (i < numBytes-1){
                                printf("[ Enter %dth coil idx between 0 and 255 ] : ", i+1);
                            }
                            else{
                                printf("[ Enter %dth coil idx between 0 and %d ] : ", i+1, (int)pow(2.0, (double)availBits)-1);
                            }
                            scanf("%d", &values[i]);
                            memcpy(&request[12+i+1], &values[i], sizeof(unsigned char));
                            PrintHexa(request, len);
                        }
                    }
                    slave_addr = 0x01;
                    func_code = 0x0F;
                    request[1] = 0x00;
                    request[5] = 0x06 + 0x01 + numBytes;
                    memcpy(&request[6], &slave_addr, sizeof(unsigned char));
                    memcpy(&request[7], &func_code, sizeof(unsigned char));
                    memcpy(&request[9], &startAddr, sizeof(unsigned char));
                    memcpy(&request[11], &numCoils, sizeof(unsigned char));
                    memcpy(&request[12], &numBytes, sizeof(unsigned char));// the number of data bytes to follow
                    writeMultipleCoils(sockfd, request, response, len);
                    break;
                case 3:
                    printf("Reading holding registers...\n");
                    len = sizeof(char) * 12;
                    request = (char*)malloc(len);
                    memset(request, 0, len);
                    slave_addr = 0x01;
                    func_code = 0x03;
                    request[1] = 0x00;
                    memcpy(&request[6], &slave_addr, sizeof(unsigned char));
                    memcpy(&request[7], &func_code, sizeof(unsigned char));
                    memcpy(&request[9], &startAddr, sizeof(unsigned char));
                    memcpy(&request[11], &numCoils, sizeof(unsigned char));
                    readHoldingRegisters(sockfd, request, response, len);
                    break;
                case 4:
                    printf("Writing multiple holding registers...\n");
                    i = 0, numBytes = 0;
                    numBytes = numCoils*2; // 'numCoils' registers * 2 bytes (each)
                    len = sizeof(char) * (12 + 1 + numBytes);
                    request = (char*)malloc(len);
                    slave_addr = 0x01;
                    func_code = 0x10; // 16 in dec
                    memcpy(&request[6], &slave_addr, sizeof(unsigned char));
                    memcpy(&request[7], &func_code, sizeof(unsigned char));
                    memcpy(&request[9], &startAddr, sizeof(unsigned char));
                    memcpy(&request[11], &numCoils, sizeof(unsigned char));

                    values = (int*)malloc(sizeof(int)*numBytes);
                    memset(values, 0, sizeof(int)*numBytes);
                    for (i=0; i<numCoils; i++){
                        printf("[ Enter %dth value to write to register ] : ", i+1);
                        scanf("%d", &values[i]);
                        memcpy(&request[12+((i+1)*2)], &values[i], sizeof(unsigned char));
                        PrintHexa(request, len);
                    }
                    memcpy(&request[12], &numBytes, sizeof(unsigned char));
                    writeMultipleRegisters(sockfd, request, response, len);
                    break;
                case 100:
                    printf("Socket Closing...\n");
                    close(sockfd);
                    return 0;
            }
        }
    }

    return 0;
}

int receiveParams(int* startAddr, int* numCoils){
    int funcNum = 0;
    printf("Which function do you want?\n");
    printf("[1]: Read Coils\n[2]: Write multiple Coils\n[3]: Read Holding Registers\n[4]: Write multiple (Holding) Registers\n[100]: Quit\n");
    printf("Select a function [1, 2, 3, 4, 100]: ");
    scanf("%d", &funcNum);
    if (funcNum == 100){
        printf("Terminating ModBus TCP client...\n");
        return funcNum;
    }
    printf("Enter the Start Address: ");
    scanf("%d", startAddr);
    printf("Enter the number of coils to be read: ");
    scanf("%d", numCoils);
    return funcNum;
}


int readCoils(int sockfd, char* request, char* response, size_t len){
    int readlen = 0, i = 0;
    if (send(sockfd, request, len, 0) < 0 ){
        perror("send() failed!\n");
        exit(0);
    }
    else{
        printf("Query sent to Modbus TCP server!\n");
        printf("[ request message ]\n");
        PrintHexa(request, len);
    }

    readlen = recv(sockfd, (char*)response, RESPLEN, 0);
    printf("Received length: %d\n", readlen);
    printf("\n[ received message (in HEX) ]\n");
    PrintHexa(response, readlen);
    int funcCode = (int)response[7];
    int bytesFollow = (int)response[8];
    printf("------------------------------\n");
    printf("[Function code(HEX)] : %02X (%d in DEC)\n", funcCode, funcCode);
    printf("[Following Bytes(HEX)] : %02X (%d in DEC)\n", bytesFollow, bytesFollow);
    printf("[Coil status(HEX)] : ");
    for (i=0; i<bytesFollow; i++){
        printf("%02X ", response[9+i]);
    }
    printf("\n");
    printf("------------------------------\n");
    printf("\n");

    return readlen;
}

int writeMultipleCoils(int sockfd, char* request, char*response, size_t len){
    int readlen = 0;
    if (send(sockfd, request, len, 0) < 0){
        perror("send() failed!\n");
        exit(0);
    }
    else{
        printf("Writing Multiple Coils on the Modbus TCP server!\n");
        printf("[ request message ] \n");
        PrintHexa(request, len);
    }

    readlen = recv(sockfd, (char*)response, RESPLEN, 0);
    printf("Received length: %d\n", readlen);

    printf("\n[ received message ]\n");
    PrintHexa(response, readlen);
    int funcCode = (int)response[7];
    int coilStart = (int)response[9];
    int coilWritten = (int)response[11];
    printf("------------------------------\n");
    printf("[Function code(HEX)] : %02X (%d in DEC)\n", funcCode, funcCode);
    printf("[Starting Coil(HEX)] : %02X (%d in DEC)\n", coilStart, coilStart);
    printf("[Coils Written(HEX)] : %02X (%d in DEC)\n", coilWritten, coilWritten);
    printf("------------------------------\n");
    printf("\n");
    return readlen;
}
int readHoldingRegisters(int sockfd, char* request, char* response, size_t len){
    int readlen = 0, i = 0;
    if (send(sockfd, request, len, 0) < 0){
        perror("send() failed!\n");
        exit(0);
    }
    else{
        printf("Read Holding Register from the Modbus TCP server!\n");
        printf("[ request message ] \n");
        PrintHexa(request, len);
    }

    readlen = recv(sockfd, (char*)response, RESPLEN, 0);
    printf("Received length: %d\n", readlen);
    printf("\n[ received message ]\n");
    PrintHexa(response, readlen);
    int funcCode = (int)response[7];
    int numBytes = (int)response[8] / 2;
    printf("------------------------------\n");
    printf("[Function code(HEX)] : %02X (%d in DEC)\n", funcCode, funcCode);
    printf("[Following Bytes(HEX)] : %02X (%d in DEC)\n", numBytes, numBytes);
    for (i=0; i<numBytes; i++){
        printf("[%dth register content(HEX)] : %02X%02X\n", i+1, response[9+(i*2)], response[9+(i*2)+1]);
    }
    printf("------------------------------\n");
    printf("\n");
    return readlen;
}
int writeMultipleRegisters(int sockfd, char* request, char* response, size_t len){
    int readlen = 0;
    if (send(sockfd, request, len, 0) < 0){
        perror("send() failed!\n");
        exit(0);
    }
    else{
        printf("Writing Multiple Registers on the Modbus TCP server!\n");
        printf("[ request message ] \n");
        PrintHexa(request, len);
    }

    readlen = recv(sockfd, (char*)response, RESPLEN, 0);
    printf("Received length: %d\n", readlen);
    PrintHexa(response, readlen);
    int funcCode = (int)response[7];
    int regStart = (int)response[9];
    int regWritten = (int)response[11];
    printf("------------------------------\n");
    printf("[Function code(HEX)] : %02X (%d in DEC)\n", funcCode, funcCode);
    printf("[Starting Register(HEX)] : %02X (%d in DEC)\n", regStart, regStart);
    printf("[Registers Written(HEX)] : %02X (%d in DEC)\n", regWritten, regWritten);
    printf("------------------------------\n");
    printf("\n");
    return readlen;
}

void PrintHexa(char* buff, size_t len){
    size_t i;
    for(i = 0; i < len; i++){
        printf("%02X ", (unsigned char)buff[i]); 
    }
    printf("\n");
}


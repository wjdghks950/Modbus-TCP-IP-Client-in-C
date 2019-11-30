# Modbus TCP/IP Client(Master) Program

This is a Modbus TCP/IP client(master) program implementation in C.

# Requirements

Download EasyModbusTCPServer.jar from 
    https://sourceforge.net/projects/easymodbustcp-udp-java/

You will need either gcc or a C compiler to compile and run this program.

# Compile

    gcc ModTCPclient.c -o modTCPClient -lm

# Run

    ./modTCPClient 127.0.0.1

You can enter the IP address of the ModbusTCP server you want to send your requests to.

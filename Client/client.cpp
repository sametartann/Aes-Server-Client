// Client.cpp
#include <iostream>
#include <string>
#include <conio.h> // For _kbhit
#include <thread> // For multithreading
#include "aes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <random>

// AES key for encryption and decryption
static const unsigned char aes_key[] = {
    'k', 'e', 'y', '1', '2', '3', '4', '5', '6', '7', '8', '9',  'a', 'b', 'c', 'd'
};

// Initialization vector for AES with null values
static unsigned char iv[16] = { 0 };

class ClientSocketConn {
public:
    ClientSocketConn(std::string serverIP, int serverPort) {
        this->serverIP = serverIP;
        portNumber = serverPort;
        initialize();
    }

    // Function to set the socket to non-blocking mode
    void setNonBlocking(int sockfd) {
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sockfd, FIONBIO, &mode);
#else
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
    }

    // Function to check if there is data available to be received
    bool hasData(int sockfd) {
#ifdef _WIN32
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sockfd, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        return select(0, &readSet, nullptr, nullptr, &timeout) > 0;
#else
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sockfd, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        return select(sockfd + 1, &readSet, nullptr, nullptr, &timeout) > 0;
#endif
    }

    std::string receiveData() {
        unsigned char temp[16] = { 0 };
        char buffer[1024] = { 0 };
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Receive failed." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }

        buffer[bytesRead] = '\0'; // Add null terminator to the received data

        std::string receivedData(buffer);
        for (int i = 0; i < 16; i++) {
            temp[i] = receivedData.back();
            receivedData.pop_back();
        }

        for (int i = 0; i < 16; i++) {
            iv[i] = temp[15 - i];
        }

        return receivedData;
    }

    void sendMessage(std::string& message, const unsigned char* iv) {
        // Send a message and iv to the server
        std::string messageToSend = message + std::string(reinterpret_cast<const char*>(iv));
        std::cout << "Sending message: " << messageToSend << std::endl;
        int bytesSent = send(clientSocket, messageToSend.c_str(), messageToSend.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }
    }

    SOCKET getServerSocket() {
        return this->serverSocket;
    }

    SOCKET getClientSocket() {
        return this->clientSocket;
    }

    std::string getServerIP() {
        return this->serverIP;
    }

    int getPortNumber() {
        return this->portNumber;
    }

    void close() {
        // Close the client socket
        closesocket(clientSocket);

        // Cleanup Winsock
        WSACleanup();
    }

private:
    WSADATA wsaData;
    SOCKET serverSocket;
    int portNumber;
    std::string serverIP;
    SOCKET clientSocket;

    void initialize() {

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            exit(1);
        }
        createSocket();
    }

    void createSocket() {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket." << std::endl;
            WSACleanup();
            exit(1);
        }
        setServerAddressStructure();
    }

    void setServerAddressStructure() {

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(portNumber);
        if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr)) <= 0) {
            std::cerr << "Invalid address/Address not supported." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }

        connectToServer(serverAddress);
    }

    void connectToServer(sockaddr_in serverAddress) {
        std::cout << "Connecting to " << getServerIP() << "\\" << getPortNumber() << std::endl;
        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Connection failed." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }

        std::cout << "Connection made!" << std::endl;
    }


};

void randomizeIV(unsigned char* iv) {
    //std::cout << "Generating random IV: ";
    for (int i = 0; i < 16; i++) {
        iv[i] = rand() % 256;
        //std::cout << (iv[i]);
    }
    //std::cout << std::endl;
}


// Encrypts a message using AES
std::string encryptMessage(const std::string& message, const unsigned char* iv) {
    AES aes(aes_key);
    std::string encryptedMessage = aes.Encrypt(message, iv);
    return encryptedMessage;
}

// Decrypts a message using AES
std::string decryptMessage(const std::string& encryptedMessage, const unsigned char* iv) {
    AES aes(aes_key);
    std::string decryptedMessage = aes.Decrypt(encryptedMessage, iv);
    return decryptedMessage;
}

void receiveMessages(ClientSocketConn& socket) {
    while (true) {
        if (socket.hasData(socket.getClientSocket())) {
            std::string receivedMessage = socket.receiveData();
            std::string decryptedMessage = decryptMessage(receivedMessage, iv);

            std::cout << "Server: " << decryptedMessage << std::endl;
            std::cout << "--------------------------------------" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Delay to prevent high CPU usage
    }
}

int main() {

    std::cout << "AES Console Messaging Program\n";

    std::string serverIP = "127.0.0.1"; //REPLACE WITH YOUR SERVER IP
    int port = 8080; //REPLACE WITH YOUR SERVER PORT
    ClientSocketConn clientSocket(serverIP, port);
    clientSocket.setNonBlocking(clientSocket.getServerSocket());

    std::cout << "--------------------------------------\n";
    std::cout << "Enter a message to encrypt (or 'q' to quit): ";

    std::thread receiveThread(receiveMessages, std::ref(clientSocket));


    while (true) {

        if (_kbhit) {
            std::string message;
            std::getline(std::cin, message);

            if (message == "q")
                break;

            randomizeIV(iv);
            std::string encryptedMessage = encryptMessage(message, iv);
            clientSocket.sendMessage(encryptedMessage, iv);
        }
    }

    std::cout << "Connection closing" << std::endl;
    //close 
    clientSocket.close();
    receiveThread.join(); // Wait for the receive thread to finish
    std::cout << "Program terminated.\n";
    return 0;
}

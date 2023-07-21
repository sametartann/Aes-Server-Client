#include <iostream>
#include <string>
#include <conio.h> // For _kbhit
#include <thread> // For multithreading
#include "aes.h" // AES implementation
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

// AES key for encryption and decryption
static const unsigned char aes_key[] = {
    'k', 'e', 'y', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd'
};

// Initialization vector for AES with null values
static unsigned char iv[16] = { 0 };


class SocketConn {
public:
    SocketConn(int port) {
        portNumber = port;
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

        std::cout << "Receive data'dan selam : ";
        for (int i = 0; i < 16; i++) {
            std::cout << iv[i];
        }
        std::cout << std::endl;

        return receivedData;
    }

    void sendMessage(std::string& message, const unsigned char* iv) {

        std::string messageToSend = message + std::string(reinterpret_cast<const char*>(iv));
        std::cout << "Sending message: " << messageToSend << std::endl;
        int bytesSent = send(clientSocket, messageToSend.c_str(), messageToSend.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed." << std::endl;
            closesocket(clientSocket);
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }
    }

    SOCKET getClientSocket() {
        return clientSocket;
    }

    void close() {
        // Close the client socket
        closesocket(clientSocket);

        // Close the server socket
        closesocket(serverSocket);

        // Cleanup Winsock
        WSACleanup();
    }



private:
    WSADATA wsaData;
    SOCKET serverSocket;
    int portNumber;
    SOCKET clientSocket;

    void initialize() {

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            exit(1);
        }
        createSocket();
    }

    void createSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket." << std::endl;
            WSACleanup();
            exit(1);
        }
        bindAndListenSocket();
    }

    void bindAndListenSocket() {

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;  // Bind to any available network interface
        serverAddress.sin_port = htons(portNumber);        // Port to listen on

        if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Binding failed." << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }
        // Start listening for incoming connections
        if (listen(serverSocket, 3) == SOCKET_ERROR) {
            std::cerr << "Listen failed." << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(1);
        }

        std::cout << "Server listening on port " << portNumber << "..." << std::endl;
        connect();
    }

    void connect() {
        // Accept incoming connection
        sockaddr_in clientAddress{};
        int clientAddressSize = sizeof(clientAddress);
        clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            closesocket(serverSocket);
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

void receiveMessages(SocketConn& socket) {
    while (true) {
        if (socket.hasData(socket.getClientSocket())) {
            std::string receivedMessage = socket.receiveData();
            std::string decryptedMessage = decryptMessage(receivedMessage, iv);

            std::cout << "Client: " << decryptedMessage << std::endl;
            std::cout << "--------------------------------------" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Delay to prevent high CPU usage

    }
}

int main() {

    std::cout << "AES Console Messaging Program\n";

    SocketConn socket(8080);
    socket.setNonBlocking(socket.getClientSocket());
    std::cout << "--------------------------------------\n";
    std::cout << "Enter a message to encrypt (or 'q' to quit): ";

    std::thread receiveThread(receiveMessages, std::ref(socket));

    while (true) {
        if (_kbhit) {
            std::string message;
            std::getline(std::cin, message);

            if (message == "q") {
                break;
            }

            randomizeIV(iv);
            std::string encryptedMessage = encryptMessage(message, iv);
            socket.sendMessage(encryptedMessage, iv);
        }
    }
    std::cout << "Connection closing" << std::endl;
    socket.close();
    receiveThread.join(); // Wait for the receive thread to finish
    std::cout << "Program terminated.\n";

    return 0;
}

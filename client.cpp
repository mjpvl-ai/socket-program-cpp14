#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include "message.pb.h"

#define DEFAULT_PORT 8081
#define DEFAULT_SERVER_IP "127.0.0.1"

// Function to handle sending the request
void send_request(const std::string& server_ip, int port, ClientMessage& request) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    std::string serialized_msg;
    request.SerializeToString(&serialized_msg);
    send(sock, serialized_msg.c_str(), serialized_msg.size(), 0);

    char buffer[1024] = {0};
    recv(sock, buffer, sizeof(buffer), 0);

    ServerMessage response;
    if (response.ParseFromString(std::string(buffer))) {
        if (response.type() == REGISTRATION_ACK) {
            std::cout << "Server Response: " << response.reg_ack().status_message() << std::endl;
        } else if (response.type() == PDU_SESSION_ACK) {
            std::cout << "PDU Allocated: " << response.pdu_ack().pdu_id() << " - " << response.pdu_ack().status_message() << std::endl;
        } else if (response.type() == DEREGISTRATION_ACK) {
            std::cout << "Server Response: " << response.dereg_ack().status_message() << std::endl;
        }
    } else {
        std::cerr << "Failed to parse server response\n";
    }

    close(sock);
}

// Parse command-line arguments
void parse_arguments(int argc, char* argv[], std::string& server_ip, int& port, ClientMessage& message) {
    int option;
    std::string type;
    int id = -1, sst = -1;
    std::string sd = "";

    while ((option = getopt(argc, argv, "h:p:t:i:s:d:")) != -1) {
        switch (option) {
            case 'h':
                server_ip = optarg;
                break;
            case 'p':
                port = std::stoi(optarg);
                break;
            case 't':
                type = optarg;
                break;
            case 'i':
                id = std::stoi(optarg);
                break;
            case 's':
                sst = std::stoi(optarg);
                break;
            case 'd':
                sd = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-h server_ip] [-p port] [-t message_type] [-i id] [-s sst] [-d sd]" << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    // Set the message based on type and additional parameters
    if (type == "REGISTRATION_REQUEST") {
        message.set_type(REGISTRATION_REQUEST);
        if (id != -1) {
            message.mutable_reg_req()->set_id(id);
        } else {
            std::cerr << "ID is required for REGISTRATION_REQUEST" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (type == "PDU_SESSION_REQUEST") {
        message.set_type(PDU_SESSION_REQUEST);
        if (id != -1 && sst != -1 && !sd.empty()) {
            message.mutable_pdu_req()->set_id(id);
            message.mutable_pdu_req()->set_sst(sst);
            message.mutable_pdu_req()->set_sd(sd);
        } else {
            std::cerr << "ID, SST, and SD are required for PDU_SESSION_REQUEST" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else if (type == "DEREGISTRATION_REQUEST") {
        message.set_type(DEREGISTRATION_REQUEST);
        if (id != -1) {
            message.mutable_dereg_req()->set_id(id);
        } else {
            std::cerr << "ID is required for DEREGISTRATION_REQUEST" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        std::cerr << "Invalid message type" << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::string server_ip = DEFAULT_SERVER_IP;
    int port = DEFAULT_PORT;
    ClientMessage request;

    // Parse command-line arguments
    parse_arguments(argc, argv, server_ip, port, request);

    // Send the constructed request
    send_request(server_ip, port, request);

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

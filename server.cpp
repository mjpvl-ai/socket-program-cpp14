#include <iostream>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <iomanip>
#include <cstdlib>  // For std::stoi
#include <getopt.h> // For getopt_long (optional)
#include "message.pb.h"

std::unordered_map<int, bool> registered_users; // Stores registered users (ID -> registered flag)

#define DEFAULT_PORT 8081

// Function to validate the hexadecimal string 'sd' (should be a 4-byte hexadecimal number)
bool is_valid_hexadecimal(const std::string& str) {
    if (str.size() != 4) return false;  // Ensure it's exactly 4 characters
    for (char c : str) {
        if (!std::isxdigit(c)) return false;  // Check if all characters are hexadecimal digits
    }
    return true;
}

// Function to validate 'sst' (should be between 1 and 255)
bool is_valid_sst(int sst) {
    return sst >= 1 && sst <= 255;
}

// Function to handle client requests
void handle_client(int client_socket) {
    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0) {
        std::cerr << "Failed to receive data or connection closed\n";
        close(client_socket);
        return;
    }

    ClientMessage client_msg;
    if (!client_msg.ParseFromArray(buffer, bytes_received)) {
        std::cerr << "Error: Failed to parse client message\n";
        close(client_socket);
        return;
    }

    ServerMessage server_msg;

    switch (client_msg.type()) {
        case REGISTRATION_REQUEST: {
            int id = client_msg.reg_req().id();

            if (registered_users.find(id) == registered_users.end()) {
                registered_users[id] = true;  // Mark user as registered

                RegistrationAck* ack = server_msg.mutable_reg_ack();
                ack->set_id(id);
                ack->set_status(200);
                ack->set_status_message("Registration Successful");

                server_msg.set_type(REGISTRATION_ACK);
                std::cout << "User Registered: " << id << std::endl;
            } else {
                RegistrationAck* ack = server_msg.mutable_reg_ack();
                ack->set_id(id);
                ack->set_status(400);
                ack->set_status_message("User Already Registered");

                server_msg.set_type(REGISTRATION_ACK);
            }
            break;
        }

        case PDU_SESSION_REQUEST: {
            int id = client_msg.pdu_req().id();
            int sst = client_msg.pdu_req().sst();
            std::string sd = client_msg.pdu_req().sd();

            // Validate 'sst' (must be between 1 and 255)
            if (!is_valid_sst(sst)) {
                PduSessionAck* ack = server_msg.mutable_pdu_ack();
                ack->set_id(id);
                ack->set_status(400);
                ack->set_status_message("Invalid SST Value. Must be between 1 and 255.");

                server_msg.set_type(PDU_SESSION_ACK);
                break;
            }

            // Validate 'sd' (must be a valid 4-byte hexadecimal number)
            if (!is_valid_hexadecimal(sd)) {
                PduSessionAck* ack = server_msg.mutable_pdu_ack();
                ack->set_id(id);
                ack->set_status(400);
                ack->set_status_message("Invalid SD Value. Must be a 4-byte hexadecimal number.");

                server_msg.set_type(PDU_SESSION_ACK);
                break;
            }

            // Check if ID is registered
            if (registered_users.find(id) == registered_users.end()) {
                // Reject PDU session request if ID is not registered
                PduSessionAck* ack = server_msg.mutable_pdu_ack();
                ack->set_id(id);
                ack->set_status(403);
                ack->set_status_message("PDU Session Denied: ID Not Registered");

                server_msg.set_type(PDU_SESSION_ACK);
            } else {
                // Assign a PDU ID and approve session
                PduSessionAck* ack = server_msg.mutable_pdu_ack();
                ack->set_id(id);
                ack->set_pdu_id(rand() % 15 + 1); // Random PDU ID (1-15)
                ack->set_status(200);
                ack->set_status_message("PDU Session Established");

                server_msg.set_type(PDU_SESSION_ACK);
                std::cout << "PDU Session Created for User ID: " << id << std::endl;
            }
            break;
        }

        case DEREGISTRATION_REQUEST: {
            int id = client_msg.dereg_req().id();

            if (registered_users.find(id) != registered_users.end()) {
                registered_users.erase(id);  // Remove user from registered list

                DeregistrationAck* ack = server_msg.mutable_dereg_ack();
                ack->set_id(id);
                ack->set_status(200);
                ack->set_status_message("Deregistration Successful");

                server_msg.set_type(DEREGISTRATION_ACK);
                std::cout << "User Deregistered: " << id << std::endl;
            } else {
                DeregistrationAck* ack = server_msg.mutable_dereg_ack();
                ack->set_id(id);
                ack->set_status(400);
                ack->set_status_message("Deregistration Failed: ID Not Found");

                server_msg.set_type(DEREGISTRATION_ACK);
            }
            break;
        }

        default:
            std::cerr << "Unknown request type\n";
            close(client_socket);
            return;
    }

    std::string serialized_response;
    if (!server_msg.SerializeToString(&serialized_response)) {
        std::cerr << "Error: Failed to serialize response\n";
        close(client_socket);
        return;
    }

    send(client_socket, serialized_response.c_str(), serialized_response.size(), 0);
    close(client_socket);
}

// Function to handle command-line arguments
void parse_arguments(int argc, char** argv, int& port) {
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = std::stoi(optarg);  // Convert string to integer
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -p <port>\n";
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    int server_fd, client_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int port = DEFAULT_PORT;
    parse_arguments(argc, argv, port);  // Parse command-line arguments for port

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << "...\n";

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_socket);
    }

    close(server_fd);
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

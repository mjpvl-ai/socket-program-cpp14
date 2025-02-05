#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

const std::string SERVER_URL = "http://server:8081";

// Callback function for handling response
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

// Function to send HTTP POST request
std::string sendPostRequest(const std::string &url, const json &data) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return "";
    }

    std::string response;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    std::string jsonData = data.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "CURL request failed: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return response;
}

// Function to register a user
void registerUser(int id) {
    json requestData = {{"id", id}};
    std::string response = sendPostRequest(SERVER_URL + "/register", requestData);
    cout << "Response: " << response << endl;
}

// Function to initiate PDU session
void pduSession(int id, int sst, const std::string &sd) {
    json requestData = {{"id", id}, {"sst", sst}, {"sd", sd}};
    std::string response = sendPostRequest(SERVER_URL + "/pdu-session", requestData);
    cout << "Response: " << response << endl;
}

// Function to send DELETE request
std::string sendDeleteRequest(const std::string &url, const json &data) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return "";
    }

    std::string response;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE"); // DELETE method
    std::string jsonData = data.dump();
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "CURL request failed: " << curl_easy_strerror(res) << endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return response;
}

// Updated Deregister Function
void deregisterUser(int id) {
    json requestData = {{"id", id}};
    std::string response = sendDeleteRequest(SERVER_URL + "/deregister", requestData);
    cout << "Response: " << response << endl;
}


int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    int choice, id, sst;
    std::string sd;

    while (true) {
        cout << "\n--- Client API ---\n";
        cout << "1. Register User\n";
        cout << "2. Request PDU Session\n";
        cout << "3. Deregister User\n";
        cout << "4. Exit\n";
        cout << "Enter choice: ";
        cin >> choice;

        if (choice == 1) {
            cout << "Enter User ID: ";
            cin >> id;
            registerUser(id);
        } else if (choice == 2) {
            cout << "Enter User ID: ";
            cin >> id;
            cout << "Enter SST (1-255): ";
            cin >> sst;
            cout << "Enter SD (4-bit binary, e.g., 0001): ";
            cin >> sd;
            pduSession(id, sst, sd);
        } else if (choice == 3) {
            cout << "Enter User ID: ";
            cin >> id;
            deregisterUser(id);
        } else if (choice == 4) {
            break;
        } else {
            cout << "Invalid choice. Try again.\n";
        }
    }

    curl_global_cleanup();
    return 0;
}

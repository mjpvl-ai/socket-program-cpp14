#include <iostream>
#include <unordered_map>
#include <pistache/endpoint.h>
#include <pistache/router.h>
#include <pistache/http.h>
#include <nlohmann/json.hpp>

using namespace Pistache;
using json = nlohmann::json;

std::unordered_map<int, bool> registered_users; // Stores registered users

class ServerAPI {
public:
    explicit ServerAPI(Address addr) : httpEndpoint(std::make_shared<Http::Endpoint>(addr)) {}

    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options().threads(thr);
        httpEndpoint->init(opts);
        setupRoutes();
    }

    void start() {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serve();
    }

private:
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;

    void setupRoutes() {
        Rest::Routes::Post(router, "/register", Rest::Routes::bind(&ServerAPI::registerUser, this));
        Rest::Routes::Post(router, "/pdu-session", Rest::Routes::bind(&ServerAPI::pduSession, this));
        Rest::Routes::Delete(router, "/deregister", Rest::Routes::bind(&ServerAPI::deregisterUser, this));
    }

    void registerUser(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            auto body = json::parse(request.body());
            int id = body.at("id");

            json responseJson;
            std::cout << "Received registration request with ID: " << id << std::endl;

            if (registered_users.find(id) == registered_users.end()) {
                registered_users[id] = true;
                responseJson["status"] = 200;
                responseJson["message"] = "Registration Successful";
            } else {
                responseJson["status"] = 400;
                responseJson["message"] = "User Already Registered";
            }
            response.send(Http::Code::Ok, responseJson.dump());
        } catch (const std::exception& e) {
            std::cerr << "Error parsing registration request: " << e.what() << std::endl;
            response.send(Http::Code::Bad_Request, "Invalid JSON format");
        }
    }

    void pduSession(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            std::string bodyStr = request.body();
            std::cout << "Raw request body: " << bodyStr << std::endl;

            auto body = json::parse(bodyStr);  // JSON Parsing
            int id = body.at("id");
            int sst = body.at("sst");
            std::string sd = body.at("sd");

            json responseJson;
            if (registered_users.find(id) == registered_users.end()) {
                responseJson["status"] = 403;
                responseJson["message"] = "PDU Session Denied: ID Not Registered";
            } else if (sst < 1 || sst > 255) {
                responseJson["status"] = 400;
                responseJson["message"] = "Invalid SST Value. Must be between 1 and 255.";
            } else if (sd.size() != 4 || sd.find_first_not_of("01") != std::string::npos) {
                responseJson["status"] = 400;
                responseJson["message"] = "Invalid SD Value. Must be a 4-bit binary number (0000-1111).";
            } else {
                responseJson["status"] = 200;
                responseJson["pdu_id"] = rand() % 15 + 1;
                responseJson["message"] = "PDU Session Established";
            }
            response.send(Http::Code::Ok, responseJson.dump());
        } catch (const json::exception &e) {
            std::cerr << "Error parsing PDU session request: " << e.what() << std::endl;
            response.send(Http::Code::Bad_Request, "Invalid JSON format");
        }
    }


    void deregisterUser(const Rest::Request& request, Http::ResponseWriter response) {
        try {
            auto body = json::parse(request.body());
            int id = body.at("id");

            json responseJson;
            std::cout << "Received deregistration request with ID: " << id << std::endl;

            if (registered_users.erase(id)) {
                responseJson["status"] = 200;
                responseJson["message"] = "Deregistration Successful";
            } else {
                responseJson["status"] = 400;
                responseJson["message"] = "Deregistration Failed: ID Not Found";
            }
            response.send(Http::Code::Ok, responseJson.dump());
        } catch (const std::exception& e) {
            std::cerr << "Error parsing deregistration request: " << e.what() << std::endl;
            response.send(Http::Code::Bad_Request, "Invalid JSON format");
        }
    }
};

int main() {
    Address addr(Ipv4::any(), Port(8081));
    ServerAPI server(addr);

    server.init();
    std::cout << "REST API Server running on port 8081...\n";
    server.start();

    return 0;
}

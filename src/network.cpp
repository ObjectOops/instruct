#include "network.hpp"

bool instruct::net::ping(std::string url) {
    httplib::Client client {url};
    httplib::Result res {client.Get("/heartbeat")};
    
    // Managed to establish connection.
    return res.error() != httplib::Error::Connection;
}

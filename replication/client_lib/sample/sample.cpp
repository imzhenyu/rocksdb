#include "rrdb_client.h"
#include <iostream>
#include <string>
#include <unistd.h>

using namespace dsn::apps;

int main()
{
    bool init = rrdb_client_factory::initialize("config.ini");
    if (!init) {
        std::cerr << "init failed" << std::endl;
        return -1;
    }
    irrdb_client* client = rrdb_client_factory::get_client("rrdb.instance0");
    int ret = client->set("k1", "s1", "v1");
    std::cout<< "set key error=" << ret << std::endl;

    std::string value;
    ret = client->get("k1", "s1", value);
    std::cout<< "get key error=" << ret << std::endl;
    std::cout<< "get key=" << value << std::endl;

    ret = client->del("k1", "s1");
    std::cout<< "del key error=" << ret << std::endl;

    ret = client->get("k1", "s1", value);
    std::cout<< "get after del, error=" << ret << "," << client->get_error_string(ret) << std::endl;

}

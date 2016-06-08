#include <stdio.h>
#include <iostream>
#include "env_handler_fork.h"
#include "env_handler_supervisor.h"

using namespace pegasus::test;

void env_handler_fork_example()
{
    env_handler* eh = new env_handler_fork();
    std::cout << "new" <<std::endl;
    getchar();
    eh->start_all();
    std::cout << "after start all" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->kill_meta(1);
    std::cout << "after kill meta 1" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->start_meta(1);
    std::cout << "after start meta 1" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->kill_replica(2);
    std::cout << "after kill replica 2" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->start_replica(2);
    std::cout << "after start replica 2" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->kill_meta_primary();
    std::cout << "after kill meta primary" << std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();

    eh->kill_all();
    std::cout << "after kill all" <<std::endl;
    system("ps aux | grep rrdb | grep app_list");
    getchar();
    delete eh;
}

void env_handler_supervisor_example()
{

    env_handler* eh = new env_handler_supervisor();
    std::cout << "new" <<std::endl;
    getchar();

    eh->kill_replica(2);
    std::cout << "after kill replica 2" <<std::endl;
    getchar();

    eh->start_replica(2);
    std::cout << "after start replica 2" <<std::endl;
    getchar();

    eh->kill_meta_primary();
    std::cout << "after kill meta primary" << std::endl;
    getchar();

    eh->kill_all();
    std::cout << "after kill all" <<std::endl;
    getchar();
    delete eh;
}


int main(int argc, char **argv)
{
    dsn_run(argc, argv);
    //env_handler_fork_example()
    //env_handler_supervisor_example()
}

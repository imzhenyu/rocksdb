#include <stdio.h>
#include <iostream>
#include "tool.h"
#include <string>
#include <vector>

using namespace ::dsn::apps;

#define PARA_NUM 10

int main(int argc, const char* argv[])
{
    printHead();
    std::cout << std::endl;
    printHelpInfo();
    std::cout << std::endl;

    std::string config_file = argc > 1 ? argv[1] : "config.ini";
    if (!rrdb_client_factory::initialize(config_file.c_str()))
    {
        std::cout << "ERROR: init pegasus failed: " << config_file << std::endl;
        return -1;
    }
    else
    {
        std::cout << "The config file is: " << config_file << std::endl;
    }

    std::string op_name;
    std::string app_name;

    std::vector<dsn::rpc_address> meta_servers;
    dsn::replication::replication_app_client_base::load_meta_servers(meta_servers);
    dsn::replication::replication_ddl_client* client_of_dsn = new dsn::replication::replication_ddl_client(meta_servers);


    std::string ip_addr_of_cluster;
    irrdb_client* client_of_rrdb = rrdb_client_factory::get_client(app_name.c_str(), ip_addr_of_cluster.c_str());
    if (client_of_rrdb != NULL)
    {
        std::cout << "The connecting cluster is: " << client_of_rrdb->get_cluster_meta_servers() << std::endl;
    }

    while ( true )
    {
        int Argc = 0;
        std::string Argv[PARA_NUM];
        scanfCommand(Argc, Argv, PARA_NUM);

        if ( Argc < 0 )
            break;

        if ( Argc == 0 )
            continue;

        int partition_count = 4;
        int replica_count = 3;
        std::string status;
        bool detailed = false;
        std::string out_file;

        for(int index = 1; index < Argc; index++)
        {
            if(Argv[index] == "-pc" && Argc > index + 1)
                partition_count = atol(Argv[++index].c_str());
            else if(Argv[index] == "-rc" && Argc > index + 1)
                replica_count = atol(Argv[++index].c_str());
            else if(Argv[index] == "-status" && Argc > index + 1)
                status = Argv[++index];
            else if(Argv[index] == "-detailed")
                detailed = true;
            else if(Argv[index] == "-o"&& Argc > index + 1)
                out_file = Argv[++index];
        }

        op_name = Argv[0];
        if ( op_name == HELP_OP )
            help_op(Argc);
        else if ( op_name == CLUSTER_INFO_OP )
        {
            cluster_info_op(*client_of_dsn);
        }
        else if ( op_name == CREATE_APP_OP )
        {
            if ( Argc >= 3 )
                create_app_op(Argv[1], Argv[2], partition_count, replica_count, *client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "create <app_name> <app_type> [-pc partition_count] [-rc replication_count]" << std::endl;
        }
        else if( op_name == DROP_APP_OP )
        {
            if ( Argc == 2 )
                drop_app_op(Argv[1], *client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "drop <app_name>" << std::endl;
        }
        else if ( op_name == LIST_APPS_OP )
        {
            if ( Argc == 1 || (Argc == 3 && (Argv[1] == "-status" || Argv[1] == "-o"))
                 || (Argc == 5 && Argv[1] == "-status" && Argv[3] == "-o") )
                list_apps_op(status, out_file, *client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "ls [-status <all|available|creating|creating_failed|dropping|dropping_failed|dropped>] [-o <out_file>]" << std::endl;
        }
        else if ( op_name == LIST_APP_OP )
        {
            if ( Argc >= 2 && Argc <= 5 )
                list_app_op(Argv[1], detailed, out_file, *client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "app <app_name> [-detailed] [-o <out_file>]" << std::endl;
        }
        else if ( op_name == LIST_NODES_OP) {
            if ( Argc == 1 || Argc == 3 || Argc == 5 )
                list_node_op(status, out_file, *client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "nodes [-status <all|alive|unalive>] [-o <out_file>]" << std::endl;
        }
        else if (op_name == STOP_MIGRATION_OP) {
            if ( Argc == 1 )
                stop_migration_op(*client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "stop_migration" << std::endl;
        }
        else if (op_name == START_MIGRATION_OP) {
            if ( Argc == 1 )
                start_migration_op(*client_of_dsn);
            else
                std::cout << "USAGE: " << std::endl << "\t"
                          << "start_migration" << std::endl;
        }
        else if (op_name == BALANCER_OP) {
            dsn::replication::balancer_proposal_request request;
            for (int i=1; i<Argc-1; i+=2) {
                if ( Argv[i] == "-gpid" ){
                    sscanf(Argv[i+1].c_str(), "%d.%d", &request.gpid.app_id, &request.gpid.pidx);
                }
                else if ( Argv[i] == "-type" ){
                    std::map<std::string, dsn::replication::balancer_type> mapper = {
                        {"move_pri", dsn::replication::BT_MOVE_PRIMARY},
                        {"copy_pri", dsn::replication::BT_COPY_PRIMARY},
                        {"copy_sec", dsn::replication::BT_COPY_SECONDARY}
                    };
                    if (mapper.find(Argv[i+1]) == mapper.end()) {
                        std::cout << "balancer -gpid <appid.pidx> -type <move_pri|copy_pri|copy_sec> -from <from_address> -to <to_address>" << std::endl;
                    }
                    request.type = mapper[Argv[i+1]];
                }
                else if ( Argv[i] == "-from" ) {
                    request.from_addr.from_string_ipv4(Argv[i+1].c_str());
                }
                else if ( Argv[i] == "-to" ) {
                    request.to_addr.from_string_ipv4(Argv[i+1].c_str());
                }
            }
            dsn::error_code err = client_of_dsn->send_balancer_proposal(request);
            std::cout << "send balancer proposal result: " << err.to_string() << std::endl;
        }
        else if (op_name == CONNECT )
        {
            if ( Argc == 1 ) {
                client_of_rrdb = rrdb_client_factory::get_client(app_name.c_str(), ip_addr_of_cluster.c_str());
                if (client_of_rrdb != NULL) {
                    std::cout << "The connecting cluster is: " << client_of_rrdb->get_cluster_meta_servers() << std::endl;
                }
                continue;
            }
            else if (Argc != 2) {
                std::cout << "USAGE: connect [meta_servers]" << std::endl;
                continue;
            }

            std::string tmp_cluster = Argv[1];
            std::vector< ::dsn::rpc_address> servers;
            bool parseSuccess = connect_op(tmp_cluster, servers);
            if ( parseSuccess )
            {
                delete client_of_dsn;
                client_of_dsn = new dsn::replication::replication_ddl_client(servers);
                std::cout << "OK" << std::endl;
                ip_addr_of_cluster = tmp_cluster;
                app_name = "";
            }
            else
                std::cout << "USAGE: connect [meta_servers]" << std::endl;
        }
        else if ( op_name == USE_OP )
            use_op(Argc, Argv, app_name);
        else if ( op_name == HASH_OP )
            hash_op(Argc, Argv, app_name, *client_of_dsn);
        else if ( op_name == GET_OP )
        {
            if (app_name.empty()) {
                std::cout << "No app is using now!" << std::endl;
                std::cout << "USAGE: use [app_name]" << std::endl;
                continue;
            }
            client_of_rrdb = rrdb_client_factory::get_client(app_name.c_str(), ip_addr_of_cluster.c_str());
            if (client_of_rrdb != NULL)
                get_op(Argc, Argv, client_of_rrdb);
        }
        else if (op_name == SET_OP)
        {
            if (app_name.empty()) {
                std::cout << "No app is using now!" << std::endl;
                std::cout << "USAGE: use [app_name]" << std::endl;
                continue;
            }
            client_of_rrdb = rrdb_client_factory::get_client(app_name.c_str(), ip_addr_of_cluster.c_str());
            if (client_of_rrdb != NULL)
                set_op(Argc, Argv, client_of_rrdb);
        }
        else if ( op_name == DEL_OP )
        {
            if (app_name.empty()) {
                std::cout << "No app is using now!" << std::endl;
                std::cout << "USAGE: use [app_name]" << std::endl;
                continue;
            }
            client_of_rrdb = rrdb_client_factory::get_client(app_name.c_str(), ip_addr_of_cluster.c_str());
            if (client_of_rrdb != NULL)
                del_op(Argc, Argv, client_of_rrdb);
        }
        else if ( op_name == EXIT_OP )
            return 0;
        else
            std::cout << "ERROR: Invalid op-name: " << op_name << std::endl;
    }

    return 0;
}

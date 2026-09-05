// Copyright (c) 2018 - 2026, The Arqma Network
//
// All rights reserved.

#include "storage_server.h"

#include <boost/program_options.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
  namespace po = boost::program_options;
  std::string listen = "127.0.0.1:22021";
  po::options_description desc{"arqma-storage"};
  desc.add_options()("help,h", "show help")(
      "listen", po::value<std::string>(&listen)->default_value(listen), "host:port (HTTP Storage Server)");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help"))
  {
    std::cout << desc
              << "\nCompanion binary for arqmad. Probe with:\n"
                 "  arqmad --storage-client-url=http://"
              << listen << "\n";
    return 0;
  }
  const auto colon = listen.rfind(':');
  if (colon == std::string::npos)
  {
    std::cerr << "listen must be host:port\n";
    return 1;
  }
  const auto host = listen.substr(0, colon);
  const auto port = static_cast<std::uint16_t>(std::stoi(listen.substr(colon + 1)));
  arq_storage::StorageServer server;
  if (const auto ec = server.listen(host, port))
  {
    std::cerr << "bind failed: " << ec.message() << "\n";
    return 1;
  }
  std::cout << "arqma-storage listening on " << server.base_url() << std::endl;
  while (server.running())
    std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}

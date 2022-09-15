/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:29:35 2022 Francois Michaut
** Last update Wed Sep 14 20:49:39 2022 Francois Michaut
**
** ClientConfig.cpp : ClientConfig implementation
*/

#include "FileShareProtocol/ClientConfig.hpp"

#include <iostream>

namespace FileShareProtocol {
    inline static ClientConfig make_default_config() noexcept {
        try {
            return ClientConfig(); // TODO set defaults
        } catch (std::exception &e) {
            std::cerr << "Error while initializing default configuration: " << e.what() << std::endl;
            std::terminate();
        } catch (...) {
            std::cerr << "Unknown error while initializing default configuration" << std::endl;
            std::terminate();
        }
    }

    ClientConfig ClientConfig::default_config = make_default_config(); //NOLINT

    ClientConfig::ClientConfig(
        std::string default_destination_folder, std::string root_name,
        std::vector<std::string> public_paths, std::vector<std::string> private_paths,
        TransportProtocol transport_protocol
    ) :
        root_name(std::move(root_name)), public_paths(std::move(public_paths)),
        private_paths(std::move(private_paths)), transport_protocol(transport_protocol),
        default_destination_folder(std::move(default_destination_folder))
    {}
}

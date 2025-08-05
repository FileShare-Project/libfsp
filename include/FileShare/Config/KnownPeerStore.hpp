/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Sat Aug 16 20:24:31 2025 Francois Michaut
** Last update Mon Aug 18 20:02:00 2025 Francois Michaut
**
** KnownPeerStore.hpp : Storage and retrival of known peers
*/

#include "FileShare/Peer/PreAuthPeer.hpp"
#include "FileShare/Utils/Strings.hpp"

#include <string>
#include <unordered_map>

// TODO: Make a custom cereal archive for known_peers : https://uscilab.github.io/cereal/serialization_archives.html#adding-more-archives
namespace FileShare {
    class KnownPeerStore {
        public:
            // TODO: Figure out the constructor (std::string filepath = "default_path") ?
            KnownPeerStore() = default;

            // Raises if uuid / public_key is not unique (skip silently if exact match)
            void insert(std::string_view uuid, std::string_view public_key);
            void remove(std::string_view uuid, std::string_view public_key);

            // raises if uuid / public_key dont match
            auto contains(std::string_view uuid, std::string_view public_key) -> bool;
            auto contains(const PreAuthPeer &peer) -> bool;

        private:
            std::unordered_map<std::string, std::string, FileShare::Utils::string_hash, std::equal_to<>> m_known_peers;
    };
}

// TODO: Maybe store the peer certificates, so we can validate that they are not changing
// (eg: peer could bump its notAfter date all the time with the same pkey, and we would
// accept him forever)

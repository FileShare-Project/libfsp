/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Mon Aug 18 17:16:41 2025 Francois Michaut
** Last update Thu Aug 21 14:06:50 2025 Francois Michaut
**
** KnownPeerStore.cpp : Implementation of the storage of known peers
*/

#include "FileShare/Config/KnownPeerStore.hpp"

#include <stdexcept>

// TODO: Real implementation
namespace FileShare {
    // Raises if uuid / public_key is not unique (skip silently if exact match)
    void KnownPeerStore::insert(std::string_view uuid, std::string_view public_key) {
        auto result = m_known_peers.emplace(uuid, public_key);

        // TODO: Support pubkey rotation
        if (!result.second && result.first->second != public_key) {
            throw std::runtime_error("A different PublicKey exists for this UUID");
        }
    }

    void KnownPeerStore::remove(std::string_view uuid, std::string_view public_key) {
        auto iter = m_known_peers.find(uuid);

        if (iter != m_known_peers.end()) {
            if (iter->second != public_key) {
                throw std::runtime_error("A different PublicKey exists for this UUID");
            }
            m_known_peers.erase(iter);
        }
    }

    // raises if uuid / public_key not unique
    auto KnownPeerStore::contains(std::string_view uuid, std::string_view public_key) -> bool {
        auto iter = m_known_peers.find(uuid);

        if (iter == m_known_peers.end()) {
            return false;
        }
        if (iter->second != public_key) {
            throw std::runtime_error("A different PublicKey exists for this UUID");
        }
        return true;
    }

    auto KnownPeerStore::contains(const PreAuthPeer &peer) -> bool {
        return contains(peer.get_device_uuid(), peer.get_public_key());
    }
}

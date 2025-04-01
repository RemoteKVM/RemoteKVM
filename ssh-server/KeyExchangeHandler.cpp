#include "KeyExchangeHandler.h"
#include <sstream>
#include <random> // for random number generation
#include <cstring> // for strlen
#include <boost/random.hpp>
#include <gmpxx.h>
#include "SshMsgKexInit.h"
#include "RSA.h"


mpz_class KeyExchangeHandler::mod_exp_gmp(mpz_class base, mpz_class exponent, mpz_class modulus) {
    mpz_class result;
    mpz_powm(result.get_mpz_t(), base.get_mpz_t(), exponent.get_mpz_t(), modulus.get_mpz_t());
    return result;
}

mpz_class KeyExchangeHandler::generate_private_key_gmp(const mpz_class& max_exclusive_start_from_zero) {
    gmp_randclass rng(gmp_randinit_mt);
    rng.seed(time(nullptr));
    return rng.get_z_range(max_exclusive_start_from_zero) + 2; // Generates in [2, max_exclusive_start_from_zero + 1]
}

void KeyExchangeHandler::performVersionExchange() {
    // Read the client's version string
    char client_version[256] = {0};
    ssize_t bytes_read = read(socket, client_version, sizeof(client_version) - 1);
    if (bytes_read <= 0) {
        throw SshException("Error reading client version");
    }
    client_version[bytes_read] = '\0';
    hMsgData.set_vc(client_version);

    if (strncmp(client_version, "SSH-2.0", 4) != 0) {
        throw SshException("Client did not send a valid SSH version string. Support version 2.0 only.");
    }

    // Send SSH version string
    const char* server_version = "SSH-2.0-RemoteKVMsSSH_1.0\r\n";
    if (write(socket, server_version, strlen(server_version)) < 0) {
        throw SshException("Error sending SSH version string");
    }
    hMsgData.set_vs(server_version);
}


void KeyExchangeHandler::negotiateEncryptionAlgorithms()
{
    // Build our SSH_MSG_KEX_INIT offering only our chosen algorithms.
    SshMsgKexInit serverKexInit;
    serverKexInit.kex_algorithms = {"diffie-hellman-group14-sha256"};
    serverKexInit.server_host_key_algorithms = {"rsa-sha2-256"};
    serverKexInit.encryption_algorithms_client_to_server = {"aes256-ctr"};
    serverKexInit.encryption_algorithms_server_to_client = {"aes256-ctr"};
    serverKexInit.mac_algorithms_client_to_server = {"hmac-sha2-256"};
    serverKexInit.mac_algorithms_server_to_client = {"hmac-sha2-256"};
    serverKexInit.compression_algorithms_client_to_server = {"none"};
    serverKexInit.compression_algorithms_server_to_client = {"none"};
    serverKexInit.languages_client_to_server = {};
    serverKexInit.languages_server_to_client = {};
    serverKexInit.first_kex_packet_follows = false;

    std::vector<uint8_t> payload = serverKexInit.serialize();
    sshSocket->sendPacket(payload);
    hMsgData.set_is(payload);
    

    // Read the peer's SSH_MSG_KEX_INIT message.
    std::vector<uint8_t> peerKexInitPayload = sshSocket->readPacket();
    SshMsgKexInit peerKexInit = SshMsgKexInit::deserialize(peerKexInitPayload);
    hMsgData.set_ic(peerKexInitPayload);
    
    if(!peerKexInit.checkSportedAlgoritems(serverKexInit))
    {
        throw SshException("The clinet don't support the algorithms that the server support!");
    }
    
}


mpz_class KeyExchangeHandler::readClientPublicKey()
{
    std::vector<uint8_t> clients_df_init_mgs = sshSocket->readPacket();
    if (clients_df_init_mgs[0] != SSH_MSG_KEX_DH_INIT)
    {
        throw SshException("Expected SSH_MSG_KEX_INIT message from client!");
    }
    std::vector<uint8_t> e_bytes(clients_df_init_mgs.begin() + 1, clients_df_init_mgs.end());
    return SshTypeHelper::mpintToMpz(e_bytes);
}

void KeyExchangeHandler::sendDhReplyMsg(const std::vector<uint8_t>& k_s, const mpz_class& f_gmp, const std::vector<uint8_t>& H_singed)
{
    std::vector<uint8_t> dh_reply_msg;
    dh_reply_msg.push_back(SSH_MSG_KEX_DH_REPLY);

    SshTypeHelper::appendAsSshStringToVector(dh_reply_msg, std::string(k_s.begin(), k_s.end()));
    SshTypeHelper::appendAsMpintToVector(dh_reply_msg, f_gmp);
    SshTypeHelper::appendAsSshStringToVector(dh_reply_msg, std::string(H_singed.begin(), H_singed.end()));

    sshSocket->sendPacket(dh_reply_msg);
}


void KeyExchangeHandler::performDiffieHellmanKeyExchange()
{
    // Read client's public key (e)
    mpz_class e_gmp = readClientPublicKey();
    hMsgData.set_e(e_gmp);

    // Generate server's private key (b)
    mpz_class b_gmp = generate_private_key_gmp(p_gmp - 1); // max value is set to p-1 because b must be in the range of 1 to p-1

    // Compute server's public key (f)
    mpz_class f_gmp = mod_exp_gmp(g_gmp, b_gmp, p_gmp); // server's public key (f) = g^b mod p
    hMsgData.set_f(f_gmp);

    // Compute shared secret key (k)
    shared_secret_mpz = mod_exp_gmp(e_gmp, b_gmp, p_gmp); // shared secret key (k) = e^b mod p
    hMsgData.set_k(shared_secret_mpz);
    sshSocket->setSheredSecret(shared_secret_mpz);

    RSA rsa;
    rsa.loadPrivateKeyFromPEM(HOST_KEY_PEM_FILE);
    std::vector<uint8_t> k_s = rsa.getPublicKeyBlob();
    hMsgData.set_ks(k_s);
    std::vector<uint8_t> h_data_hashed = SHA256::hash(hMsgData.get_h_data_bytes());
    std::vector<uint8_t> H_singed = rsa.getSSHSignature(h_data_hashed);
    sshSocket->setSessionID(h_data_hashed);

    sendDhReplyMsg(k_s, f_gmp, H_singed);
}

void KeyExchangeHandler::sendAndResiveNewKeysMsg()
{
    std::vector<uint8_t> client_new_keys_msg = sshSocket->readPacket();
    if (client_new_keys_msg[0] != SSH_MSG_NEWKEYS)
    {
        throw SshException("Expected SSH_MSG_NEWKEYS message from client!");
    }
    //send new keys
    std::vector<uint8_t> new_keys_msg_to_send;
    new_keys_msg_to_send.push_back(SSH_MSG_NEWKEYS);
    sshSocket->sendPacket(new_keys_msg_to_send);
}



void KeyExchangeHandler::perform_ssh_init() 
{
    performVersionExchange();
    
    negotiateEncryptionAlgorithms();

    performDiffieHellmanKeyExchange();

    sendAndResiveNewKeysMsg();

    sshSocket->startEncryptedPhase();
}
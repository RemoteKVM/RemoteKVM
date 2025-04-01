#pragma once

#include <string>
#include <gmpxx.h> // GMP C++ interface
#include <iostream> // For basic error output
#include <unistd.h> // For read, write, close
#include <string>
#include <vector> // For std::vector
#include "SshSocket.h"
#include "SshException.h"
#include "SshTypeHelper.h"
#include "HMsgData.h"

// SSH message types
#define SSH_MSG_KEX_INIT 20
#define SSH_MSG_NEWKEYS 21
#define SSH_MSG_KEX_DH_INIT 30
#define SSH_MSG_KEX_DH_REPLY 31

#define HOST_KEY_PEM_FILE "host_key.pem"

class KeyExchangeHandler {
public:
    KeyExchangeHandler(int socket, SshSocket* sshSocket) :  socket(socket), sshSocket(sshSocket) {}
    ~KeyExchangeHandler() {}

    void perform_ssh_init();
    mpz_class get_shared_secret() const { return shared_secret_mpz; }

private:
    int socket;
    SshSocket* sshSocket;
    HMsgData hMsgData;
    mpz_class shared_secret_mpz;


    // Group 14 Prime (p) - copied directly from RFC 3526, 2048-bit prime.
    const mpz_class p_gmp = mpz_class(
        "FFFFFFFFFFFFFFFFC90FDAA22168C234"
        "C4C6628B80DC1CD129024E088A67CC74"
        "020BBEA63B139B22514A08798E3404DD"
        "EF9519B3CD3A431B302B0A6DF25F1437"
        "4FE1356D6D51C245E485B576625E7EC6"
        "F44C42E9A637ED6B0BFF5CB6F406B7ED"
        "EE386BFB5A899FA5AE9F24117C4B1FE6"
        "49286651ECE45B3DC2007CB8A163BF05"
        "98DA48361C55D39A69163FA8FD24CF5F"
        "83655D23DCA3AD961C62F356208552BB"
        "9ED529077096966D670C354E4ABC9804"
        "F1746C08CA18217C32905E462E36CE3B"
        "E39E772C180E86039B2783A2EC07A28F"
        "B5C55DF06F4C52C9DE2BCBF695581718"
        "3995497CEA956AE515D2261898FA0510"
        "15728E5A8AACAA68FFFFFFFFFFFFFFFF",
        16 // Base 16 (hex)
    );

    // Group 14 Generator (g) - is simply 2
    const mpz_class g_gmp = 2;


    void performVersionExchange();
    void negotiateEncryptionAlgorithms();
    void performDiffieHellmanKeyExchange();
    void sendAndResiveNewKeysMsg();


    mpz_class readClientPublicKey();
    void sendDhReplyMsg(const std::vector<uint8_t>& k_s, const mpz_class& f_gmp, const std::vector<uint8_t>& H_singed);
    
    // GMP helper functions
    mpz_class mod_exp_gmp(mpz_class base, mpz_class exponent, mpz_class modulus);
    mpz_class generate_private_key_gmp(const mpz_class& max_value);
};
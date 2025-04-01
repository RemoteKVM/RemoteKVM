#pragma once

#include <string>
#include <vector>
#include "SshException.h"
#include "SshTypeHelper.h"

class HMsgData {
public:
    void set_vc(const std::string& vc_string) {
        vc = vc_string;
        remove_cr_lf(vc);
    }

    void set_vs(const std::string& vs_string) {
        vs = vs_string;
        remove_cr_lf(vs);
    }

    void set_ic(const std::vector<uint8_t>& ic_payload) {
        ic = ic_payload;
    }

    void set_is(const std::vector<uint8_t>& is_payload) {
        is = is_payload;
    }

    void set_ks(const std::vector<uint8_t>& ks_host_key) {
        ks = ks_host_key;
    }

    void set_e(const mpz_class& e_mpint) {
        e = e_mpint;
    }

    void set_f(const mpz_class& f_exchange_value_str) {
        f = f_exchange_value_str;
    }

    void set_k(const mpz_class& k_shared_secret_str) {
        k = k_shared_secret_str;
    }

    /*
        * Returns the data bytes that need to be hashed for the H field of the SSH_MSG_KEXINIT message.
        * The data is concatenated in the following order:
        * V_C || V_S || I_C || I_S || K_S || e || f || K
    */
    std::vector<uint8_t> get_h_data_bytes() const {
        if (vc.empty() || vs.empty() || ic.empty() || is.empty() || ks.empty()) {
            throw SshException("All string and byte fields (V_C, V_S, I_C, I_S, K_S) must be set.");
        }

        std::vector<uint8_t> data_to_hash;

        SshTypeHelper::appendAsSshStringToVector(data_to_hash, vc);
        SshTypeHelper::appendAsSshStringToVector(data_to_hash, vs);
        SshTypeHelper::appendAsSshStringToVector(data_to_hash, std::string(ic.begin(), ic.end()));
        SshTypeHelper::appendAsSshStringToVector(data_to_hash, std::string(is.begin(), is.end()));
        SshTypeHelper::appendAsSshStringToVector(data_to_hash, std::string(ks.begin(), ks.end()));
        SshTypeHelper::appendAsMpintToVector(data_to_hash, e);
        SshTypeHelper::appendAsMpintToVector(data_to_hash, f);
        SshTypeHelper::appendAsMpintToVector(data_to_hash, k);

        return data_to_hash;
    }


private:
    std::string vc; // V_C - version string of the client
    std::string vs; // V_S - version string of the server
    std::vector<uint8_t> ic; // I_C - payload of the client's SSH_MSG_KEXINIT
    std::vector<uint8_t> is; // I_S - payload of the server's SSH_MSG_KEXINIT
    std::vector<uint8_t> ks; // K_S - server's public host key
    mpz_class e; // e - exchange value sent by the client
    mpz_class f; // f - exchange value sent by the server
    mpz_class k; // K - shared secret

    // Remove carriage return  and line feed characters (\n and \r) from a string 
    void remove_cr_lf(std::string& str) {
        str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
        str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    }
};
#pragma once

#include <string>
#include <gmpxx.h> // GMP C++ interface
#include <vector>
#include <sstream>

class SshTypeHelper {
public:
    // Convert an mpz_class to an SSH mpint (multi-precision integer) string
    static std::string mpzToMpint(const mpz_class &num);
    // Convert an SSH mpint (multi-precision integer) string to an mpz_class
    static mpz_class mpintToMpz(const std::vector<uint8_t>& mpint);

    static std::string stringToSshString(const std::string& str);

   static std::string stringsVectorToSshNameList(const std::vector<std::string>& names);

    static void appendNameListToVector(std::vector<uint8_t>& vec, const std::vector<std::string>& names);

    static void appendAsSshStringToVector(std::vector<uint8_t>& vec, const std::string& str);

    static void appendAsMpintToVector(std::vector<uint8_t>& vec, const mpz_class& num);

    static uint32_t swapByteOrder(uint32_t original);

    // Read an SSH string from data starting at offset. Advances offset accordingly.
    static std::string readSshString(const std::vector<uint8_t>& data, size_t& offset);

    // read a comma-separated string, split it into a vector of names. advances offset accordingly.
    static std::vector<std::string> readSshStringsList(const std::vector<uint8_t>& data, size_t& offset);

    // read an unit32_t from data starting at offset. Advances offset accordingly.
    static uint32_t readUint32(const std::vector<uint8_t>& data, size_t& offset);

    // Write a uint32_t to a vector of bytes.
    static void writeUint32(std::vector<uint8_t>& data, uint32_t value);
};
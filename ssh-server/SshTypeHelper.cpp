#include "SshTypeHelper.h"


std::string SshTypeHelper::mpzToMpint(const mpz_class &num) 
{
    if (num == 0)
    {
        return std::string("\x00\x00\x00\x00", 4); // Zero case
    }

    mpz_class val = num;
    bool isNegative = val < 0;
    if (isNegative)
    {
        val = -val; // Convert to positive for processing
    }
    
    size_t size = (mpz_sizeinbase(val.get_mpz_t(), 2) + 7) / 8; // Calculate byte size
    std::vector<uint8_t> bytes(size);

    mpz_export(bytes.data(), &size, 1, 1, 1, 0, val.get_mpz_t()); // copy the value to row bytes

    // Handle sign extension (two's complement)
    if (isNegative) {
        for (size_t i = 0; i < size; ++i) bytes[i] = ~bytes[i];
        for (size_t i = size; i-- > 0;) {
            if (++bytes[i]) break;
        }
        if ((bytes[0] & 0x80) == 0) bytes.insert(bytes.begin(), 0xFF);
    } else {
        if ((bytes[0] & 0x80) != 0) bytes.insert(bytes.begin(), 0x00);
    }

    std::string result;
    uint32_t len = swapByteOrder(bytes.size()); // convert the length to network byte order
    result.append(reinterpret_cast<char *>(&len), 4); // add the length of the mpint
    result.append(reinterpret_cast<char *>(bytes.data()), bytes.size()); // add the integer value itself
    return result;
}

mpz_class SshTypeHelper::mpintToMpz(const std::vector<uint8_t>& mpint) 
{
    if (mpint.size() < 4)
    {
        throw std::invalid_argument("Invalid mpint format");
    }

    uint32_t len;
    std::memcpy(&len, mpint.data(), 4); // Read the length of the mpint
    len = swapByteOrder(len);

    if (mpint.size() != len + 4) 
    {
        throw std::invalid_argument("Size mismatch");
    }

    if (len == 0) // Zero case
    {
        return 0;
    }

    bool isNegative = (mpint[4] & 0x80) != 0; // check the sign bit (MSB)
    mpz_class result;
    mpz_import(result.get_mpz_t(), len, 1, 1, 1, 0, mpint.data() + 4); // copy the value to the result

    if (isNegative) // if the value is negative, convert it to two's complement
    {
        mpz_class twoComp = mpz_class(1) << (len * 8);
        result -= twoComp;
    }
    return result;
}

std::string SshTypeHelper::stringToSshString(const std::string& str) {
    uint32_t len = str.size();
    uint32_t netLen = swapByteOrder(len);
    uint8_t lenBytes[4];
    memcpy(lenBytes, &netLen, 4);
    std::string result;
    result.append(reinterpret_cast<char *>(&lenBytes), 4); // add the length of the mpint
    result.append(str); // add the integer value itself
    return result;
}

std::string SshTypeHelper::stringsVectorToSshNameList(const std::vector<std::string>& names) {
    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
            joined.push_back(',');
        }
        joined.append(names[i]);
    }
    return stringToSshString(joined);
}

void SshTypeHelper::appendNameListToVector(std::vector<uint8_t>& vec, const std::vector<std::string>& names) {
    std::string nameList = SshTypeHelper::stringsVectorToSshNameList(names);
    vec.insert(vec.end(), nameList.begin(), nameList.end());
}

void SshTypeHelper::appendAsSshStringToVector(std::vector<uint8_t>& vec, const std::string& str) {
    std::string sshStr = SshTypeHelper::stringToSshString(str);
    vec.insert(vec.end(), sshStr.begin(), sshStr.end());
}

void SshTypeHelper::appendAsMpintToVector(std::vector<uint8_t>& vec, const mpz_class& num) {
    std::string mpint = SshTypeHelper::mpzToMpint(num);
    vec.insert(vec.end(), mpint.begin(), mpint.end());
}

uint32_t SshTypeHelper::swapByteOrder(uint32_t original) 
{
    return ((original & 0x000000FFU) << 24) |
        ((original & 0x0000FF00U) << 8)  |
        ((original & 0x00FF0000U) >> 8)  |
        ((original & 0xFF000000U) >> 24);
}

std::string SshTypeHelper::readSshString(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) {
        throw std::runtime_error("Buffer too short to read SSH string length");
    }
    uint32_t netLen;
    memcpy(&netLen, data.data() + offset, 4);
    uint32_t len = SshTypeHelper::swapByteOrder(netLen);
    offset += 4;
    if (offset + len > data.size()) {
        throw std::runtime_error("Buffer too short to read SSH string");
    }
    std::string str(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return str;
}

std::vector<std::string> SshTypeHelper::readSshStringsList(const std::vector<uint8_t>& data, size_t& offset) {
    std::string nameListStr = readSshString(data, offset);
    std::vector<std::string> result;
    std::istringstream iss(nameListStr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}


uint32_t SshTypeHelper::readUint32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) {
        throw std::runtime_error("Buffer too short to read uint32_t");
    }
    uint32_t val;
    memcpy(&val, data.data() + offset, 4);
    offset += 4;
    return SshTypeHelper::swapByteOrder(val);
}

void SshTypeHelper::writeUint32(std::vector<uint8_t>& data, uint32_t value) {
    uint32_t netValue = SshTypeHelper::swapByteOrder(value);
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&netValue), reinterpret_cast<uint8_t*>(&netValue) + 4);
}
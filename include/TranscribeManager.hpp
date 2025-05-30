#ifndef TRANSCRIBEMANAGER_HPP_
#define TRANSCRIBEMANAGER_HPP_

#include <string>
#include <vector>

class TranscribeManager {
public:
    static std::string getSignedWebsocketUrl(
            const std::string& accessKey, const std::string& secretKey, const std::string& region, const std::string& language);

    static bool parseResponse(const std::string& response, std::string& payload, bool& isError, bool verbose = false);

    static bool makeRequest(std::string& request, const std::vector<uint8_t>& data);
private:
    static std::string getSha256(std::string str);
    static void getSignatureKey(unsigned char *signatureKey, const std::string& secretKey,
            const std::string& datestamp, const std::string& region, const std::string& service);
    static void getHMAC(unsigned char *hmac, unsigned char *key, int keyLen, const std::string& str);
    static std::string toHex(unsigned char *hmac);

    static bool verifyCRC(const char* buffer, const uint32_t totalLength);
    static void parseHeader(const char** buffer, bool& isError, bool verbose = false);

    static void writeHeader(char** buffer, const std::string& key, const std::string& val);
};

#endif /* TRANSCRIBEMANAGER_HPP_ */
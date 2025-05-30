#include "TranscribeManager.hpp"
#include "CRC.h"

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <iomanip>
#include <regex>
#include <iostream>
#include <byteswap.h>

using namespace std;

// see
// https://docs.aws.amazon.com/transcribe/latest/dg/websocket.html#websocket-url
// https://docs.aws.amazon.com/transcribe/latest/dg/event-stream.html

string TranscribeManager::getSignedWebsocketUrl(const string& accessKey,
        const string& secretKey, const string& region, const string& language) {
    string method = "GET";
    string service = "transcribe";
    string endpoint = "wss://transcribestreaming." + region + ".amazonaws.com:8443";
    string host = "transcribestreaming." + region + ".amazonaws.com:8443";

    time_t now = time(0);
    tm *gmtm = gmtime(&now);

    char amzDate[21];
    snprintf (amzDate, 21, "%04d%02d%02dT%02d%02d%02dZ",
            1900 + gmtm->tm_year, 1 + gmtm->tm_mon, gmtm->tm_mday,
            gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec);

    char datestamp[9];
    snprintf (datestamp, 9, "%04d%02d%02d", 1900 + gmtm->tm_year, 1 + gmtm->tm_mon, gmtm->tm_mday);

    string canonical_uri = "/stream-transcription-websocket";
    string canonical_headers = "host:" + host + "\n";
    string signed_headers = "host";
    string algorithm = "AWS4-HMAC-SHA256";
    string credential_scope = string(datestamp) + "%2F" + region + "%2F" + service + "%2F" + "aws4_request";

    string canonical_querystring  = "X-Amz-Algorithm=" + algorithm;
    canonical_querystring += "&X-Amz-Credential="+ accessKey + "%2F" + credential_scope;
    canonical_querystring += "&X-Amz-Date=" + string(amzDate);
    canonical_querystring += "&X-Amz-Expires=300";
    canonical_querystring += "&X-Amz-SignedHeaders=" + signed_headers;
    canonical_querystring += "&language-code=" + language;
    canonical_querystring += "&media-encoding=pcm&sample-rate=8000";
    
    string payload_hash = getSha256("");

    string canonical_request = method + '\n'
       + canonical_uri + '\n'
       + canonical_querystring + '\n'
       + canonical_headers + '\n'
       + signed_headers + '\n'
       + payload_hash;

    string string_to_sign = algorithm + "\n"
       + amzDate + "\n"
       + regex_replace(credential_scope, regex("%2F"), "/") + "\n"
       + getSha256(canonical_request);

    //sample:
    //getSignatureKey(signing_key, "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY", "20120215", "us-east-1", "iam");
    unsigned char signing_key[SHA256_DIGEST_LENGTH];
    getSignatureKey(signing_key, secretKey, datestamp, region, service);

    unsigned char signatureBinary[SHA256_DIGEST_LENGTH];
    getHMAC(signatureBinary, signing_key, SHA256_DIGEST_LENGTH, string_to_sign);
    string signature = toHex(signatureBinary);

    canonical_querystring += "&X-Amz-Signature=" + signature;
    string request_url = endpoint + canonical_uri + "?" + canonical_querystring;

    return request_url;
}

string TranscribeManager::getSha256(string str) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, str.c_str(), str.length());
    unsigned char hash[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_Final(hash, &ctx);

    ostringstream os;
    os << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        os << setw(2) << static_cast<unsigned int>(hash[i]);
    }

    return os.str();
}

void TranscribeManager::getSignatureKey(unsigned char *signatureKey, const string& secretKey,
            const string& datestamp, const string& region, const string& service) {
    string key = string("AWS4") + secretKey;
    unsigned char kDate[SHA256_DIGEST_LENGTH];
    unsigned char kRegion[SHA256_DIGEST_LENGTH];
    unsigned char kService[SHA256_DIGEST_LENGTH];
    unsigned char kSigning[SHA256_DIGEST_LENGTH];
    getHMAC(kDate, (unsigned char *)key.c_str(), key.length(), datestamp);
    getHMAC(kRegion, kDate, SHA256_DIGEST_LENGTH, region);
    getHMAC(kService, kRegion, SHA256_DIGEST_LENGTH, service);
    getHMAC(kSigning, kService, SHA256_DIGEST_LENGTH, "aws4_request");

    memcpy(signatureKey, kSigning, SHA256_DIGEST_LENGTH);
}

void TranscribeManager::getHMAC(unsigned char *hmac, unsigned char *key, int keyLen, const string& str) {
    unsigned char *data = (unsigned char*)str.c_str();
    unsigned char *result = HMAC(EVP_sha256(), key, keyLen, data, strlen((char *)data), NULL, NULL);
    memcpy(hmac, result, SHA256_DIGEST_LENGTH);
}

string TranscribeManager::toHex(unsigned char *hmac) {
    ostringstream os;
    os << hex << setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        os << setw(2) << static_cast<unsigned int>(hmac[i]);
    }

    return os.str();
}

///////////////////////////////////////////////////////////////////////////////////////////

bool TranscribeManager::parseResponse(const string& response, string& payload, bool& isError, bool verbose) {
    const char* buffer = response.c_str();

    uint32_t totalLen;
    memcpy(&totalLen, &buffer[0], sizeof(uint32_t));
    totalLen = bswap_32(totalLen);

    uint32_t headerLen;
    memcpy(&headerLen, &buffer[4], sizeof(uint32_t));
    headerLen = bswap_32(headerLen);

    if (!verifyCRC(buffer, totalLen)) {
        return false;
    }

    buffer += 12; // bytes 0 - 11 are prelude

    const int numberOfHeaders = 3;
    for (int i = 0; i < numberOfHeaders; i++) {
        parseHeader(&buffer, isError, verbose);
    }

    payload = string(buffer, totalLen - headerLen - 4*4);

    return true;
}

bool TranscribeManager::verifyCRC(const char* buffer, const uint32_t totalLength) {
    uint32_t preludeCRC;
    memcpy(&preludeCRC, &buffer[8], 4);
    preludeCRC = bswap_32(preludeCRC);

    uint32_t calculatedPreludeCRC = CRC::Calculate(&buffer[0], 8, CRC::CRC_32());
    if (calculatedPreludeCRC != preludeCRC) {
        cout << "Prelude CRC didn't match!" << endl;
        return false;
    }

    uint32_t messageCRC;
    memcpy(&messageCRC, &buffer[totalLength - 4], 4);
    messageCRC = bswap_32(messageCRC);

    uint32_t calculatedMessageCRC = CRC::Calculate(buffer, totalLength - 4, CRC::CRC_32());

    if (calculatedMessageCRC != messageCRC) {
        cout << "Message CRC didn't match!" << endl;
        return false;
    }

    return true;
}

void TranscribeManager::parseHeader(const char** buffer, bool& isError, bool verbose) {
    uint8_t headerNameLen;
    memcpy(&headerNameLen, *buffer, sizeof(uint8_t));
    (*buffer)++;

    string headerName(*buffer, headerNameLen);
    *buffer += headerNameLen;

    uint8_t headerType;
    memcpy(&headerType, *buffer, sizeof(uint8_t));
    (*buffer)++;

    uint16_t headerValLen;
    memcpy(&headerValLen, *buffer, sizeof(uint16_t));
    headerValLen = bswap_16(headerValLen);
    *buffer += 2;

    string headerVal(*buffer, headerValLen);
    *buffer += headerValLen;

    if (headerVal == "exception") {
        isError = true;
    }
    if (verbose) {
        cout << headerName << "(" << (int)headerType << "): " << headerVal << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool TranscribeManager::makeRequest(string& request, const vector<uint8_t>& data) {
    char preludeAndCrcBuffer[4*3];
    char headerBuffer[88];
    char messageCrcBuffer[4];

    // prelude
    uint32_t totalLen = sizeof(preludeAndCrcBuffer) + sizeof(headerBuffer) + data.size() + sizeof(messageCrcBuffer);
    uint32_t headerLen = sizeof(headerBuffer);
    totalLen = bswap_32(totalLen);
    headerLen = bswap_32(headerLen);

    memcpy(&preludeAndCrcBuffer[0], &totalLen, sizeof(uint32_t));
    memcpy(&preludeAndCrcBuffer[4], &headerLen, sizeof(uint32_t));

    uint32_t preludeCRC = CRC::Calculate(&preludeAndCrcBuffer[0], 8, CRC::CRC_32());
    preludeCRC = bswap_32(preludeCRC);
    memcpy(&preludeAndCrcBuffer[8], &preludeCRC, sizeof(uint32_t));

    // header
    char* buffer = headerBuffer;
    writeHeader(&buffer, ":content-type", "application/octet-stream");
    writeHeader(&buffer, ":event-type", "AudioEvent");
    writeHeader(&buffer, ":message-type", "event");

    // write everything to response string except for the message CRC
    request.append(preludeAndCrcBuffer, sizeof(preludeAndCrcBuffer));
    request.append(headerBuffer, sizeof(headerBuffer));
    request.append(data.begin(), data.end());

    // message CRC
    uint32_t messageCRC = CRC::Calculate(request.c_str(), request.length(), CRC::CRC_32());
    messageCRC = bswap_32(messageCRC);
    memcpy(messageCrcBuffer, &messageCRC, sizeof(uint32_t));

    // write message CRC to response string
    request.append(messageCrcBuffer, sizeof(messageCrcBuffer));

    return true;
}

void TranscribeManager::writeHeader(char** buffer, const string& key, const string& val) {
    uint8_t keyLen = key.length();
    memcpy(*buffer, &keyLen, sizeof(uint8_t));
    (*buffer)++;

    memcpy(*buffer, key.c_str(), keyLen);
    (*buffer) += keyLen;

    uint8_t valueType = 7;
    memcpy(*buffer, &valueType, sizeof(uint8_t));
    (*buffer)++;

    uint16_t valLen = bswap_16(val.length());
    memcpy(*buffer, &valLen, sizeof(uint16_t));
    (*buffer) += 2;

    memcpy(*buffer, val.c_str(), val.length());
    (*buffer) += val.length();
}
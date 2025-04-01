
#ifndef TTSCACHE_H
#define TTSCACHE_H

#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>  // For SHA-256 hashing

class TTSCache {
public:
    static TTSCache& getInstance();

    bool isCached(const std::string& key);
    std::vector<uint8_t> getCachedAudio(const std::string& key);
    void saveToCache(const std::string& key, const std::vector<uint8_t>& audioData);

    static std::string generateHash(const std::string& vendor, const std::string& voiceName, const std::string& text);
    static std::string getCacheFilePath(const std::string& hash);

private:
    TTSCache();
    ~TTSCache();

    std::unordered_map<std::string, std::vector<uint8_t>> memoryCache;
    std::list<std::string> cacheOrder;
    std::queue<std::pair<std::string, std::vector<uint8_t>>> fileWriteQueue;
    size_t maxCacheSize = 100; // Keep only 100 items in-memory
    bool stopThreads;
    size_t threadCount; // Number of worker threads
    std::vector<std::thread> workers;
    std::mutex cacheMutex;
    std::mutex queueMutex;
    std::condition_variable queueCondition;

    void evictOldest();
    void fileWriterThread();
};

#endif // TTSCACHE_H

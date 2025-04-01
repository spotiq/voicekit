#include "TTSCache.h"

#define CACHE_DIR "./tts_cache/"

// Singleton instance
TTSCache& TTSCache::getInstance() {
    static TTSCache instance;
    return instance;
}

// Constructor ensures cache directory exists and starts worker threads
TTSCache::TTSCache() : stopThreads(false), threadCount(2) {
    try {
        if (!std::filesystem::exists(CACHE_DIR)) {
            std::filesystem::create_directory(CACHE_DIR);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create cache directory: " + std::string(e.what()));
    }
    
    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back(&TTSCache::fileWriterThread, this);
    }
}

// Destructor ensures all threads stop and finish processing
TTSCache::~TTSCache() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopThreads = true;
    }
    queueCondition.notify_all();
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// Generate SHA-256 hash for file naming
std::string TTSCache::generateHash(const std::string& vendor, const std::string& voiceName, const std::string& text) {
    std::string input = vendor + voiceName + text;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

// Get file path from hash
std::string TTSCache::getCacheFilePath(const std::string& hash) {
    return std::string(CACHE_DIR) + hash + ".raw";
}

// Check if audio is cached (memory or disk)
bool TTSCache::isCached(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (memoryCache.find(key) != memoryCache.end()) {
        return true;
    }
    return std::filesystem::exists(getCacheFilePath(key));
}

// Retrieve audio from cache (memory or disk)
std::vector<uint8_t> TTSCache::getCachedAudio(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    if (memoryCache.find(key) != memoryCache.end()) {
        return memoryCache[key];
    }
    
    std::ifstream file(getCacheFilePath(key), std::ios::binary);
    if (!file) return {};
    
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Save audio to cache (memory and queue for async disk writing)
void TTSCache::saveToCache(const std::string& key, const std::vector<uint8_t>& audioData) {
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (memoryCache.size() >= maxCacheSize) {
            evictOldest();
        }
        memoryCache[key] = audioData;
        cacheOrder.push_back(key);
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        fileWriteQueue.emplace(key, audioData);
    }
    queueCondition.notify_one();
}

// Worker thread function to process file writes
void TTSCache::fileWriterThread() {
    while (true) {
        std::pair<std::string, std::vector<uint8_t>> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] { return stopThreads || !fileWriteQueue.empty(); });
            if (stopThreads && fileWriteQueue.empty()) return;
            task = std::move(fileWriteQueue.front());
            fileWriteQueue.pop();
        }
        try {
            std::ofstream file(getCacheFilePath(task.first), std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file for writing: " + getCacheFilePath(task.first));
            }
            file.write(reinterpret_cast<const char*>(task.second.data()), task.second.size());
        } catch (const std::exception& e) {
            std::cerr << "Error writing to file: " << e.what() << std::endl;
        }
    }
}

// Remove oldest cache entry
void TTSCache::evictOldest() {
    if (cacheOrder.empty()) return;
    std::string oldestKey = cacheOrder.front();
    cacheOrder.pop_front();
    memoryCache.erase(oldestKey);
}
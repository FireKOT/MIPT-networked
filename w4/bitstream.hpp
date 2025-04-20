#pragma once


#include <cstdint>
#include <string>
#include <cstring>
#include <assert.h>


class Bitstream {

public:

    Bitstream (uint64_t defaultCapacity = 1024):
    
      data_(nullptr),
      writePtr_(nullptr),
      readPtr_(nullptr),
      capacity_(defaultCapacity) {

        data_ = reinterpret_cast<char*>(calloc(capacity_, sizeof(char)));
        writePtr_ = data_;
        readPtr_ = data_;
    }

    Bitstream (void *data, uint64_t dataSize):
    
      data_(nullptr),
      writePtr_(nullptr),
      readPtr_(nullptr),
      capacity_(dataSize) {

        data_ = reinterpret_cast<char*>(calloc(capacity_, sizeof(char)));
        memcpy(data_, data, dataSize);

        writePtr_ = data_ + dataSize;
        readPtr_ = data_;
    }


    ~Bitstream () {

        free(data_);
    }


    template <typename T>
    void write (const T &val) {

        resizeIfNeed<T>();
    
        memcpy(writePtr_, &val, sizeof(T));
        writePtr_ += sizeof(T);
    }


    template <typename T>
    void read (T &val) {

        assert(isCorrectRead<T>());
    
        T *ptr = reinterpret_cast<T*>(readPtr_);
        val = *ptr;
    
        readPtr_ += sizeof(T);
    }

    template <typename T>
    void skip () {

        assert(isCorrectRead<T>());    
        readPtr_ += sizeof(T);
    }

    void dumpToMem (void *dest) const {

        mempcpy(dest, data_, size());
    }

    uint64_t size () const {

        return writePtr_ - data_;
    }

    void* data () {

        return reinterpret_cast<void*>(data_);
    }

    const void* data () const {

        return reinterpret_cast<const void*>(data_);
    }

private:

    uint64_t kResizeFactor = 2;

    template <typename T>
    constexpr void resizeIfNeed () {

        uint64_t neccesarySize = size() + sizeof(T);
        if (neccesarySize < capacity_) return;

        while (neccesarySize >= capacity_) {

            capacity_ *= kResizeFactor;
        }

        uint64_t writeSize = writePtr_ - data_;
        uint64_t readSize  = readPtr_  - data_;

        data_ = reinterpret_cast<char*>(realloc(data_, capacity_));

        writePtr_ = data_ + writeSize;
        readPtr_  = data_ + readSize;
    }

    template <typename T>
    constexpr bool isCorrectRead () const {

        assert(readPtr_ < writePtr_);

        return (writePtr_ - readPtr_) >= sizeof(T);
    }

private:

    char *data_;
    char *writePtr_;
    char *readPtr_;
    
    uint64_t capacity_;
};  

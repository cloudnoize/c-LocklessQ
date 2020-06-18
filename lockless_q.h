#include <atomic>
#include <cassert>
#include <string.h>

// Single Consumer Single producer queue.
class Lockless_q {
  char *buff_ = nullptr;
  // Index of the next cell to read.
  std::atomic_long readIx_ = 0;
  // Index of the next cell to write
  std::atomic_long writeIdx_ = 0;
  std::atomic_int diff = 0;

public:
  const int size_ = 0;

  Lockless_q(int size) : size_(size) { buff_ = new char[size]; }
  ~Lockless_q() { delete[] buff_; }

  // Can't read more than buffer size or writer - reader.
  virtual int read(char *inBuff, const int inSize) { // ADD ASSERTS AS IN READ
    // First thing, record writer value atomicaaly, to maintina the invariant.
    auto wrPos = writeIdx_.load();
    // At this phase only this method can change the reader index.
    // So we are safe in term that diff can only increase.
    auto rdPos = readIx_.load();
    auto avlble = wrPos - rdPos;
    // Available can't be bigger than size, it means that we override samples that we didn't read.
    assert(avlble <= size_);

    // Can we read all requested or only partial.
    auto sizeToRead = avlble >= inSize ? inSize : avlble;

    // Actuall location of the reader index.
    auto physcalIdx = rdPos % size_;
    auto avlbleReadTillEnd = size_ - physcalIdx;
    auto diff = avlbleReadTillEnd - sizeToRead;

    if (diff >= 0) {
      // Copy in one shot
      memcpy(inBuff, &buff_[physcalIdx], sizeToRead);
    } else {
      // read till end
      memcpy(inBuff, &buff_[physcalIdx], avlbleReadTillEnd);
      // read remain from beginning
      auto absDiff = diff * (-1);
      memcpy(inBuff + avlbleReadTillEnd, buff_, absDiff);
    }
    // At this phase it's safe to increment
    readIx_ += sizeToRead;
    return sizeToRead;
  }

  // Can't write on bytes we didn't read yet.
  virtual int write(const char *inBuff, const int inSize) {
    // First thing, record reader value atomicaaly, to maintain the invariant.
    auto rdPos = readIx_.load();

    // At this phase only this method can change the writer index.
    // So we are safe in term that available can only increase.
    auto wrPos = writeIdx_.load();

    // We can't write on  bytes that we didn't read yet.
    auto wasntReadYet = wrPos - rdPos;
    auto avlble = size_ - wasntReadYet;

    // If negative it means that reader, read unwritten values
    assert(avlble >= 0);

    auto sizeToWrite = avlble >= inSize ? inSize : avlble;

    // Actuall location of the reader index.
    auto physcalIdx = wrPos % size_;
    auto avlbleWriteTillEnd = size_ - physcalIdx;
    auto diff = avlbleWriteTillEnd - sizeToWrite;

    if (diff >= 0) { // ASSERTSSHOULD LESS THAN OR LEASS THAN EQ ?
      // Copy in one shot
      assert(physcalIdx + sizeToWrite <= size_);
      memcpy(&buff_[physcalIdx], inBuff, sizeToWrite);
    } else {
      // write till end
      assert(physcalIdx + avlbleWriteTillEnd <= size_);
      memcpy(&buff_[physcalIdx], inBuff, avlbleWriteTillEnd);
      // read remain from beginning
      auto absDiff = diff * (-1);
      assert(avlbleWriteTillEnd + absDiff <= inSize);
      memcpy(buff_, inBuff + avlbleWriteTillEnd, absDiff);
    }

    // At this phase it's safe to increment
    writeIdx_ += sizeToWrite;
    return sizeToWrite;
  }
};
;
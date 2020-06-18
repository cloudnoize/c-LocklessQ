#include <lockless_q.h>

#include "gtest/gtest.h"
#include <chrono>
#include <random>
#include <thread>

TEST(lockless, size) {
  Lockless_q q{44100};
  EXPECT_EQ(q.size_, 44100);
}

TEST(lockless, Simple_produce_consume) {
  Lockless_q q{44100};
  auto v = int32_t(8);
  auto c = int32_t(0);
  q.write((char *)&v, 4);
  q.read((char *)&c, 4);
  EXPECT_EQ(c, v);
}

TEST(lockless, Write_all_mod0) {
  Lockless_q q{8};
  auto v = int32_t(8);
  auto c = int32_t(9);
  auto d = int32_t(10);
  auto n = q.write((char *)&v, 4);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&c, 4);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&d, 4);
  EXPECT_EQ(n, 0);
}

TEST(lockless, Write_all_mod2) {
  Lockless_q q{10};
  auto v = int32_t(8);
  auto c = int32_t(9);
  auto d = int32_t(10);
  auto n = q.write((char *)&v, 4);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&c, 4);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&d, 4);
  EXPECT_EQ(n, 2);
  n = q.write((char *)&d, 4);
  EXPECT_EQ(n, 0);
}

TEST(lockless, bigreader) {
  Lockless_q q{5};
  auto v = int32_t(8);
  auto c = int32_t(9);
  auto d = int32_t(10);
  char buff[100];
  auto n = q.write((char *)&v, 4);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&c, 4);
  EXPECT_EQ(n, 1);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 5);
  n = q.write((char *)&v, 4);
  EXPECT_EQ(n, 4);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 4);
  n = q.write((char *)&v, 1);
  EXPECT_EQ(n, 1);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 1);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 0);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 0);
}

TEST(lockless, bigwriter) {
  Lockless_q q{5};
  auto v = int32_t(8);
  auto c = int32_t(9);
  auto d = int32_t(10);
  char buff[100];
  auto n = q.write((char *)&v, 4);
  EXPECT_EQ(n, 4);
  n = q.write(buff, 100);
  EXPECT_EQ(n, 1);
  n = q.read(buff, 2);
  EXPECT_EQ(n, 2);
  n = q.write(buff, 100);
  EXPECT_EQ(n, 2);
  n = q.read(buff, 100);
  EXPECT_EQ(n, 5);
  n = q.write(buff, 100);
  EXPECT_EQ(n, 5);
  n = q.read(buff, 1);
  EXPECT_EQ(n, 1);
  n = q.write(buff, 100);
  EXPECT_EQ(n, 1);
}
TEST(lockless, read_partial_available) {
  Lockless_q q{44100};
  auto v = int32_t(8);
  auto v2 = int8_t(8);
  auto c = int32_t(0);
  auto c2 = int32_t(0);
  q.write((char *)&v, 4);
  q.write((char *)&v2, 1);
  auto n = q.read((char *)&c, 4);
  EXPECT_EQ(n, 4);
  EXPECT_EQ(c, v);

  n = q.read((char *)&c2, 4);
  EXPECT_EQ(n, 1);
  EXPECT_EQ(c2, v2);
}

TEST(lockless, puser_reader) {
  std::mt19937_64 eng{std::random_device{}()};
  std::uniform_int_distribution<> dist{0, 10};
  Lockless_q q{44100};
  int32_t arr[1000];
  memset((char *)arr, 0, 4000);

  auto pusher = [&]() {
    for (int32_t i = 0; i < 1000; i++) {
      q.write((char *)&i, 4);
      std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
    }
  };
  auto reader = [&]() {
    int idx = 0;
    char buff[4];
    while (true) {
      auto n = q.read(buff, 4);
      auto nums = n / 4;
      auto mod = n % 4;
      EXPECT_EQ(0, mod);
      for (int i = 0; i < nums; i++, idx++) {
        arr[idx] = *((int32_t *)&buff[i * 4]);
        std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});
      }
      if (idx == 1000) {
        break;
      }
    }
  };

  std::thread t2(reader);
  std::thread t1(pusher);

  t1.join();
  t2.join();

  for (int i = 0; i < 1000; i++) {
    EXPECT_EQ(i, arr[i]);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
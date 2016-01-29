// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerialBench.h"
#include "Encryption.h"
#include <iostream>
#include <memory>

struct BenchmarkEntry {
  BenchmarkEntry(const char* name, IBenchmark* benchmark) :
    name(name),
    benchmark(benchmark)
  {}

  const char* name;
  std::unique_ptr<IBenchmark> benchmark;
};

static BenchmarkEntry benchmarks[] = {
  { "encryption", new Encryption }
};

static void PrintUsage(void) {
  std::cout << "Available benchmarks:" << std::endl;
  for (const auto& benchmark : benchmarks)
    std::cout << ' ' << benchmark.name << std::endl;
}

int main(int argc, const char* argv[]) {
  if (argc != 2)
    return PrintUsage(), -1;

  for(auto& cur : benchmarks)
    if (!strcmp(argv[1], cur.name))
      return cur.benchmark->Benchmark(std::cout);
  return -1;
}

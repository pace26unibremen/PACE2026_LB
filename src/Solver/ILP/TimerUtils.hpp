#pragma once
#include <chrono>
#include <iostream>

#define TIMER_START(name) auto name = std::chrono::steady_clock::now();
#define TIMER_LOG(name, label) std::clog << (label) << ": " \
    << std::chrono::duration_cast<std::chrono::milliseconds>( \
       std::chrono::steady_clock::now() - name).count() << " ms\n";
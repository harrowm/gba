#pragma once

#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"
#include <sstream>
#include <set>

// Common helper functions for all CPU tests
std::string serializeCPUState(const CPU& cpu);
void validateUnchangedRegisters(const CPU& cpu, const std::string& beforeState, const std::set<int>& changedRegisters);

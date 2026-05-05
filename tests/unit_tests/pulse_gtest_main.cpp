// Copyright (c) 2026, The Arqma Network
//
// Minimal gtest entrypoint for ARQMA_CI_PULSE_GTEST (Pulse-only unit_tests binary).

#include "gtest/gtest.h"

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

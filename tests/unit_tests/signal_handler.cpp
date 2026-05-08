#include "gtest/gtest.h"

#include "common/util.h"

TEST(signal_handler, recognizes_termination_signals)
{
#if defined(WIN32)
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(CTRL_C_EVENT));
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(CTRL_BREAK_EVENT));
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(CTRL_CLOSE_EVENT));
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(CTRL_LOGOFF_EVENT));
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(CTRL_SHUTDOWN_EVENT));
  EXPECT_FALSE(tools::signal_handler::is_termination_signal(0));
#else
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(SIGINT));
  EXPECT_TRUE(tools::signal_handler::is_termination_signal(SIGTERM));
  EXPECT_FALSE(tools::signal_handler::is_termination_signal(SIGPIPE));
#endif
}

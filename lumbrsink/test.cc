#include "gtest/gtest.h"

#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"

#include "lumbrsink/sink.h"
#include "lumbrsink/testable_logserver.h"


TEST(SINK, Post)
{
	std::condition_variable cv;
	bool server_graceful_shutdown = false;
	TestService service(cv);
	std::thread server_starter(
			[&service,&server_graceful_shutdown]
			{
				server_graceful_shutdown = service.run("0.0.0.0", 5000);
			});

	lumbr::LumbrSink my_sink("http://localhost:5000");
	absl::AddLogSink(&my_sink);

	LOG(INFO) << "Testing remote logging!";
	LOG(ERROR) << "This is a serious database entry.";

	// wait 5 seconds or when 2 log entries were received
	std::thread(
			[&cv,&service]
			{
				std::unique_lock<std::mutex> lock(service.mtx_);
				cv.wait_for(lock, std::chrono::seconds(5),
						[&service]{ return service.entries_.size() > 2; });
			}).join();

	// check graceful shutdown
	EXPECT_EQ(2, service.entries_.size());
	// check entry 0
	auto& entry0 = service.entries_[0];
	ASSERT_FALSE(entry0.is_error);
	EXPECT_STREQ("INFO", entry0.severity.c_str());
	EXPECT_STREQ("lumbrsink/test.cc", entry0.file.c_str());
	EXPECT_EQ(25, entry0.line);
	EXPECT_STREQ("Testing remote logging!", entry0.message.c_str());
	// check entry 1
	auto& entry1 = service.entries_[1];
	ASSERT_FALSE(entry1.is_error);
	EXPECT_STREQ("ERROR", entry1.severity.c_str());
	EXPECT_STREQ("lumbrsink/test.cc", entry1.file.c_str());
	EXPECT_EQ(26, entry1.line);
	EXPECT_STREQ("This is a serious database entry.", entry1.message.c_str());

	// shutdown server
	service.server_.stop();
	server_starter.join();

	EXPECT_TRUE(server_graceful_shutdown);
}


int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	absl::InitializeLog();
	return RUN_ALL_TESTS();
}

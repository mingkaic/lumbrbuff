#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"

#include "lumbrsink/sink.h"

ABSL_FLAG(std::string, server_addr, "http://localhost:5000", "Target test server address");

int main(int argc, char** argv)
{
	absl::ParseCommandLine(argc, argv);
	std::string server_addr = absl::GetFlag(FLAGS_server_addr);

	absl::InitializeLog();
	lumbr::LumbrSink my_sink(server_addr);
	absl::AddLogSink(&my_sink);

	LOG(INFO) << "Lorem ipsum dolor sit amet, consectetur adipiscing elit";
	LOG(WARNING) << "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua";
	LOG(ERROR) << "Ut enim ad minim veniam";
	LOG(INFO) << "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat";

	return 0;
}

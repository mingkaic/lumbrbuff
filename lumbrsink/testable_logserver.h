
#ifndef TESTABLE_LOGSERVER_H
#define TESTABLE_LOGSERVER_H

#include "nlohmann/json.hpp"

#include "httplib.h"

class TestService
{
public:
	struct LogEntries
	{
		bool is_error;
		std::string severity;
		std::string timestamp;
		std::string file;
		int line;
		std::string message;
	};

	TestService (std::condition_variable& cvar) : cvar_(std::ref(cvar)) {}

	void Post (const httplib::Request& req, httplib::Response& res)
	{
		try
		{
			auto j = nlohmann::json::parse(req.body);
			std::string severity  = j["severity"];
			std::string timestamp = j["timestamp"];
			std::string file      = j["file"];
			int line              = j["line"];
			std::string message   = j["message"];

			res.status = 200;
			res.set_content("Log Received", "text/plain");
			std::lock_guard<std::mutex> lock(mtx_);
			entries_.push_back(
				LogEntries{
					.is_error=false,
					.severity=severity,
					.timestamp=timestamp,
					.file=file,
					.line=line,
					.message=message,
				});
		}
		catch (const std::exception& e)
		{
			res.status = 400;
			std::lock_guard<std::mutex> lock(mtx_);
			entries_.push_back(LogEntries{.is_error=true});
		}
	}

	bool run (const std::string addr, int port)
	{
		server_.Post("/logs",
				[this](
					const httplib::Request& req,
					httplib::Response& res)
				{
					this->Post(req, res);
				});
		if (!server_.listen(addr, port))
		{
			return false;
		}
		return true;
	}

	httplib::Server server_;

	std::condition_variable& cvar_;

	std::mutex mtx_;

	std::vector<LogEntries> entries_;
};

#endif // TESTABLE_LOGSERVER_H

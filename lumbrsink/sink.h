
#ifndef LUMBR_SINK_H
#define LUMBR_SINK_H

#include <string>

#include "absl/log/log_sink.h"
#include "absl/log/log_entry.h"
#include "absl/log/log.h"

#include "httplib.h"

namespace lumbr
{

class LumbrSink : public absl::LogSink
{
public:
	LumbrSink (const std::string& server_url) : url_(server_url), cli_(server_url) {}

	void Send (const absl::LogEntry& entry) override
	{
		std::string json_payload = "{";
		json_payload += "\"severity\":\"" + std::string(absl::LogSeverityName(
					entry.log_severity())) + "\",";
		json_payload += "\"timestamp\":\"" + absl::FormatTime(entry.timestamp()) + "\",";
		json_payload += "\"file\":\"" + std::string(entry.source_filename()) + "\",";
		json_payload += "\"line\":" + std::to_string(entry.source_line()) + ",";
		json_payload += "\"message\":\"" + std::string(entry.text_message()) + "\"";
		json_payload += "}";


		cli_.Post("/logs", json_payload, "application/json");
	}

private:
	std::string url_;

	httplib::Client cli_;
};

} // namespace lumbr

#endif // LUMBR_SINK_H

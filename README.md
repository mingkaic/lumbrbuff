# Lumbrbuff 💪

Send abseil logs to a http server.

When to use:
- using abseil logs. (https://abseil.io/docs/cpp/guides/logging)
- don't care about security.
- log source machine keeps dying then corrupting the logs because of CPU/memory/storage constraints.
- willing to use bazel

# How to use

On a machine that you TRUST run `bazel run //lumbrpile:app`.
This runs a POST endpoint on :5000/logs to receive abseil logs.
It also hosts serves a dashboard on :5000/ that shows the persisted log entries in real time.

WARNING: as of now, the lumbrpile app server is written entirely by AI. Use caution.

On the log source binary add `//lumbrsink` as a dependency.
From the source main, initialize abseil logs and add the sink as follows:

```
#include "lumbrsink/sink.h"


int main() {
    ...
    absl::InitializeLog();
	lumbr::LumbrSink my_sink("http://{server_addr}:5000");
	absl::AddLogSink(&my_sink);

    ...
}
```

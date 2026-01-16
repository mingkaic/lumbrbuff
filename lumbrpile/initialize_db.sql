CREATE TABLE log_entries (
    id SERIAL PRIMARY KEY,
    severity VARCHAR(20),
    timestamp TIMESTAMPTZ,
    file TEXT,
    line INT,
    message TEXT
);

CREATE INDEX idx_logs_timestamp ON log_entries (timestamp DESC);

package main

import (
	"database/sql"
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/gin-gonic/gin"

	_ "github.com/lib/pq" // PostgreSQL driver
)

type (
	LogEntry struct {
		ID        int       `json:"id" db:"id"`
		Severity  string    `json:"severity" db:"severity"`
		Timestamp time.Time `json:"timestamp" db:"timestamp"`
		File      string    `json:"file" db:"file"`
		Line      int       `json:"line" db:"line"`
		Message   string    `json:"message" db:"message"`
	}

	dbParams []string
)

var (
	db *sql.DB

	dbUser = flag.String("db_user", "", "postgres username")
	dbPwd  = flag.String("db_pwd", "", "postgres password")
	dbHost = flag.String("db_host", "", "postgres db host")
	dbPort = flag.Int("db_port", 0, "postgres db port")

	// Mandatory field
	dbTable = flag.String("db_table", "log_entries", "postgres db table name")
)

const defaultLimit = 100

func (p dbParams) AddStringFlag(key string, val *string) dbParams {
	if *val != "" {
		return append(p, key+"="+*val)
	}
	return p
}

func (p dbParams) AddIntFlag(key string, val *int) dbParams {
	if *val != 0 {
		return append(p, fmt.Sprintf("%s=%d", key, *val))
	}
	return p
}

func (p dbParams) String() string {
	return strings.Join(p, " ")
}

func main() {
	flag.Parse()

	var err error

	pgParams := dbParams{
		"dbname=" + *dbTable,
		"sslmode=disable",
	}.
		AddStringFlag("host", dbHost).
		AddIntFlag("port", dbPort).
		AddStringFlag("user", dbUser).
		AddStringFlag("password", dbPwd).String()

	log.Printf("Connecting postgres DB on %s", pgParams)
	db, err = sql.Open("postgres", pgParams)
	if err != nil {
		log.Fatal(err)
	}
	defer db.Close()
	if err = db.Ping(); err != nil {
		log.Fatal(err)
	}
	log.Printf("Postgres DB connection successful")

	router := gin.Default()
	router.LoadHTMLGlob("templates/*")
	router.GET("/", func(ctx *gin.Context) {
		ctx.HTML(http.StatusOK, "index.html", nil)
	})
	router.GET("/logs", handleGetLogs)
	router.POST("/logs", handlePostLogs)
	router.DELETE("/logs", handleDeleteLogs)

	fmt.Println("Server starting on :8080...")
	log.Fatal(router.Run(":8080"))
}

func handleGetLogs(ctx *gin.Context) {
	limit := defaultLimit
	if val, found := ctx.GetQuery("limit"); found {
		if lval, err := strconv.Atoi(val); err != nil && lval > 0 {
			limit = lval
		}
	}

	query := fmt.Sprintf(`SELECT id, severity, timestamp, file, line, message FROM %s WHERE 1=1`, *dbTable)
	args := []interface{}{}
	argCount := 1
	if before, found := ctx.GetQuery("before"); found && before != "" {
		query += fmt.Sprintf(" AND timestamp < $%d", argCount)
		args = append(args, before)
		argCount++
	}
	if severity, found := ctx.GetQuery("severity"); found && severity != "" {
		query += fmt.Sprintf(" AND severity = $%d", argCount)
		args = append(args, severity)
		argCount++
	}
	if search, found := ctx.GetQuery("search"); found && search != "" {
		query += fmt.Sprintf(" AND message ~ $%d", argCount)
		args = append(args, search)
		argCount++
	}
	query += fmt.Sprintf(" ORDER BY timestamp DESC LIMIT $%d", argCount)
	args = append(args, limit)

	rows, err := db.Query(query, args...)
	if err != nil {
		ctx.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	defer rows.Close()

	var results []LogEntry
	for rows.Next() {
		var l LogEntry
		rows.Scan(&l.ID, &l.Severity, &l.Timestamp, &l.File, &l.Line, &l.Message)
		results = append(results, l)
	}
	ctx.IndentedJSON(http.StatusCreated, results)
}

func handlePostLogs(ctx *gin.Context) {
	var entry LogEntry
	if err := json.NewDecoder(ctx.Request.Body).Decode(&entry); err != nil {
		ctx.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if entry.Timestamp.IsZero() {
		entry.Timestamp = time.Now()
	}

	query := `INSERT INTO log_entries (severity, timestamp, file, line, message)
	VALUES ($1, $2, $3, $4, $5)`
	_, err := db.Exec(query,
		entry.Severity, entry.Timestamp, entry.File, entry.Line, entry.Message)
	if err != nil {
		ctx.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	ctx.IndentedJSON(http.StatusCreated, entry)
}

func handleDeleteLogs(ctx *gin.Context) {
	before, found := ctx.GetQuery("before")
	if !found || before == "" {
		ctx.JSON(http.StatusBadRequest, gin.H{"error": "before timestamp not specified"})
		return
	}
	_, err := db.Exec("DELETE FROM log_entries WHERE timestamp < $1", before)
	if err != nil {
		ctx.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	ctx.IndentedJSON(http.StatusNoContent, nil)
}

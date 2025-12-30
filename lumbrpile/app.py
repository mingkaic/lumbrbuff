from flask import Flask, request, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO

import datetime
import os

# Determine the absolute path to the templates directory in Bazel runfiles
template_dir = os.path.join(os.path.dirname(__file__), "templates")

app = Flask(__name__, template_folder=template_dir)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///logs.db'
db = SQLAlchemy(app)
socketio = SocketIO(app, cors_allowed_origins="*")

class LogEntry(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    severity = db.Column(db.String(20))
    timestamp = db.Column(db.String(50))
    file = db.Column(db.String(200))
    line = db.Column(db.Integer)
    message = db.Column(db.Text)

@app.route('/logs', methods=['POST'])
def receive_log():
    data = request.json
    new_log = LogEntry(**data)
    db.session.add(new_log)
    db.session.commit()

    # Push to real-time dashboard
    socketio.emit('new_log', data)
    return "OK", 200

@app.route('/')
def dashboard():
    return render_template('index.html')

if __name__ == '__main__':
    with app.app_context(): db.create_all()
    socketio.run(app, port=5000)

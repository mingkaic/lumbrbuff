import gevent.monkey
gevent.monkey.patch_all()

from flask import Flask, request, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_socketio import SocketIO

import datetime
import os

# Determine the absolute path to the templates directory in Bazel runfiles
template_dir = os.path.join(os.path.dirname(__file__), "templates")

app = Flask(__name__, template_folder=template_dir)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///logs.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)
socketio = SocketIO(app, cors_allowed_origins='*', async_mode='gevent')

class LogEntry(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    severity = db.Column(db.String(20))
    timestamp = db.Column(db.String(50))
    file = db.Column(db.String(200))
    line = db.Column(db.Integer)
    message = db.Column(db.Text)

@app.route('/logs', methods=['POST'])
def receive_log():
    data = request.get_json()
    new_log = LogEntry(**data)
    db.session.add(new_log)
    db.session.commit()

    # Push to real-time dashboard
    socketio.emit('new_log', data)
    return 'OK', 200

@app.route('/')
def dashboard():
    return render_template('index.html')

@socketio.on('connect')
def handle_connect():
    print('Client connected, sending historical logs...')
    # Query all logs from the database
    history = LogEntry.query.order_by(LogEntry.id.desc()).all()

    # Convert DB objects to a list of dictionaries
    history_data = [{
        'severity': log.severity,
        'timestamp': log.timestamp,
        'file': log.file,
        'line': log.line,
        'message': log.message
    } for log in history]

    # Emit only to the sender (the newly connected browser)
    socketio.emit('log_history', history_data)

with app.app_context():
    db.create_all()

if __name__ == '__main__':
    socketio.run(app, port=5000)

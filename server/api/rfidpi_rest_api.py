import json

from db import TagDatabase
from flask import Flask, request
from flask_cors import CORS, cross_origin

app = Flask(__name__)

cors = CORS(app, resources={r"/api/*": {"origins": "*"}})
app.config['CORS_HEADERS'] = 'Content-Type'


# Return all tags in DB
# Example curl http://localhost:5000/api/tags
@app.route("/api/tags", methods=["GET"])
@cross_origin(origin='*',headers=['Content-Type','Authorization'])
def list_all_tags():
    return tagDB.list_tags(True)

# Create a new tag
# Example curl -H "Content-Type: application/json" -X POST -d '{"binary_value":"00111101111110001001110101","hex_value":"0F7E275","facility_code":"123","unique_code":"61754","proxmark":"2004f7e275"}' http://localhost:5000/api/tag
@app.route("/api/tag", methods=["POST"])
def new_tag():
    content = request.json
    tagDB.insert_tag(content["binary_value"],content["hex_value"],content["facility_code"],content["unique_code"],content["proxmark"], )
    return "Inserted " + content["binary_value"] + " into table"

# List a specific tag
# Example curl http://localhost:5000/api/tag/2
@app.route("/api/tag/<tag_id>", methods=["GET"])
def get_tag(tag_id):
    return tagDB.list_tags(False, tag_id)

# List the lastest tagged scanned
# Example curl http://localhost:5000/api/tag/latest
@app.route("/api/tag/latest", methods=["GET"])
def list_latest_tag():
    return tagDB.list_latest_tag()

# Delete a specific tag
# Example curl -X "DELETE" http://localhost:5000/api/tag/4
@app.route("/api/tag/<tag_id>", methods=["DELETE"])
def delete_tag(tag_id):
    tagDB.delete_tag(tag_id)
    return "Deleted message " + tag_id

# Deletes the rfidpi.db file and restarts the application
# Example curl -X "DELETE" http://localhost:5000/api/resetdb
@app.route("/api/resetdb", methods=["DELETE"])
def resetdb():
    tagDB.reset_database()
    return "Database destoryed, restarting application"

# Shutsdown the server
# Example curl -X "DELETE" http://localhost:5000/api/shutdown
@app.route("/api/shutdown", methods=["GET"])
def shutdown():
    tagDB.shutdown()
    return "Shuting down Pi"

# Restarts the server
# Example curl -X "DELETE" http://localhost:5000/api/restart
@app.route("/api/restart", methods=["GET"])
def restart():
    tagDB.restart()
    return "Restarting Pi"

if __name__ == "__main__":
    tagDB = TagDatabase()
    app.run(host='0.0.0.0')

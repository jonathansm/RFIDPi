import sqlite3
import os
import json
from multiprocessing import Process

class TagDatabase:
    def __init__(self, file_name='rfidpi.db'):
        self.__file_name = file_name
        self.__conn = None
        self.__initialize()

    def __initialize(self):
        if self.__conn is not None:
            raise Exception("Cannot initialize db with open connection")

        database_exists = os.path.exists(self.__file_name)
        self.__conn = sqlite3.connect(self.__file_name, check_same_thread=False)
        self.__conn.row_factory = self.__dict_factory
        if not database_exists:
            self.__setup_database()

    def __setup_database(self):
        print("Setting up database")
        self.__execute("CREATE TABLE tags (id INTEGER PRIMARY KEY AUTOINCREMENT, binary_value TEXT, hex_value TEXT, facility_code TEXT, unique_code TEXT, proxmark TEXT, scanned TIMESTAMP)")
        self.__insert_sample_data()

    def __insert_sample_data(self):
        self.insert_tag('01110001111100001000000000','1C7C200','227','57600','2005c7c200')
        self.insert_tag('01101101111100010000000000','1B7C400','219','57856','2005b7c400')
        self.insert_tag('10101001111111010010011001','2A7F499','83','64076','2006a7f499')

    def __dict_factory(self, cursor, row):
        d = {}
        for idx, col in enumerate(cursor.description):
            d[col[0]] = row[idx]
        return d

    def list_tags(self, show_all=False, tag_id=0):
        if show_all:
            command = "SELECT id, LENGTH(binary_value) AS bits, binary_value, hex_value, facility_code, unique_code, proxmark, scanned FROM tags"
        else:
            command = "SELECT id, LENGTH(binary_value) AS bits, binary_value, hex_value, facility_code, unique_code, proxmark, scanned FROM tags WHERE id = " + tag_id

        return json.dumps(self.__query(command))

    def insert_tag(self, binary_value, hex_value, facility_code, unique_code, proxmark):
        print("Insterting tag ",binary_value)
        self.__execute("INSERT INTO tags (binary_value, hex_value, facility_code, unique_code, proxmark, scanned) VALUES (?,?,?,?,?,CURRENT_TIMESTAMP)", binary_value, hex_value, facility_code, unique_code, proxmark)

    def list_latest_tag(self):
        return json.dumps(self.__query("SELECT id FROM tags ORDER BY scanned DESC LIMIT 1"))

    def delete_tag(self, id):
    	return self.__execute("DELETE FROM tags WHERE id = (?)", id)

    def reset_database(self):
        pass

    def restart(self):
        print("Restarting")
        os.system('sudo shutdown -r now')

    def shutdown(self):
        print("Shutting down")
        os.system('sudo shutdown -h now')

    def __query(self, command, *args):
        cur = self.__conn.cursor()
        cur.execute(command, args)
        return cur.fetchall()

    def __execute(self, command, *args):
        cur = self.__conn.cursor()
        result = cur.execute(command, args)
        self.__conn.commit()
        return result
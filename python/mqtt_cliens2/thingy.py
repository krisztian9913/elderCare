import re
import os
import json
from datetime import timedelta, datetime


class Thingy91:

    Longitude = None
    Latitude = None
    Time = None
    Date = None
    file_name = None
    day = None
    json_file_name = None

    def __init__(self, ID):
        self.ID = ID

        try:
            os.mkdir(self.ID)
        except FileExistsError:
            print(f"Directory {self.ID} already exists")
        except PermissionError:
            print(f"Permission denied")

        date = datetime.now()
        self.Date = date
        self.day = date.day
        self.file_name = str(date.year) + str(date.month) + str(self.day) + "_GNSS.txt"
        self.json_file_name = self.ID + "\\" + "gnss_data.json"

    def processMSG(self, msg):
        string = msg.payload.decode("utf-8")

        cleaned_string = string.strip('\x00').strip().strip("'")

        match = re.search(r"Time \(UTC\): ([\d:]+), Latitude: ([\d.]+), Longitude: ([\d.]+)", cleaned_string)

        if match:
            self.Time = match.group(1)
            self.Latitude = match.group(2)
            self.Longitude = match.group(3)
            self.Date = datetime.now()

            print(f"Dátum: {self.Date}, GNSS time: {self.Time}, Latitude: {self.Latitude}, Longitude: {self.Longitude}")
            self.write_file()
            self.write_to_json()
        else:
            print("Nem sikerült adatokat kinyerni a payloadból: ", cleaned_string)

    def check_new_day(self):
        date = datetime.now()

        if self.day != date.day:
            self.day = date.day
            self.file_name = str(date.year) + str(date.month) + str(date.day) + "_GNSS.txt"

    def write_file(self):
        self.check_new_day()

        string = "Date: " + str(self.Date) + ", Time (UTC): " + str(self.Time) + ", Latitude: " + \
                 str(self.Latitude) + ", Longitude: " + str(self.Longitude) + "\n"

        f = open(self.ID + "\\" +self.file_name, "a", newline="")
        f.truncate()
        f.write(string)
        f.close()

    def write_to_json(self):

        timestamp = str(self.Date)
        iso_format = timestamp.replace(" ", "T")
        print(iso_format)

        new_entry = {
            "device_id": self.ID,
            "timestamp": iso_format,
            "time (utc)": self.Time,
            "latitude": self.Latitude,
            "longitude": self.Longitude
        }

        data = []

        if os.path.exists(self.json_file_name):
            f = open(self.json_file_name, "r")
            try:
                data = json.load(f)
                f.close()
            except json.JSONDecodeError:
                print("Üres vagy hibás fájl")

        data.append(new_entry)

        f = open(self.json_file_name, "w")
        json.dump(data, f, indent=2)
        f.close()

        self.clean_old_gnss_data()


    def clean_old_gnss_data(self, max_age_days=7):

        if not os.path.exists(self.json_file_name):
            print("Nincs mit tisztítani")
            return

        f = open(self.json_file_name, "r")
        try:
            data = json.load(f)
            f.close()
        except json.JSONDecodeError:
            print("A fájl nem olvasható")
            return

        now = self.Date
        cutoff = now - timedelta(days=max_age_days)


        def is_recent(entry):
            try:
                ts = datetime.fromisoformat(entry["timestamp"])

                return ts >= cutoff
            except Exception as e:
                print(f"error, {e}")
                return False

        new_data = list(filter(is_recent, data))

        f = open(self.json_file_name, "w")
        json.dump(new_data, f, indent=2)
        f.close()

        print(f"Tisztítás kész. Megmaradt bejegyzések: {len(new_data)}")


    def get_file_name(self):
        return self.file_name

    def get_Time(self):
        return self.Time

    def get_Latitude(self):
        return self.Latitude

    def get_Longitude(self):
        return self.Longitude

    def get_ID(self):
        return self.ID

    def __repr__(self):
        return f"Time (UTC): {self.Time}, Latitude: {self.Latitude}, Longitude: {self.Longitude}"

    def get_today_GNSS(self):
        try:
            f = open(self.json_file_name, "r")
            data = json.load(f)
            f.close()
        except (FileNotFoundError, json.JSONDecodeError):
            print("Hiba a fájl olvasásakor")
            return []

        today = self.Date.today().date()
        today_entries = []

        for entry in data:
            try:
                ts = datetime.fromisoformat(entry["timestamp"])
                if ts.date() == today:
                    today_entries.append(entry)
            except Exception as e:
                print(f"Hibás időformáum: {entry.get('timestamp')}, hiba: {e}")

        return json.dumps(today_entries, indent=2)



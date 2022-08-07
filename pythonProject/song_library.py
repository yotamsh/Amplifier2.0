import os
import datetime
import random
from time import sleep

import eyed3

import code_generator

SONGS_FOLDER_NAME = "songs/"

GENERAL_NAME = "general/"
MORNING_NAME = "morning/"
ALL_COLLECTIONS = (GENERAL_NAME, MORNING_NAME)

SCHEDULE = (
    (datetime.time(0, 0), [GENERAL_NAME]),
    (datetime.time(6, 0), [MORNING_NAME]),
    (datetime.time(8, 0), [GENERAL_NAME]),
    (datetime.time(9), [GENERAL_NAME]),
    (datetime.time(23,45), [MORNING_NAME]),
)


def find_schedule_index(current_time: datetime.datetime):
    schedule_index = 0
    current_hour = current_time.time()
    while schedule_index < len(SCHEDULE) - 1 and current_hour > SCHEDULE[schedule_index + 1][0]:
        schedule_index += 1
    return schedule_index


def regenerate_codes_all_songs():
    s = set()
    for collection in ALL_COLLECTIONS:
        for filename in os.listdir(SONGS_FOLDER_NAME + collection):
            songpath = SONGS_FOLDER_NAME + collection + filename
            print(f"Generating new code for: {songpath}")
            file = eyed3.load(songpath)
            new_code = code_generator.generate_new_code()
            while new_code in s:
                new_code = code_generator.generate_new_code()
            if not file.tag:
                file.tag = eyed3.id3.tag.Tag()
            file.tag.album = new_code
            file.tag.save()
            s.add(new_code)


class SongChooser:
    current_schedule_index = -1
    current_song_collection = []
    codes_dic = {}

    def __init__(self):
        self.update_song_collection(datetime.datetime.now())
        self.create_codes_dic()

    def update_song_collection(self, current_time):
        x = find_schedule_index(current_time)
        if x != self.current_schedule_index:
            self.current_song_collection = []
            for collection in SCHEDULE[x][1]:
                self.current_song_collection.extend(
                    SONGS_FOLDER_NAME + collection + filename
                    for filename in os.listdir(SONGS_FOLDER_NAME + collection))
            self.current_schedule_index = x

    def get_random_song(self) -> str:
        return random.choice(self.current_song_collection)

    def is_valid_code(self, code):
        return code in self.codes_dic

    def get_song_by_code(self, code) -> str:
        return self.codes_dic[code]

    def create_codes_dic(self):
        for collection in ALL_COLLECTIONS:
            for filename in os.listdir(SONGS_FOLDER_NAME + collection):
                songpath = SONGS_FOLDER_NAME + collection + filename
                file = eyed3.load(songpath)
                if file.tag and str(file.tag.album):
                    self.codes_dic[str(file.tag.album)] = songpath
                else:
                    pass
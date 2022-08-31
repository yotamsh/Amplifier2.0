import csv
import datetime
import os
import random
from collections import namedtuple

import eyed3
import code_generator
from enum import Enum

CODE_LENGTH = 5

SONGS_FOLDER_NAME = "songs/"


class Collection(Enum):
    GENERAL = "general/"
    MORNING = "morning/"
    PARTY = "party/"
    TV = "tv/"
    CLASSIC = "classic/"
    DISNEY = "disney/"


ALL_COLLECTIONS = {collection for collection in Collection}
ALL_COLLECTIONS_EXCEPT_TV = {collection for collection in Collection if collection is not Collection.TV}

class Schedule():
    DailyScheduleEntry = namedtuple("DailyScheduleEntry", ["time", "collections"])
    SpecialScheduleEntry = namedtuple("SpecialScheduleEntry", ["start", "end", "collections"])

    DAILY_SCHEDULE = (
        DailyScheduleEntry(datetime.time(0, 0),
                           {Collection.PARTY}),
        DailyScheduleEntry(datetime.time(2, 0),
                           ALL_COLLECTIONS_EXCEPT_TV),
        DailyScheduleEntry(datetime.time(6, 0),
                           {Collection.CLASSIC}),
        DailyScheduleEntry(datetime.time(8, 0),
                           {Collection.MORNING}),
        DailyScheduleEntry(datetime.time(10),
                           ALL_COLLECTIONS_EXCEPT_TV),
        DailyScheduleEntry(datetime.time(11),
                           {Collection.TV}),
        DailyScheduleEntry(datetime.time(12),
                           {Collection.GENERAL, Collection.DISNEY, Collection.PARTY, Collection.MORNING}),
        DailyScheduleEntry(datetime.time(16),
                           {Collection.TV}),
        DailyScheduleEntry(datetime.time(17),
                           ALL_COLLECTIONS),
        DailyScheduleEntry(datetime.time(18),
                           ALL_COLLECTIONS_EXCEPT_TV),
        DailyScheduleEntry(datetime.time(20),
                           {Collection.DISNEY}),
        DailyScheduleEntry(datetime.time(20, 30),
                           {Collection.GENERAL, Collection.DISNEY, Collection.PARTY, Collection.MORNING}),
    )

    SPECIAL_SCHEDULE = (
        # start (datetime.datetime), end (datetime.datetime), collections (tuple of Collections)
        SpecialScheduleEntry(start=datetime.datetime(2022, 8, 18, 16, 00),
                             end=datetime.datetime(2022, 8, 18, 16, 30),
                             collections={Collection.TV}),
        SpecialScheduleEntry(start=datetime.datetime(2022, 8, 18, 18, 00),
                             end=datetime.datetime(2022, 8, 18, 20, 30),
                             collections={Collection.TV}),
    )

    @staticmethod
    def get_collections_by_time(current_time) -> set:
        for special_schedule_entry in Schedule.SPECIAL_SCHEDULE:
            if special_schedule_entry.start < current_time < special_schedule_entry.end:
                return special_schedule_entry.collections
        schedule_index = 0
        current_hour = current_time.time()
        while schedule_index < len(Schedule.DAILY_SCHEDULE) - 1 \
                and current_hour > Schedule.DAILY_SCHEDULE[schedule_index + 1].time:
            schedule_index += 1
        return Schedule.DAILY_SCHEDULE[schedule_index].collections


def regenerate_codes_all_songs():
    num_songs = 0
    num_collides = 0
    with open('AmplifierSongCodes.csv', 'w', newline='') as database_file:
        writer = csv.writer(database_file)
        writer.writerow(["Collection", "Song Name", "Code"])
        s = set()
        for collection in ALL_COLLECTIONS:
            for file_path in os.listdir(SONGS_FOLDER_NAME + collection.value):
                num_songs += 1
                songpath = SONGS_FOLDER_NAME + collection.value + file_path
                print(f"Generating new code for: {songpath}")
                audio_file = eyed3.load(songpath)
                new_code = code_generator.generate_new_code(CODE_LENGTH)
                while new_code in s:
                    num_collides += 1
                    new_code = code_generator.generate_new_code(CODE_LENGTH)
                if not audio_file.tag:
                    audio_file.tag = eyed3.id3.tag.Tag()
                audio_file.tag.album = new_code
                audio_file.tag.save(version=eyed3.id3.tag.ID3_V2_3)
                s.add(new_code)
                writer.writerow([collection.name, get_song_name(file_path), new_code])
    print("Finished regenerating song codes.")
    print(f'total songs = {num_songs}, num of collides = {num_collides}.')

class SongChooser:
    current_collections = {}
    current_songs_basket = []
    codes_dic = {}

    def __init__(self, logger):
        self.logger = logger
        self.update_collection_schedule(datetime.datetime.now(), self.logger)
        self.create_codes_dic()

    def update_collection_schedule(self, current_time, logger):
        current_collections = Schedule.get_collections_by_time(current_time)
        if current_collections != self.current_collections:
            self.current_collections = current_collections
            logger.log_collections_update(self.current_collections)
            self.current_songs_basket = []
            for collection in self.current_collections:
                self.current_songs_basket.extend(
                    SONGS_FOLDER_NAME + collection.value + filename
                    for filename in os.listdir(SONGS_FOLDER_NAME + collection.value))

    def get_random_song(self) -> str:
        return random.choice(self.current_songs_basket)

    def is_valid_code(self, code):
        return code in self.codes_dic

    def get_song_by_code(self, code) -> str:
        return self.codes_dic[code]

    def create_codes_dic(self):
        print("Creating song codes dictionary")
        eyed3.log.setLevel("ERROR")
        for collection in ALL_COLLECTIONS:
            print(collection.name, end=" ", flush=True)
            for filename in os.listdir(SONGS_FOLDER_NAME + collection.value):
                songpath = SONGS_FOLDER_NAME + collection.value + filename
                file = eyed3.load(songpath)
                if file.tag and str(file.tag.album):
                    self.codes_dic[str(file.tag.album)] = songpath
                else:
                    pass
        print()


def get_song_name(song_path):
    return song_path.split("/")[-1][:-4]

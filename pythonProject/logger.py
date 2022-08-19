import time

import song_library


class Logger:

    def __init__(self):
        self.logFile = open(f"logs/log_{time.strftime('%Y-%m-%d_%H-%M-%S')}.txt", "at")

    def log_new_full_volume(self, song_path):
        self.__log_msg(f"{time.ctime()} - song \"{song_library.get_song_name(song_path)}\" got FULL VOLUMED!")

    def log_exit_full_volume(self, song_path):
        self.__log_msg(f"{time.ctime()} - song \"{song_library.get_song_name(song_path)}\" got quited-down")

    def log_collections_update(self, collections):
        self.__log_msg(f"{time.ctime()} - Amplifier collections were set to: "
                       f"{[collection.name for collection in collections]}")

    def log_max_volume_update(self, max_volume):
        self.__log_msg(f"{time.ctime()} - Amplifier max volume was set to: {max_volume}")

    def __log_msg(self, msg):
        print(msg, file=self.logFile, flush=True)

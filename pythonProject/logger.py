import time


class Logger:

    def __init__(self):
        self.logFile = open(f"logs/log_{time.strftime('%Y-%m-%d_%H-%M-%S')}.txt", "at")

    def log_new_full_volume(self, song_path):
        self.__log_msg(f"{time.ctime()} - song \"{get_song_name(song_path)}\" got FULL VOLUMED!")

    def log_exit_full_volume(self, song_path):
        self.__log_msg(f"{time.ctime()} - song \"{get_song_name(song_path)}\" got quited-down")

    def __log_msg(self, msg):
        print(msg, file=self.logFile, flush=True)


def get_song_name(song_path):
    return song_path.split("/")[-1][:-4]

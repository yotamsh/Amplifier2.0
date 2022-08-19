import datetime
import random
import time
from collections import namedtuple
from enum import Enum
import pygame
import serial

import logger
import song_library
from song_library import CODE_LENGTH


class Mode(Enum):
    IDLE = 1
    AMPLIFYING = 2
    PARTYMODE = 3
    CODEINPUT = 4


NO_DATA = -1
NUM_BUTTONS = 10
START_CODE_INPUT = 77
STOP_CODE_INPUT = 88
GET_CODE_INPUT = 99
MSG_CODE_INVALID = 0
MSG_CODE_VALID = 1

pygame.init()

winSound = pygame.mixer.Sound("sounds/win.mp3")
codeSound = pygame.mixer.Sound("sounds/code.mp3")
introSound = pygame.mixer.Sound("sounds/one_two_three.mp3")
codeInputMusicPath = "sounds/sheshtus.mp3"
failSounds = ["fail1.mp3", "fail2.mp3", "fail3.mp3", "fail4.mp3"]
quiteSound = pygame.mixer.Sound("sounds/quite.mp3")
boomSound = pygame.mixer.Sound("sounds/boom.mp3")


class AmplifierRuntime:

    def __init__(self):
        self.logger = logger.Logger()
        self.song_chooser = song_library.SongChooser(self.logger)
        self.volume_manager = VolumeManager()
        self.last_schedule_update = datetime.datetime.now()
        self.arduino = serial.Serial(port="COM5", baudrate=9600, timeout=0.1, )
        self.mixer = pygame.mixer
        self.mixer.pre_init()
        self.mixer.init()
        self.current_song = ""

        self.mode = Mode.IDLE
        self.mixer.music.stop()
        self.load_next_song()

    def run(self):
        codeSound.play()
        while True:
            if self.mode == Mode.IDLE:
                self.run_idle_mode()
            elif self.mode == Mode.AMPLIFYING:
                self.run_amplifying_mode()
            elif self.mode == Mode.PARTYMODE:
                self.run_party_mode()
            elif self.mode == Mode.CODEINPUT:
                self.run_code_input_mode()

    def run_idle_mode(self):
        self.mixer.music.stop()
        self.load_next_song()
        print(" " + self.current_song, end="", flush=True)
        while self.mode == Mode.IDLE:
            button_clicked = self.recieve_byte_msg()
            if button_clicked != NO_DATA:
                if 0 < button_clicked < 10:
                    self.mode = Mode.AMPLIFYING
                    self.mixer.music.set_volume(self.volume_manager.calculate_volume(button_clicked))
                if button_clicked == 10:
                    self.mode = Mode.PARTYMODE
                    self.mixer.music.set_volume(8)
                    self.mixer.music.play()
                    winSound.play()
                    time.sleep(0.9)
                    self.mixer.music.set_volume(1)
                elif button_clicked == START_CODE_INPUT:
                    self.mode = Mode.CODEINPUT
            else:
                self.schedule_update()

    def run_amplifying_mode(self):
        self.mixer.music.play()
        while self.mode == Mode.AMPLIFYING:
            buttons_clicked = self.recieve_byte_msg()
            if buttons_clicked != NO_DATA:
                if buttons_clicked == 0:
                    self.mixer.music.stop()
                    self.mode = Mode.IDLE
                elif 0 < buttons_clicked < 10:
                    self.mixer.music.set_volume(self.volume_manager.calculate_volume(buttons_clicked))
                elif buttons_clicked == 10:
                    self.mode = Mode.PARTYMODE
                    winSound.play()
                    time.sleep(0.9)
                    self.mixer.music.set_volume(1)
                elif buttons_clicked == START_CODE_INPUT:
                    self.mode = Mode.CODEINPUT

            elif not self.mixer.music.get_busy():
                self.send_partymode_finish_msg()
                self.mode = Mode.IDLE

    def run_party_mode(self):
        self.logger.log_new_full_volume(self.current_song)
        while self.mode == Mode.PARTYMODE:
            byte = self.recieve_byte_msg()
            if byte != NO_DATA:
                mute_level = byte - 20
                self.mixer.music.set_volume(self.volume_manager.calculate_volume(10 - mute_level))
                if mute_level == 1:
                    quiteSound.play()
                elif mute_level == 10:
                    self.mode = Mode.IDLE
                    self.mixer.music.stop()
                    boomSound.play()
                    self.logger.log_exit_full_volume(self.current_song)
            elif not self.mixer.music.get_busy():
                self.send_partymode_finish_msg()
                self.mode = Mode.IDLE

    def run_code_input_mode(self):
        codeSound.play()
        self.mixer.music.load(codeInputMusicPath)
        self.mixer.music.set_volume(0.5)
        self.mixer.music.play()
        while self.mode == Mode.CODEINPUT:
            msg = self.recieve_byte_msg()
            if msg != NO_DATA:
                if msg == STOP_CODE_INPUT:  # msg to enter codeinput mode
                    self.mode = Mode.IDLE
                elif msg == GET_CODE_INPUT:  # msg with some code song
                    msg = self.arduino.read(CODE_LENGTH)
                    code = msg[:5].decode("ascii")
                    if self.song_chooser.is_valid_code(code):
                        self.send_code_valid_msg()
                        self.mixer.music.stop()
                        introSound.play()
                        self.mode = Mode.PARTYMODE
                        self.load_song_by_code(code)
                        self.mixer.music.set_volume(1)
                        time.sleep(3.1)
                        self.mixer.music.play()
                        print(" " + self.current_song, end="", flush=True)
                        return
                    else:
                        self.send_code_invalid_msg()
                        self.mode = Mode.IDLE
        self.mixer.music.stop()
        self.play_fail_sound()
        time.sleep(2)

    def schedule_update(self):
        current_time = datetime.datetime.now()
        if (current_time - self.last_schedule_update).seconds > 60:
            self.song_chooser.update_collection_schedule(current_time, self.logger)
            self.volume_manager.update_volume_schedule(current_time, self.logger)
            self.last_schedule_update = current_time

    def load_song_by_code(self, code):
        self.current_song = self.song_chooser.get_song_by_code(code)
        self.mixer.music.load(self.current_song)

    def load_next_song(self):
        self.current_song = self.song_chooser.get_random_song()
        self.mixer.music.load(self.current_song)

    def recieve_byte_msg(self):
        msg = self.arduino.read(1)
        if (msg):
            newLinePrint(msg)
            return msg[0]
        else:
            return NO_DATA

    def send_partymode_finish_msg(self):
        self.arduino.write([0])
        print(f" (sent {0})", end="", flush=True)

    def send_code_valid_msg(self):
        self.arduino.write([MSG_CODE_VALID])
        print(f" (sent {MSG_CODE_VALID})", end="", flush=True)

    def send_code_invalid_msg(self):
        time.sleep(0.1)
        self.arduino.write([MSG_CODE_INVALID])
        print(f" (sent {MSG_CODE_INVALID})", end="", flush=True)

    def play_fail_sound(self):
        path = "sounds/" + random.choice(failSounds)
        sound = pygame.mixer.Sound(path)
        sound.play()


class VolumeManager():
    VolumeScheduleEntry = namedtuple("volumeScheduleEntry", ["time", "max_volume"])
    current_schedule_index = 0
    current_max_volume = 1

    VOLUME_SCHEDULE = (
        VolumeScheduleEntry(datetime.time(0, 0), 1),
        VolumeScheduleEntry(datetime.time(1, 0), 0.5),
        VolumeScheduleEntry(datetime.time(2, 0), 1)
    )

    def calculate_volume(self, num_of_clicked) -> float:
        relative_volume = pow((num_of_clicked + 1) / 11, 2)
        return relative_volume * self.current_max_volume

    def update_volume_schedule(self, current_time, logger):
        schedule_index = 0
        current_hour = current_time.time()
        while schedule_index < len(self.VOLUME_SCHEDULE) - 1 \
                and current_hour > self.VOLUME_SCHEDULE[schedule_index + 1].time:
            schedule_index += 1
        if self.current_max_volume != schedule_index:
            self.current_schedule_index = schedule_index
            self.current_max_volume = self.VOLUME_SCHEDULE[self.current_schedule_index].max_volume
            logger.log_max_volume_update(self.current_max_volume)


def newLinePrint(s):
    print("\n", s, end="", flush=True)

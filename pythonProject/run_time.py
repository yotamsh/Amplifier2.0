import random
from enum import Enum

import pygame
import time
import datetime

import serial

import run_time
import song_library


class Mode(Enum):
    IDLE = 1
    AMPLIFYING = 2
    PARTYMODE = 3
    CODEINPUT = 4

pygame.init()

winSound = pygame.mixer.Sound("sounds/win3.mp3")
codeSound = pygame.mixer.Sound("sounds/code2.mp3")
introSound = pygame.mixer.Sound("sounds/one_two_three.mp3")
codeInputMusicPath = "sounds/sheshtus.mp3"
failSounds = ["fail1.mp3","fail2.mp3", "fail3.mp3"]
quiteSound = pygame.mixer.Sound("sounds/quite.mp3")

class AmplifierRuntime:


    def __init__(self):
        self.song_chooser = song_library.SongChooser()
        self.arduino = serial.Serial(port="COM5", baudrate=9600, timeout=0.1, )
        self.mixer = pygame.mixer
        self.mixer.pre_init()
        self.mixer.init()

        self.mode = Mode.IDLE
        self.mixer.music.stop()
        self.load_next_song()
        self.last_library_update = datetime.datetime.now()

    def loop(self):
        while True:
            if self.mode == Mode.PARTYMODE:
                if not self.mixer.music.get_busy():
                    self.mode = Mode.IDLE
                    self.load_next_song()
                    self.send_partymode_finish_msg()
            # elif self.mode == Mode.CODEINPUT and not self.mixer.music.get_busy():
            #     self.fail_code_input()
##            else:
            if True:
                msg = self.arduino.read(1)
                #s = input("enter:")
                #msg = bytes(eval(s))

                if (msg):
                    b = msg[0]
                    print(msg)
                    if b == 77: # msg to enter codeinput mode
                        self.mode = Mode.CODEINPUT
                        codeSound.play()
                        self.mixer.music.load(codeInputMusicPath)
                        self.mixer.music.set_volume(0.5)
                        self.mixer.music.play()
                    elif b == 88: # msg to fail codeinput mode
                        self.fail_code_input()
                    elif b == 99: # msg with some code song
                        msg = self.arduino.read(5)
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
                        else:
                            self.send_code_invalid_msg()
                            self.fail_code_input()
                    elif 0 <= b and b <= 10:
                        if self.mode == Mode.IDLE and b > 0:
                            self.mode = Mode.AMPLIFYING
                            self.mixer.music.set_volume(calculate_volume(b))
                            self.mixer.music.play()
                        elif self.mode == Mode.AMPLIFYING and b == 0:
                            self.mixer.music.stop()
                            self.mode = Mode.IDLE
                            self.load_next_song()
                        elif self.mode == Mode.AMPLIFYING:
                            self.mixer.music.set_volume(calculate_volume(b))
                        elif self.mode == Mode.PARTYMODE:
                            self.mixer.music.set_volume(calculate_volume(10-b))
                            if b == 1:
                                quiteSound.play()
                            if b == 10:
                                self.mode = Mode.IDLE
                                self.mixer.music.stop()
                                self.load_next_song()
                        if self.mode == Mode.AMPLIFYING and b == 10:
                            self.mode = Mode.PARTYMODE
                            winSound.play()
                            time.sleep(0.9)
                            self.mixer.music.set_volume(1)
            current_time = datetime.datetime.now()
            if (current_time - self.last_library_update).seconds > 60:
                self.song_chooser.update_song_collection(current_time)
                self.last_library_update = current_time


    def fail_code_input(self):
        self.mode = Mode.IDLE
        self.mixer.music.stop()
        self.play_fail_sound()
        time.sleep(2)
        self.load_next_song()

    def load_song_by_code(self, code):
        self.mixer.music.load(self.song_chooser.get_song_by_code(code))

    def load_next_song(self):
        self.mixer.music.load(self.song_chooser.get_random_song())

    def send_partymode_finish_msg(self):
        self.arduino.write([0])

    def send_code_valid_msg(self):
        self.arduino.write([1])

    def send_code_invalid_msg(self):
        time.sleep(0.1)
        self.arduino.write([0])

    def play_fail_sound(self):
        path = "sounds/" + random.choice(failSounds)
        sound = pygame.mixer.Sound(path)
        sound.play()

def calculate_volume(num_of_clicked) -> float:
    return (num_of_clicked + 1) / 11
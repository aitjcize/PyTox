#
# @file   phone.py
# @author Wei-Ning Huang (AZ) <aitjcize@gmail.com>
#
# Copyright (C) 2013 - 2014 Wei-Ning Huang (AZ) <aitjcize@gmail.com>
# All Rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import sys

import cv2
import numpy as np
import pyaudio

from time import sleep
from os.path import exists
from threading import Thread
from select import select

from tox import Tox, ToxAV

SERVER = ["54.199.139.199", 33445, "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"]


DATA = 'phone.data'
cap = cv2.VideoCapture(0)
audio = pyaudio.PyAudio()

class AV(ToxAV):
    def __init__(self, core, max_calls, debug=False):
        self.debug = debug
        self.core = self.get_tox()
        self.stop = True
        self.cs = None
        self.call_idx = None
        self.call_type = self.TypeAudio
        self.ae_thread = None
        self.ve_thread = None
        self.frame_size = 960

    def update_settings(self, idx):
        self.cs = self.get_peer_csettings(idx, 0)
        self.call_type = self.cs["call_type"]
        self.frame_size = self.cs["audio_sample_rate"] * \
                self.cs["audio_frame_duration"] / 1000

        ret, frame = cap.read()
        width, height = 640, 480

        if ret:
            frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            height, width, channels = frame.shape
        else:
            print("Can't determine webcam resolution")

        try:
            self.change_settings(idx, {"max_video_width": width,
                                       "max_video_height": height})
        except: pass

    def on_invite(self, idx):
        self.update_settings(idx)

        print("Incoming %s call from %d:%s ..." % (
                "video" if self.call_type == self.TypeVideo else "audio", idx,
                self.core.get_name(self.get_peer_id(0))))

        self.answer(idx, self.call_type)
        print("Answered, in call...")

    def on_start(self, idx):
        self.update_settings(idx)
        self.prepare_transmission(idx, self.jbufdc * 2, self.VADd, True)

        self.stop = False
        self.aistream = audio.open(format=pyaudio.paInt16,
                                   channels=self.cs["audio_channels"],
                                   rate=self.cs["audio_sample_rate"],
                                   input=True,
                                   frames_per_buffer=self.frame_size)
        self.aostream = audio.open(format=pyaudio.paInt16,
                                   channels=self.cs["audio_channels"],
                                   rate=self.cs["audio_sample_rate"],
                                   output=True)

        self.ae_thread = Thread(target=self.audio_encode, args=(idx,))
        self.ae_thread.daemon = True
        self.ae_thread.start()

        if self.call_type == self.TypeVideo:
            self.ve_thread = Thread(target=self.video_encode, args=(idx,))
            self.ve_thread.daemon = True
            self.ve_thread.start()

    def on_end(self, idx):
        self.stop = True
        if self.ae_thread:
            self.ae_thread.join()
        if self.call_type == self.TypeVideo:
            if self.ve_thread:
                self.ve_thread.join()

        self.kill_transmission(idx)
        print("Call ended")

    def on_cancel(self, idx):
        self.kill_transmission(idx)

    def on_starting(self, idx):
        self.on_start(idx)

    def on_ending(self, idx):
        try:
            self.on_end(idx)
        except: pass

    def on_peer_timeout(self, idx):
        self.kill_transmission(idx)
        self.stop_call(idx)

    def on_request_timeout(self, idx):
        self.kill_transmission(idx)

    def audio_encode(self, idx):
        print("Starting audio encode thread...")

        while not self.stop:
            try:
                self.send_audio(idx, self.frame_size,
                        self.aistream.read(self.frame_size))
            except Exception as e:
                print(e)

            sleep(0.001)

    def on_audio_data(self, idx, size, data):
        if self.debug:
            sys.stdout.write('.')
            sys.stdout.flush()
        self.aostream.write(data)

    def video_encode(self, idx):
        print("Starting video encode thread...")

        while not self.stop:
            try:
                ret, frame = cap.read()
                if ret:
                    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    height, width, channels = frame.shape
                    self.send_video(idx, width, height, frame.tostring())
            except Exception as e:
                print(e)

            sleep(0.001)

    def on_video_data(self, idx, width, height, data):
        if self.debug:
            sys.stdout.write('*')
            sys.stdout.flush()

        frame = np.ndarray(shape=(height, width, 3),
                dtype=np.dtype(np.uint8), buffer=data)
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        cv2.imshow('frame', frame)
        cv2.waitKey(1)


class Phone(Tox):
    def __init__(self):
        if exists(DATA):
            self.load_from_file(DATA)

        self.set_name("PyTox-Phone")
        print('ID: %s' % self.get_address())

        self.connect()
        self.av = AV(self, 1, debug=True)

    def connect(self):
        print('connecting...')
        self.bootstrap_from_address(SERVER[0], SERVER[1], SERVER[2])

    def loop(self):
        checked = False

        try:
            while True:
                status = self.isconnected()

                if not checked and status:
                    print('Connected to DHT.')
                    checked = True

                if checked and not status:
                    print('Disconnected from DHT.')
                    self.connect()
                    checked = False

                readable, _, _ = select([sys.stdin], [], [], 0.01)

                if readable:
                    args = sys.stdin.readline().strip().split()
                    if args:
                        if args[0] == "add":
                            try:
                                self.add_friend(args[1], "Hi")
                            except: pass
                            print('Friend added')
                        elif args[0] == "msg":
                            try:
                                if len(args) > 2:
                                    friend_number = int(args[1])
                                    msg = ' '.join(args[2:])
                                    self.send_message(friend_number, msg)
                            except: pass
                        elif args[0] == "call":
                            if len(args) == 2:
                                self.call(int(args[1]))
                        elif args[0] == "cancel":
                            try:
                                if len(args) == 2:
                                    self.av.cancel(int(args[1]), 'cancel')
                                    print('Canceling...')
                            except: pass
                        elif args[0] == "hangup":
                            try:
                                self.av.hangup(self.call_idx)
                                self.av.kill_transmission(self.call_idx)
                            except: pass
                        elif args[0] == "quit":
                            raise KeyboardInterrupt
                        else:
                            print('Invalid command:', args)

                self.do()
        except KeyboardInterrupt:
            self.save_to_file(DATA)
            cap.release()
            cv2.destroyAllWindows()

    def on_friend_request(self, pk, message):
        print('Friend request from %s: %s' % (pk, message))
        self.add_friend_norequest(pk)
        print('Accepted.')

    def on_friend_message(self, friendId, message):
        name = self.get_name(friendId)
        print('%s: %s' % (name, message))

    def on_connection_status(self, friendId, status):
        print('%s %s' % (self.get_name(friendId),
                'online' if status else 'offline'))

    def call(self, friend_number):
        print('Calling %s ...' % self.get_name(friend_number))
        self.call_idx = self.av.call(friend_number, self.av.TypeVideo, 60)

if len(sys.argv) == 2:
    DATA = sys.argv[1]

t = Phone()
t.loop()

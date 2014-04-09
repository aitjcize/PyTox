#
# @file   echo.py
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


DATA = 'data'
cap = cv2.VideoCapture(0)
audio = pyaudio.PyAudio()

class AV(ToxAV):
    def __init__(self, core, width, height, debug=False):
        self.debug = debug
        self.core = self.get_tox()
        self.stop = True
        self.call_type = self.TypeAudio

    def on_invite(self):
        self.call_type = self.get_peer_transmission_type(0)
        print 'Incoming %s call from %s ...' % (
                "video" if self.call_type == self.TypeVideo else "audio",
                self.core.get_name(self.get_peer_id(0)))

        self.answer(self.call_type)
        print 'Answered, in call...'

    def on_start(self):
        self.call_type = self.get_peer_transmission_type(0)
        self.prepare_transmission(True)

        self.stop = False
        self.a_thread = Thread(target=self.a_transmission)
        self.a_thread.daemon = True

        self.astream = audio.open(format=pyaudio.paInt16, channels=1,
                                  rate=48000, input=True, output=True,
                                  frames_per_buffer=960)
        self.a_thread.start()

        if self.call_type == self.TypeVideo:
            self.v_thread = Thread(target=self.v_transmission)
            self.v_thread.daemon = True
            self.v_thread.start()

    def on_end(self):
        self.stop = True
        self.a_thread.join()
        if self.call_type == self.TypeVideo:
            self.v_thread.join()

        self.kill_transmission()
        print 'Call ended'

    def on_starting(self):
        self.on_start()

    def on_ending(self):
        self.on_end()

    def on_peer_timeout(self):
        self.stop_call()

    def on_request_timeout(self):
        self.stop_call()

    def a_transmission(self):
        print "Starting audio transmission..."

        while not self.stop:
            try:
                aret = self.recv_audio()
                if aret:
                    if self.debug:
                        sys.stdout.write('.')
                        sys.stdout.flush()
                    self.astream.write(aret["data"])

                self.send_audio(960, self.astream.read(960))
            except Exception as e:
                print e
            sleep(0.001)

    def v_transmission(self):
        print "Starting video transmission..."

        while not self.stop:
            try:
                vret = self.recv_video()
                if vret:
                    if self.debug:
                        sys.stdout.write('*')
                        sys.stdout.flush()

                    frame = np.ndarray(shape=(vret['d_h'], vret['d_w'], 3),
                            dtype=np.dtype(np.uint8), buffer=vret["data"])
                    frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                    cv2.imshow('frame', frame)
                    cv2.waitKey(1)

                ret, frame = cap.read()
                if ret:
                    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    self.send_video(frame.tostring())
                    del frame
            except Exception as e:
                print e

            sleep(0.001)


class EchoBot(Tox):
    def __init__(self):
        if exists(DATA):
            self.load_from_file(DATA)

        self.set_name("EchoBot")
        print('ID: %s' % self.get_address())

        self.connect()
        self.av = AV(self, 640, 480)

    def connect(self):
        print('connecting...')
        self.bootstrap_from_address(SERVER[0], 1, SERVER[1], SERVER[2])

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
                            print 'Friend added'
                        if args[0] == "call":
                            if len(args) == 2:
                                self.call(int(args[1]))
                        if args[0] == "hangup":
                            self.av.hangup()

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
        print('EchoBot: %s' % message)
        self.send_message(friendId, message)

    def on_connection_status(self, friendId, status):
        print '%s %s' % (self.get_name(friendId),
                'online' if status else 'offline')

    def call(self, friend_number):
        print 'Calling %s ...' % self.get_name(friend_number)
        self.av.call(friend_number, self.av.TypeVideo, 60)

if len(sys.argv) == 2:
    DATA = sys.argv[1]

t = EchoBot()
t.loop()

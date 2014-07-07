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
from tox import Tox, ToxAV

from time import sleep, time
from os.path import exists
from threading import Thread
from random import randint

SERVER = ["54.199.139.199", 33445, "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"]

DATA = 'data'

class AV(ToxAV):
    def __init__(self, core, max_calls):
        super(AV, self).__init__(core, max_calls)
        self.core = self.get_tox()
        self.daemon = True
        self.stop = True
        self.call_type = self.TypeAudio
        self.prepare_transmission(width, height, True)

    def on_invite(self):
        self.call_type = self.get_peer_transmission_type(0, 0)
        print("Incoming %s call from %s ..." % (
                "video" if self.call_type == self.TypeVideo else "audio",
                self.core.get_name(self.get_peer_id(0))))

        self.answer(self.call_type)
        print("Answered, in call...")

    def on_start(self):
        self.call_type = self.get_peer_transmission_type(0, 0)
        self.prepare_transmission(True)

        self.stop = False
        self.a_thread = Thread(target=self.audio_transmission)
        self.a_thread.daemon = True
        self.a_thread.start()

        if self.call_type == self.TypeVideo:
            self.v_thread = Thread(target=self.video_transmission)
            self.v_thread.daemon = True
            self.v_thread.start()

    def on_end(self):
        self.stop = True
        self.kill_transmission()
        self.a_thread.join()

        if self.call_type == self.TypeVideo:
            self.v_thread.join()

        print 'Call ended'

    def on_peer_timeout(self):
        self.stop_call()

    def audio_transmission(self):
        print("Starting audio transmission...")

        while not self.stop:
            try:
                ret = self.recv_audio()
                if ret:
                    sys.stdout.write('.')
                    sys.stdout.flush()
                    self.send_audio(ret["size"], ret["data"])
            except Exception as e:
                print(e)

            sleep(0.001)

    def video_transmission(self):
        print("Starting video transmission...")

        while not self.stop:
            try:
                vret = self.recv_video()
                if vret:
                    sys.stdout.write('*')
                    sys.stdout.flush()
                    self.send_video(vret['data'])
            except Exception as e:
                print(e)

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

                self.do()
                sleep(0.01)
        except KeyboardInterrupt:
            self.save_to_file(DATA)

    def on_friend_request(self, pk, message):
        print('Friend request from %s: %s' % (pk, message))
        self.add_friend_norequest(pk)
        print('Accepted.')

    def on_friend_message(self, friendId, message):
        name = self.get_name(friendId)
        print('%s: %s' % (name, message))
        print('EchoBot: %s' % message)
        self.send_message(friendId, message)

if len(sys.argv) == 2:
    DATA = sys.argv[1]

t = EchoBot()
t.loop()

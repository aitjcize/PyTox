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
from random import randint

SERVER = ["54.199.139.199", 33445, "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"]

DATA = 'echo.data'

class AV(ToxAV):
    def __init__(self, core, max_calls):
        self.core = self.get_tox()
        self.cs = None
        self.call_type = self.TypeAudio

    def on_invite(self, idx):
        self.cs = self.get_peer_csettings(idx, 0)
        self.call_type = self.cs["call_type"]

        print("Incoming %s call from %d:%s ..." % (
                "video" if self.call_type == self.TypeVideo else "audio", idx,
                self.core.get_name(self.get_peer_id(idx, 0))))

        self.answer(idx, self.call_type)
        print("Answered, in call...")

    def on_start(self, idx):
        try:
            self.change_settings(idx, {"max_video_width": 1920,
                                       "max_video_height": 1080})
        except: pass
        self.prepare_transmission(idx, self.jbufdc * 2, self.VADd,
                True if self.call_type == self.TypeVideo else False)

    def on_end(self, idx):
        self.kill_transmission(idx)
        print('Call ended')

    def on_cancel(self, idx):
        self.kill_transmission(idx)

    def on_peer_timeout(self, idx):
        self.kill_transmission(idx)
        self.stop_call(idx)

    def on_audio_data(self, idx, size, data):
        sys.stdout.write('.')
        sys.stdout.flush()
        self.send_audio(idx, size, data)

    def on_video_data(self, idx, width, height, data):
        sys.stdout.write('*')
        sys.stdout.flush()
        self.send_video(idx, width, height, data)

class EchoBot(Tox):
    def __init__(self):
        if exists(DATA):
            self.load_from_file(DATA)

        self.set_name("EchoBot")
        print('ID: %s' % self.get_address())

        self.connect()
        self.av = AV(self, 1)

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

#
# @file   echo.py
# @author Wei-Ning Huang (AZ) <aitjcize@gmail.com>
#
# Copyright (C) 2013 - 2014 Wei-Ning Huang (AZ) <aitjcize@gmail.com>
# All Rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
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

from __future__ import print_function

import sys
from pytox import Tox, ToxAV

from time import sleep
from os.path import exists

SERVER = [
    "54.199.139.199",
    33445,
    "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"
]

DATA = 'echo.data'

# echo.py features
# accept friend request
# echo back friend message
# accept and answer friend call request
# send back friend audio/video data


class AV(ToxAV):
    def __init__(self, core):
        super(AV, self).__init__(core)
        self.core = self.get_tox()
        self.cs = None
        # self.call_type = self.TypeAudio
        iti = self.iteration_interval()
        print(iti)

    def on_call(self, friend_number, audio_enabled, video_enabled):
        print("Incoming call: %d, %d, %d" % (friend_number, audio_enabled, video_enabled))
        self.answer(friend_number, 16, 64)
        print("Answered, in call...")

    def on_call_state(self, friend_number, state):
        print('call state:%d,%d' % (friend_number, state))

    def on_invite(self, idx):
        self.cs = self.get_peer_csettings(idx, 0)
        # self.call_type = self.cs["call_type"]

        print("Incoming %s call from %d:%s ..." % (
            "video" if self.call_type == self.TypeVideo else "audio", idx,
            self.core.get_name(self.get_peer_id(idx, 0))))

        self.answer(idx, self.call_type)
        print("Answered, in call...")

    def on_start(self, idx):
        try:
            self.change_settings(idx, {"max_video_width": 1920,
                                       "max_video_height": 1080})
        except:
            pass

        video_enabled = True if self.call_type == self.TypeVideo else False
        self.prepare_transmission(idx, vide_enabled)

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


class ToxOptions():
    def __init__(self):
        self.ipv6_enabled = True
        self.udp_enabled = True
        self.proxy_type = 0  # 1=http, 2=socks
        self.proxy_host = ''
        self.proxy_port = 0
        self.start_port = 0
        self.end_port = 0
        self.tcp_port = 0
        self.savedata_type = 0  # 1=toxsave, 2=secretkey
        self.savedata_data = b''
        self.savedata_length = 0


def save_to_file(tox, fname):
    data = tox.get_savedata()
    with open(fname, 'wb') as f:
        f.write(data)


def load_from_file(fname):
    return open(fname, 'rb').read()


class EchoBot(Tox):
    def __init__(self, opts=None):
        if opts is not None:
            super(EchoBot, self).__init__(opts)

        self.self_set_name("EchoBot")
        print('ID: %s' % self.self_get_address())

        self.connect()
        self.av = AV(self)

    def connect(self):
        print('connecting...')
        self.bootstrap(SERVER[0], SERVER[1], SERVER[2])

    def loop(self):
        checked = False
        save_to_file(self, DATA)

        try:
            while True:
                status = self.self_get_connection_status()

                if not checked and status:
                    print('Connected to DHT.')
                    checked = True

                if checked and not status:
                    print('Disconnected from DHT.')
                    self.connect()
                    checked = False

                self.iterate()
                sleep(0.01)
        except KeyboardInterrupt:
            save_to_file(self, DATA)

    def on_friend_request(self, pk, message):
        print('Friend request from %s: %s' % (pk, message))
        self.friend_add_norequest(pk)
        print('Accepted.')
        save_to_file(self, DATA)

    def on_friend_message(self, friendId, type, message):
        name = self.friend_get_name(friendId)
        print('%s: %s' % (name, message))
        print('EchoBot: %s' % message)
        self.friend_send_message(friendId, Tox.MESSAGE_TYPE_NORMAL, message)


opts = None
if len(sys.argv) == 2:
    DATA = sys.argv[1]
    if exists(DATA):
        opts = ToxOptions()
        opts.savedata_data = load_from_file(DATA)
        opts.savedata_length = len(opts.savedata_data)
        opts.savedata_type = Tox.SAVEDATA_TYPE_TOX_SAVE

t = EchoBot(opts)
t.loop()

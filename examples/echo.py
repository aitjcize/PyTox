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

SERVER = ["54.199.139.199", 33445, "7F9C31FE850E97CEFD4C4591DF93FC757C7C12549DDD55F8EEAECC34FE76C029"]

class AV(ToxAV):
    def __init__(self, core, width, height):
        super(AV, self).__init__(core, width, height)
        self.core = self.get_tox()
        self.daemon = True
        self.stop = True

    def on_invite(self):
        print 'Incoming call from %s ...' % self.core.get_name(
                self.get_peer_id(0))
        self.answer(self.TypeAudio)

    def on_start(self):
        self.prepare_transmission(False)
        self.stop = False
        self.thread = Thread(target=self.transmission)
        self.thread.daemon = True
        self.thread.start()

    def on_end(self):
        self.stop = True
        self.kill_transmission()
        self.thread.join()
        print 'Call ended'

    def on_peer_timeout(self):
        self.stop_call()

    def transmission(self):
        print "Starting transmission..."

        while not self.stop:
            ret = self.recv_audio()
            if ret:
                sys.stdout.write('.')
                sys.stdout.flush()
                self.send_audio(ret["size"], ret["data"])

            sleep(0.001)


class EchoBot(Tox):
    def __init__(self):
        if exists('data'):
            self.load_from_file('data')

        self.set_name("EchoBot")
        print('ID: %s' % self.get_address())

        self.connect()
        self.av = AV(self, 480, 320)

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
            self.save_to_file('data')

    def on_friend_request(self, pk, message):
        print('Friend request from %s: %s' % (pk, message))
        self.add_friend_norequest(pk)
        print('Accepted.')

    def on_friend_message(self, friendId, message):
        name = self.get_name(friendId)
        print('%s: %s' % (name, message))
        print('EchoBot: %s' % message)
        self.send_message(friendId, message)

t = EchoBot()
t.loop()

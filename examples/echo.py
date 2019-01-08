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
    "192.210.149.121",
    33445,
    "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67"
]

DATA = 'echo.data'

# echo.py features
# - accept friend request
# - echo back friend message
# - accept and answer friend call request
# - send back friend audio/video data
# - send back files friend sent


class AV(ToxAV):
    def __init__(self, core):
        super(AV, self).__init__(core)
        self.core = self.get_tox()

    def on_call(self, fid, audio_enabled, video_enabled):
        print("Incoming %s call from %d:%s ..." % (
            "video" if video_enabled else "audio", fid,
            self.core.friend_get_name(fid)))
        bret = self.answer(fid, 48, 64)
        print("Answered, in call..." + str(bret))

    def on_call_state(self, fid, state):
        print('call state:fn=%d, state=%d' % (fid, state))

    def on_audio_bit_rate(self, fid, video_bit_rate):
        print('audio bit rate status: fn=%d, abr=%d' %
              (fid, audio_bit_rate))

    def on_video_bit_rate(self, fid, video_bit_rate):
        print('video bit rate status: fn=%d, vbr=%d' %
              (fid, video_bit_rate))

    def on_audio_receive_frame(self, fid, pcm, sample_count,
                               channels, sampling_rate):
        # print('audio frame: %d, %d, %d, %d' %
        #      (fid, sample_count, channels, sampling_rate))
        # print('pcm len:%d, %s' % (len(pcm), str(type(pcm))))
        sys.stdout.write('.')
        sys.stdout.flush()
        bret = self.audio_send_frame(fid, pcm, sample_count,
                                     channels, sampling_rate)
        if bret is False:
            pass

    def on_video_receive_frame(self, fid, width, height, frame):
        # print('video frame: %d, %d, %d, ' % (fid, width, height))
        sys.stdout.write('*')
        sys.stdout.flush()
        bret = self.video_send_frame(fid, width, height, frame)
        if bret is False:
            print('video send frame error.')
            pass

    def witerate(self):
        self.iterate()


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

        self.files = {}

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

                self.av.witerate()
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

    def on_file_recv(self, fid, filenumber, kind, size, filename):
        print (fid, filenumber, kind, size, filename)
        if size == 0:
            return

        self.files[(fid, filenumber)] = {
            'f': bytes(),
            'filename': filename,
            'size': size
        }

        self.file_control(fid, filenumber, Tox.FILE_CONTROL_RESUME)

    def on_file_recv_chunk(self, fid, filenumber, position, data):
        filename = self.files[(fid, filenumber)]['filename']
        size = self.files[(fid, filenumber)]['size']
        print (fid, filenumber, filename, position/float(size)*100)

        if data is None:
            msg = "I got '{}', sending it back right away!".format(filename)
            self.friend_send_message(fid, Tox.MESSAGE_TYPE_NORMAL, msg)

            self.files[(fid, 0)] = self.files[(fid, filenumber)]

            length = self.files[(fid, filenumber)]['size']
            self.file_send(fid, 0, length, filename, filename)

            del self.files[(fid, filenumber)]
            return

        self.files[(fid, filenumber)]['f'] += data

    def on_file_chunk_request(self, fid, filenumber, position, length):
        if length == 0:
            return

        data = self.files[(fid, filenumber)]['f'][position:(position + length)]
        self.file_send_chunk(fid, filenumber, position, data)


opts = None
opts = ToxOptions()
opts.udp_enabled = True

if len(sys.argv) == 2:
    DATA = sys.argv[1]

if exists(DATA):
    opts.savedata_data = load_from_file(DATA)
    opts.savedata_length = len(opts.savedata_data)
    opts.savedata_type = Tox.SAVEDATA_TYPE_TOX_SAVE

t = EchoBot(opts)
t.loop()

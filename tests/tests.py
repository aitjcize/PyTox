#
# @file   tests.py
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

import hashlib
import os
import re
import sys
import unittest

from pytox import Tox, OperationFailedError
from time import sleep

ADDR_SIZE = 76
CLIENT_ID_SIZE = 64


def unittest_skip(reason):
    def _wrap1(func):
        def _wrap2(self, *args, **kwargs):
            pass
        return _wrap2
    return _wrap1


def patch_unittest():
    major, minor, micro, release, serial = sys.version_info
    if major == 2 and minor <= 6:
        unittest.skip = unittest_skip

# Patch unittest for Python version <= 2.6
patch_unittest()


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


class AliceTox(Tox):
    def __init__(self, opts):
        super(AliceTox, self).__init__(opts)


class BobTox(Tox):
    def __init__(self, opts):
        super(BobTox, self).__init__(opts)


class ToxTest(unittest.TestCase):
    def setUp(self):
        opt = ToxOptions()
        self.alice = AliceTox(opt)
        self.bob = BobTox(opt)

        self.loop_until_connected()

    def tearDown(self):
        """
        t:kill
        """
        self.alice.kill()
        self.bob.kill()

    def loop(self, n):
        """
        t:iterate
        t:iteration_interval
        """
        interval = self.bob.iteration_interval()
        for i in range(n):
            self.alice.iterate()
            self.bob.iterate()
            sleep(interval / 2000.0)

    def loop_until_connected(self):
        """
        t:on_self_connection_status
        t:self_get_connection_status
        """

        def on_self_connection_status(self, status):
            if status != Tox.CONNECTION_NONE:
                self.mycon_status = True
            else:
                self.mycon_status = False
            assert self.self_get_connection_status() == status

        self.alice.mycon_status = False
        self.bob.mycon_status = False
        AliceTox.on_self_connection_status = on_self_connection_status
        BobTox.on_self_connection_status = on_self_connection_status

        while not self.alice.mycon_status or not self.bob.mycon_status:
            self.loop(50)

        AliceTox.on_self_connection_status = Tox.on_self_connection_status
        BobTox.on_self_connection_status = Tox.on_self_connection_status

    def wait_callback(self, obj, attr):
        count = 0
        THRESHOLD = 200

        while not getattr(obj, attr):
            self.loop(50)
            if count >= THRESHOLD:
                return False
            count += 1

        return True

    def wait_callbacks(self, obj, attrs):
        count = 0
        THRESHOLD = 400

        while not all([getattr(obj, attr) for attr in attrs]):
            self.loop(50)
            if count >= THRESHOLD:
                return False
            count += 1

        return True

    def ensure_exec(self, method, args):
        count = 0
        THRESHOLD = 200

        while True:
            try:
                ret = method(*args)
                break
            except Exception as ex:
                self.loop(50)
                assert count < THRESHOLD
                count += 1

        return ret

    def bob_add_alice_as_friend(self):
        """
        t:friend_add
        t:friend_add_norequest
        t:on_friend_request
        t:friend_by_public_key
        """
        MSG = 'Hi, this is Bob.'
        bob_addr = self.bob.self_get_address()

        def on_friend_request(self, pk, message):
            assert pk == bob_addr[:CLIENT_ID_SIZE]
            assert message == MSG
            self.friend_add_norequest(pk)
            self.friend_added = True

        AliceTox.on_friend_request = on_friend_request

        alice_addr = self.alice.self_get_address()
        self.alice.friend_added = False
        self.bob.friend_add(alice_addr, MSG)

        assert self.wait_callback(self.alice, 'friend_added')
        AliceTox.on_friend_request = Tox.on_friend_request

        self.bid = self.alice.friend_by_public_key(bob_addr)
        self.aid = self.bob.friend_by_public_key(alice_addr)

        #: Wait until both are online
        def on_friend_connection_status(self, friend_id, status):
            assert status is True
            self.friend_conn_status = True

        def on_friend_status(self, friend_id, new_status):
            self.friend_status = True

        AliceTox.on_friend_connection_status = on_friend_connection_status
        BobTox.on_friend_connection_status = on_friend_connection_status
        AliceTox.on_friend_status = on_friend_status
        BobTox.on_friend_status = on_friend_status

        self.alice.friend_conn_status = False
        self.bob.friend_conn_status = False
        self.alice.friend_status = False
        self.bob.friend_status = False

        assert self.wait_callbacks(self.alice,
                                   ['friend_conn_status', 'friend_status'])
        assert self.wait_callbacks(self.bob,
                                   ['friend_conn_status', 'friend_status'])

        AliceTox.on_friend_connection_status = Tox.on_friend_connection_status
        BobTox.on_friend_connection_status = Tox.on_friend_connection_status
        AliceTox.on_friend_status = Tox.on_friend_status
        BobTox.on_friend_status = Tox.on_friend_status

    def test_bootstrap(self):
        """
        t:bootstrap
        """
        assert self.alice.self_get_connection_status() != Tox.CONNECTION_NONE
        assert self.bob.self_get_connection_status() != Tox.CONNECTION_NONE

    def test_address(self):
        """
        t:self_get_address
        t:self_get_nospam
        t:self_set_nospam
        t:self_get_keys
        """
        assert len(self.alice.self_get_address()) == ADDR_SIZE
        assert len(self.bob.self_get_address()) == ADDR_SIZE

        self.alice.self_set_nospam(0x12345678)
        assert self.alice.self_get_nospam() == 0x12345678

        pk, sk = self.alice.self_get_keys()
        assert pk == self.alice.self_get_address()[:CLIENT_ID_SIZE]

    def test_self_name(self):
        """
        t:self_set_name
        t:self_get_name
        t:self_get_name_size
        """
        self.alice.self_set_name('Alice')
        self.loop(10)
        assert self.alice.self_get_name() == 'Alice'
        assert self.alice.self_get_name_size() == len('Alice')

    def test_status_message(self):
        """
        t:self_set_status_message
        t:self_get_status_message
        t:self_get_status_message_size
        t:friend_set_status_message
        t:friend_get_status_message
        t:friend_get_status_message_size
        t:on_friend_status_message
        """
        self.bob_add_alice_as_friend()

        MSG = 'Happy'
        AID = self.aid

        def on_friend_status_message(self, friend_id, new_message):
            assert friend_id == AID
            assert new_message == MSG
            self.sm = True

        BobTox.on_friend_status_message = on_friend_status_message
        self.bob.sm = False

        self.alice.self_set_status_message(MSG)
        assert self.wait_callback(self.bob, 'sm')
        BobTox.on_friend_status_message = Tox.on_friend_status_message

        assert self.alice.self_get_status_message() == MSG
        assert self.alice.self_get_status_message_size() == len(MSG)
        assert self.bob.friend_get_status_message(self.aid) == MSG
        assert self.bob.friend_get_status_message_size(self.aid) == len(MSG)

    def test_user_status(self):
        """
        t:self_get_status
        t:self_set_status
        t:friend_get_status
        t:friend_get_status
        t:on_friend_status
        """
        self.bob_add_alice_as_friend()

        AID = self.aid

        def on_friend_status(self, friend_id, new_status):
            assert friend_id == AID
            assert new_status == Tox.USER_STATUS_BUSY
            self.friend_status = True

        self.alice.self_set_status(Tox.USER_STATUS_BUSY)

        BobTox.on_friend_status = on_friend_status
        self.bob.friend_status = False
        assert self.wait_callback(self.bob, 'friend_status')
        BobTox.on_friend_status = Tox.on_friend_status

        assert self.alice.self_get_status() == Tox.USER_STATUS_BUSY
        assert self.bob.friend_get_status(self.aid) == Tox.USER_STATUS_BUSY

    def test_connection_status(self):
        """
        t:friend_get_connection_status
        t:on_friend_connection_status
        """
        self.bob_add_alice_as_friend()

        AID = self.aid

        def on_friend_connection_status(self, friend_id, status):
            assert friend_id == AID
            assert status is False
            self.friend_conn_status = True

        opt = ToxOptions()
        BobTox.on_friend_connection_status = on_friend_connection_status
        self.bob.friend_conn_status = False
        self.alice.kill()
        self.alice = Tox(opt)
        assert self.wait_callback(self.bob, 'friend_conn_status')
        BobTox.on_friend_connection_status = Tox.on_friend_connection_status

        assert self.bob.friend_get_connection_status(self.aid) is False

    def test_tox(self):
        """
        t:get_savedata_size
        t:get_savedata
        """
        assert self.alice.get_savedata_size() > 0
        data = self.alice.get_savedata()
        assert data is not None
        addr = self.alice.self_get_address()

        self.alice.kill()

        opt = ToxOptions()
        opt.savedata_data = data
        opt.savedata_length = len(data)

        self.alice = Tox(opt)
        assert addr == self.alice.self_get_address()

    def test_friend(self):
        """
        t:friend_delete
        t:friend_exists
        t:friend_get_public_key
        t:self_get_friend_list
        t:self_get_friend_list_size
        t:self_set_name
        t:friend_get_name
        t:friend_get_name_size
        t:on_friend_name
        """

        #: Test friend request
        self.bob_add_alice_as_friend()

        assert self.alice.friend_exists(self.bid)
        assert self.bob.friend_exists(self.aid)

        #: Test friend exists
        assert not self.alice.friend_exists(self.bid + 1)
        assert not self.bob.friend_exists(self.aid + 1)

        #: Test friend_get_public_key
        assert self.alice.friend_get_public_key(self.bid) == \
            self.bob.self_get_address()[:CLIENT_ID_SIZE]
        assert self.bob.friend_get_public_key(self.aid) == \
            self.alice.self_get_address()[:CLIENT_ID_SIZE]

        #: Test self_get_friend_list
        assert self.alice.self_get_friend_list() == [self.bid]
        assert self.bob.self_get_friend_list() == [self.aid]
        assert self.alice.self_get_friend_list_size() == 1
        assert self.bob.self_get_friend_list_size() == 1

        #: Test friend name
        NEWNAME = 'Jenny'
        AID = self.aid

        def on_friend_name(self, fid, newname):
            assert fid == AID
            assert newname == NEWNAME
            self.nc = True

        BobTox.on_friend_name = on_friend_name

        self.bob.nc = False
        self.alice.self_set_name(NEWNAME)

        assert self.wait_callback(self.bob, 'nc')
        assert self.bob.friend_get_name(self.aid) == NEWNAME
        assert self.bob.friend_get_name_size(self.aid) == len(NEWNAME)
        BobTox.on_friend_name = Tox.on_friend_name

    def test_friend_message_and_action(self):
        """
        t:on_friend_action
        t:on_friend_message
        t:friend_send_message
        """
        self.bob_add_alice_as_friend()

        #: Test message
        MSG = 'Hi, Bob!'
        BID = self.bid

        def on_friend_message(self, fid, msg_type, message):
            assert fid == BID
            assert message == MSG
            self.fm = True

        AliceTox.on_friend_message = on_friend_message

        self.ensure_exec(self.bob.friend_send_message, (self.aid, Tox.MESSAGE_TYPE_NORMAL, MSG))
        self.alice.fm = False
        assert self.wait_callback(self.alice, 'fm')

        AliceTox.on_friend_message = Tox.on_friend_message

        #: Test action
        ACTION = 'Kick'
        BID = self.bid

        def on_friend_action(self, fid, msg_type, action):
            assert fid == BID
            assert action == ACTION
            self.fa = True

        AliceTox.on_friend_message = on_friend_action

        self.ensure_exec(self.bob.friend_send_message, (self.aid, Tox.MESSAGE_TYPE_ACTION, ACTION))
        self.alice.fa = False
        assert self.wait_callback(self.alice, 'fa')

        AliceTox.on_friend_message = Tox.on_friend_message

        #: Test delete friend
        self.alice.friend_delete(self.bid)
        self.loop(10)
        assert not self.alice.friend_exists(self.bid)

    def test_meta_status(self):
        """
        t:on_friend_read_receipt
        t:on_friend_typing
        t:self_set_typing
        t:friend_get_typing
        t:friend_get_last_online
        """
        self.bob_add_alice_as_friend()

        AID = self.aid

        #: Test typing status
        def on_friend_typing(self, fid, is_typing):
            assert fid == AID
            assert is_typing is True
            assert self.friend_get_typing(fid) is True
            self.friend_typing = True

        BobTox.on_friend_typing = on_friend_typing
        self.bob.friend_typing = False
        self.alice.self_set_typing(self.bid, True)
        assert self.wait_callback(self.bob, 'friend_typing')
        BobTox.on_friend_typing = Tox.on_friend_typing

        #: Test last online
        assert self.alice.friend_get_last_online(self.bid) is not None
        assert self.bob.friend_get_last_online(self.aid) is not None

    def test_group(self):
        """
        t:add_groupchat
        t:count_chatlist
        t:del_groupchat
        t:get_chatlist
        t:group_action_send
        t:group_get_names
        t:group_get_title
        t:group_get_type
        t:group_message_send
        t:group_number_peers
        t:group_peername
        t:group_set_title
        t:invite_friend
        t:join_groupchat
        t:on_group_action
        t:on_group_invite
        t:on_group_message
        t:on_group_namelist_change
        """
        self.bob_add_alice_as_friend()

        #: Test group add
        group_id = self.bob.add_groupchat()
        assert group_id >= 0

        self.loop(50)

        BID = self.bid

        def on_group_invite(self, fid, type_, data):
            assert fid == BID
            assert type_ == 0
            gn = self.join_groupchat(fid, data)
            assert type_ == self.group_get_type(gn)
            self.gi = True

        AliceTox.on_group_invite = on_group_invite

        def on_group_namelist_change(self, gid, peer_number, change):
            assert gid == group_id
            assert change == Tox.CHAT_CHANGE_PEER_ADD
            self.gn = True

        AliceTox.on_group_namelist_change = on_group_namelist_change

        self.alice.gi = False
        self.alice.gn = False

        self.ensure_exec(self.bob.invite_friend, (self.aid, group_id))

        assert self.wait_callbacks(self.alice, ['gi', 'gn'])

        AliceTox.on_group_invite = Tox.on_group_invite
        AliceTox.on_group_namelist_change = Tox.on_group_namelist_change

        #: Test group number of peers
        self.loop(50)
        assert self.bob.group_number_peers(group_id) == 2

        #: Test group peername
        self.alice.self_set_name('Alice')
        self.bob.self_set_name('Bob')

        def on_group_namelist_change(self, gid, peer_number, change):
            if change == Tox.CHAT_CHANGE_PEER_NAME:
                self.gn = True

        AliceTox.on_group_namelist_change = on_group_namelist_change
        self.alice.gn = False

        assert self.wait_callback(self.alice, 'gn')
        AliceTox.on_group_namelist_change = Tox.on_group_namelist_change

        peernames = [self.bob.group_peername(group_id, i) for i in
                     range(self.bob.group_number_peers(group_id))]

        assert 'Alice' in peernames
        assert 'Bob' in peernames

        assert sorted(self.bob.group_get_names(group_id)) == ['Alice', 'Bob']

        #: Test title change
        self.bob.group_set_title(group_id, 'My special title')
        assert self.bob.group_get_title(group_id) == 'My special title'

        #: Test group message
        AID = self.aid
        BID = self.bid
        MSG = 'Group message test'

        def on_group_message(self, gid, fgid, message):
            if fgid == AID:
                assert gid == group_id
                assert message == MSG
                self.gm = True

        AliceTox.on_group_message = on_group_message
        self.alice.gm = False

        self.ensure_exec(self.bob.group_message_send, (group_id, MSG))

        assert self.wait_callback(self.alice, 'gm')
        AliceTox.on_group_message = Tox.on_group_message

        #: Test group action
        AID = self.aid
        BID = self.bid
        MSG = 'Group action test'

        def on_group_action(self, gid, fgid, action):
            if fgid == AID:
                assert gid == group_id
                assert action == MSG
                self.ga = True

        AliceTox.on_group_action = on_group_action
        self.alice.ga = False

        self.ensure_exec(self.bob.group_action_send, (group_id, MSG))

        assert self.wait_callback(self.alice, 'ga')
        AliceTox.on_group_action = Tox.on_group_action

        #: Test chatlist
        assert len(self.bob.get_chatlist()) == self.bob.count_chatlist()
        assert len(self.alice.get_chatlist()) == self.bob.count_chatlist()

        assert self.bob.count_chatlist() == 1
        self.bob.del_groupchat(group_id)
        assert self.bob.count_chatlist() == 0

    def test_file_transfer(self):
        """
        t:file_send
        t:file_send_chunk
        t:file_control
        t:file_seek
        t:file_get_file_id
        t:on_file_recv
        t:on_file_recv_control
        t:on_file_recv_chunk
        t:on_file_chunk_request
        """
        self.bob_add_alice_as_friend()

        FILE = os.urandom(1024 * 1024)
        FILE_NAME = "test.bin"
        FILE_SIZE = len(FILE)
        OFFSET = 567

        m = hashlib.md5()
        m.update(FILE[OFFSET:])
        FILE_DIGEST = m.hexdigest()

        BID = self.bid
        CONTEXT = {'FILE': bytes(), 'RECEIVED': 0, 'START': False, 'SENT': 0}

        self.alice.completed = False
        self.bob.completed = False

        def on_file_recv(self, fid, filenumber, kind, size, filename):
            assert fid == BID
            assert size == FILE_SIZE
            assert filename == FILE_NAME
            retv = self.file_seek(fid, filenumber, OFFSET)
            assert retv is True
            self.file_control(fid, filenumber, Tox.FILE_CONTROL_RESUME)

        def on_file_recv_control(self, fid, file_number, control):
            assert fid == BID
        #     if receive_send == 0 and ct == Tox.FILE_CONTROL_FINISHED:
        #         assert CONTEXT['RECEIVED'] == FILE_SIZE
        #         m = hashlib.md5()
        #         m.update(CONTEXT['FILE'])
        #         assert m.hexdigest() == FILE_DIGEST
        #         self.completed = True

        def on_file_recv_chunk(self, fid, file_number, position, data):
            assert fid == BID
            if data is None:
                assert CONTEXT['RECEIVED'] == (FILE_SIZE - OFFSET)
                m = hashlib.md5()
                m.update(CONTEXT['FILE'])
                assert m.hexdigest() == FILE_DIGEST
                self.completed = True
                self.file_control(fid, file_number, Tox.FILE_CONTROL_CANCEL)
                return
            CONTEXT['FILE'] += data
            CONTEXT['RECEIVED'] += len(data)
            # if CONTEXT['RECEIVED'] < FILE_SIZE:
            #    assert self.file_data_remaining(
            #        fid, file_number, 1) == FILE_SIZE - CONTEXT['RECEIVED']

        # AliceTox.on_file_send_request = on_file_send_request
        # AliceTox.on_file_control = on_file_control
        # AliceTox.on_file_data = on_file_data

        AliceTox.on_file_recv = on_file_recv
        AliceTox.on_file_recv_control = on_file_recv_control
        AliceTox.on_file_recv_chunk = on_file_recv_chunk

        def on_file_recv_control2(self, fid, file_number, control):
            if control == Tox.FILE_CONTROL_RESUME:
                CONTEXT['START'] = True
            elif control == Tox.FILE_CONTROL_CANCEL:
                self.completed = True
                pass

        def on_file_chunk_request(self, fid, file_number, position, length):
            if length == 0:
                return
            data = FILE[position:(position + length)]
            self.file_send_chunk(fid, file_number, position, data)

        BobTox.on_file_recv_control = on_file_recv_control2
        BobTox.on_file_chunk_request = on_file_chunk_request

        FN = self.bob.file_send(self.aid, 0, FILE_SIZE, FILE_NAME, FILE_NAME)
        FID = self.bob.file_get_file_id(self.aid, FN)
        hexFID = "".join("{:02x}".format(ord(c)) for _, c in enumerate(FILE_NAME))
        assert FID.startswith(hexFID.upper())

        while not self.alice.completed and not self.bob.completed:
            self.alice.iterate()
            self.bob.iterate()
            sleep(0.02)

        AliceTox.on_file_recv = Tox.on_file_recv
        AliceTox.on_file_recv_control = Tox.on_file_recv_control
        AliceTox.on_file_recv_chunk = Tox.on_file_recv_chunk
        BobTox.on_file_recv_control = Tox.on_file_recv_control
        BobTox.on_file_chunk_request = Tox.on_file_chunk_request

if __name__ == '__main__':
    methods = set([x for x in dir(Tox)
                  if not x[0].isupper() and not x[0] == '_'])
    docs = "".join([getattr(ToxTest, x).__doc__ for x in dir(ToxTest)
                    if getattr(ToxTest, x).__doc__ is not None])

    tested = set(re.findall(r't:(.*?)\n', docs))
    not_tested = methods.difference(tested)

    print('Test Coverage: %.2f%%' % (len(tested) * 100.0 / len(methods)))
    if len(not_tested):
        print('Not tested:\n    %s' % "\n    ".join(sorted(list(not_tested))))

    unittest.main()

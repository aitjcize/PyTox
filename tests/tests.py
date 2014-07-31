import hashlib
import os
import re
import unittest

from tox import Tox, OperationFailedError
from time import sleep

ADDR_SIZE = 76
CLIENT_ID_SIZE = 64

class AliceTox(Tox):
    pass

class BobTox(Tox):
    pass

class ToxTest(unittest.TestCase):
    def setUp(self):
        self.alice = AliceTox()
        self.bob = BobTox()

        self.loop_until_connected()

    def tearDown(self):
        """
        t:kill
        """
        self.alice.kill()
        self.bob.kill()

    def loop(self, n):
        """
        t:do
        t:do_interval
        """
        interval = self.bob.do_interval()
        for i in range(n):
            self.alice.do()
            self.bob.do()
            sleep(interval / 1000.0)

    def loop_until_connected(self):
        """
        t:isconnected
        """
        while not self.alice.isconnected() or not self.bob.isconnected():
            self.loop(50)

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
            except:
                self.loop(50)
                assert count < THRESHOLD
                count += 1

        return ret

    def bob_add_alice_as_friend(self):
        """
        t:add_friend
        t:add_friend_norequest
        t:on_friend_request
        t:get_friend_id
        """
        MSG = 'Hi, this is Bob.'
        bob_addr = self.bob.get_address()

        def on_friend_request(self, pk, message):
            assert pk == bob_addr[:CLIENT_ID_SIZE]
            assert message == MSG
            self.add_friend_norequest(pk)
            self.fr = True

        AliceTox.on_friend_request = on_friend_request

        alice_addr = self.alice.get_address()
        self.alice.fr = False
        self.bob.add_friend(alice_addr, MSG)

        assert self.wait_callback(self.alice, 'fr')
        AliceTox.on_friend_request = Tox.on_friend_request

        self.bid = self.alice.get_friend_id(bob_addr)
        self.aid = self.bob.get_friend_id(alice_addr)

        #: Wait until both are online
        def on_connection_status(self, friend_id, status):
            assert status == True
            self.cs = True

        def on_user_status(self, friend_id, new_status):
            self.us = True

        AliceTox.on_connection_status = on_connection_status
        BobTox.on_connection_status = on_connection_status
        AliceTox.on_user_status = on_user_status
        BobTox.on_user_status = on_user_status

        self.alice.cs = False
        self.bob.cs = False
        self.alice.us = False
        self.bob.us = False

        assert self.wait_callbacks(self.alice, ['cs', 'us'])
        assert self.wait_callbacks(self.bob, ['cs', 'us'])

        AliceTox.on_connection_status = Tox.on_connection_status
        BobTox.on_connection_status = Tox.on_connection_status
        AliceTox.on_user_status = Tox.on_user_status
        BobTox.on_user_status = Tox.on_user_status

    def test_boostrap(self):
        """
        t:bootstrap_from_address
        """
        assert self.alice.isconnected()
        assert self.bob.isconnected()

    def test_address(self):
        """
        t:get_address
        t:get_nospam
        t:set_nospam
        t:get_keys
        """
        assert len(self.alice.get_address()) == ADDR_SIZE
        assert len(self.bob.get_address()) == ADDR_SIZE

        self.alice.set_nospam(0x12345678)
        assert self.alice.get_nospam() == 0x12345678

        pk, sk = self.alice.get_keys()
        assert pk == self.alice.get_address()[:CLIENT_ID_SIZE]

    def test_self_name(self):
        """
        t:set_name
        t:get_self_name
        t:get_self_name_size
        """
        self.alice.set_name('Alice')
        self.loop(10)
        assert self.alice.get_self_name() == 'Alice'
        assert self.alice.get_self_name_size() == len('Alice')

    def test_status_message(self):
        """
        t:get_self_status_message
        t:get_self_status_message_size
        t:get_status_message
        t:get_status_message_size
        t:on_status_message
        t:set_status_message
        """
        self.bob_add_alice_as_friend()

        MSG = 'Happy'
        AID = self.aid
        def on_status_message(self, friend_id, new_message):
            assert friend_id == AID
            assert new_message == MSG
            self.sm = True

        BobTox.on_status_message = on_status_message
        self.bob.sm = False

        self.alice.set_status_message(MSG)
        assert self.wait_callback(self.bob, 'sm')
        BobTox.on_status_message = Tox.on_status_message

        assert self.alice.get_self_status_message() == MSG
        assert self.alice.get_self_status_message_size() == len(MSG)
        assert self.bob.get_status_message(self.aid) == MSG
        assert self.bob.get_status_message_size(self.aid) == len(MSG)

    def test_user_status(self):
        """
        t:get_self_user_status
        t:get_user_status
        t:on_user_status
        t:set_user_status
        """
        self.bob_add_alice_as_friend()

        AID = self.aid
        def on_user_status(self, friend_id, new_status):
            assert friend_id == AID
            assert new_status == Tox.USERSTATUS_BUSY
            self.us = True

        self.alice.set_user_status(Tox.USERSTATUS_BUSY)

        BobTox.on_user_status = on_user_status
        self.bob.us = False
        assert self.wait_callback(self.bob, 'us')
        BobTox.on_user_status = Tox.on_user_status

        assert self.alice.get_self_user_status() == Tox.USERSTATUS_BUSY
        assert self.bob.get_user_status(self.aid) == Tox.USERSTATUS_BUSY

    def test_connection_status(self):
        """
        t:get_friend_connection_status
        t:on_connection_status
        """
        self.bob_add_alice_as_friend()

        AID = self.aid
        def on_connection_status(self, friend_id, status):
            assert friend_id == AID
            assert status == False
            self.cs = True

        BobTox.on_connection_status = on_connection_status
        self.bob.cs = False
        self.alice.kill()
        self.alice = Tox()
        assert self.wait_callback(self.bob, 'cs')
        BobTox.on_connection_status = Tox.on_connection_status

        assert self.bob.get_friend_connection_status(self.aid) == False

    def test_tox(self):
        """
        t:size
        t:save
        t:load
        """
        assert self.alice.size() > 0
        data = self.alice.save()
        assert data != None
        addr = self.alice.get_address()

        self.alice.kill()
        self.alice = Tox()
        self.alice.load(data)
        assert addr == self.alice.get_address()

    def test_tox_from_file(self):
        """
        t:save_to_file
        t:load_from_file
        """
        self.alice.save_to_file('data')
        addr = self.alice.get_address()

        self.alice.kill()
        self.alice = Tox()

        #: Test invalid file
        try:
            self.alice.load_from_file('not_exists')
        except OperationFailedError:
            pass
        else:
            assert False

        self.alice.load_from_file('data')

        assert addr == self.alice.get_address()

    def test_friend(self):
        """
        t:count_friendlist
        t:del_friend
        t:friend_exists
        t:get_client_id
        t:get_friendlist
        t:get_name
        t:get_name_size
        t:get_num_online_friends
        t:on_name_change
        """

        #: Test friend request
        self.bob_add_alice_as_friend()

        assert self.alice.friend_exists(self.bid)
        assert self.bob.friend_exists(self.aid)

        #: Test friend exists
        assert not self.alice.friend_exists(self.bid + 1)
        assert not self.bob.friend_exists(self.aid + 1)

        #: Test get_cliend_id
        assert self.alice.get_client_id(self.bid) == \
                self.bob.get_address()[:CLIENT_ID_SIZE]
        assert self.bob.get_client_id(self.aid) == \
                self.alice.get_address()[:CLIENT_ID_SIZE]

        #: Test friendlist
        assert self.alice.get_friendlist() == [self.bid]
        assert self.bob.get_friendlist() == [self.aid]
        assert self.alice.count_friendlist() == 1
        assert self.bob.count_friendlist() == 1
        #assert self.alice.get_num_online_friends() == 1
        #assert self.bob.get_num_online_friends() == 1

        #: Test friend name
        NEWNAME = 'Jenny'
        AID = self.aid
        def on_name_change(self, fid, newname):
            assert fid == AID
            assert newname == NEWNAME
            self.nc = True

        BobTox.on_name_change = on_name_change

        self.bob.nc = False
        self.alice.set_name(NEWNAME)

        assert self.wait_callback(self.bob, 'nc')
        assert self.bob.get_name(self.aid) == NEWNAME
        assert self.bob.get_name_size(self.aid) == len(NEWNAME)
        BobTox.on_name_change = Tox.on_name_change

    def test_friend_message_and_action(self):
        """
        t:on_friend_action
        t:on_friend_message
        t:send_action
        t:send_action_withid
        t:send_message
        t:send_message_withid
        """
        self.bob_add_alice_as_friend()

        #: Test message
        MSG = 'Hi, Bob!'
        BID = self.bid
        def on_friend_message(self, fid, message):
            assert fid == BID
            assert message == MSG
            self.fm = True

        AliceTox.on_friend_message = on_friend_message

        self.ensure_exec(self.bob.send_message, (self.aid, MSG))
        self.alice.fm = False
        assert self.wait_callback(self.alice, 'fm')

        self.ensure_exec(self.bob.send_message_withid, (self.aid, 42, MSG))
        self.alice.fm = False
        assert self.wait_callback(self.alice, 'fm')

        AliceTox.on_friend_message = Tox.on_friend_message

        #: Test action
        ACTION = 'Kick'
        BID = self.bid
        def on_friend_action(self, fid, action):
            assert fid == BID
            assert action == ACTION
            self.fa = True

        AliceTox.on_friend_action = on_friend_action

        self.ensure_exec(self.bob.send_action, (self.aid, ACTION))
        self.alice.fa = False
        assert self.wait_callback(self.alice, 'fa')

        self.ensure_exec(self.bob.send_action_withid, (self.aid, 42, ACTION))
        self.alice.fa = False
        assert self.wait_callback(self.alice, 'fa')

        AliceTox.on_friend_action = Tox.on_friend_action

        #: Test delete friend
        self.alice.del_friend(self.bid)
        self.loop(10)
        assert not self.alice.friend_exists(self.bid)

    def test_meta_status(self):
        """
        t:on_read_receipt
        t:on_typing_change
        t:set_send_receipts
        t:set_user_is_typing
        t:get_is_typing
        t:get_last_online
        """
        self.bob_add_alice_as_friend()

        #: Test send receipts
        AID = self.aid
        MSG = 'Hi, Bob!'

        checked = {'checked': False, 'MID': 0}
        def on_read_receipt(self, fid, receipt):
            assert fid == AID
            if not checked['checked']:
                checked['checked'] = True
                assert receipt == checked['MID']
            self.rr = True

        BobTox.on_read_receipt = on_read_receipt
        self.bob.rr = False

        self.bob.set_send_receipts(self.aid, True)
        checked['MID'] = self.ensure_exec(self.bob.send_message,
                (self.aid, MSG))
        assert self.wait_callback(self.bob, 'rr')

        self.bob.set_send_receipts(self.aid, False)
        BobTox.on_read_receipt = Tox.on_read_receipt

        #: Test typing status
        def on_typing_change(self, fid, is_typing):
            assert fid == AID
            assert is_typing == True
            assert self.get_is_typing(fid) == True
            self.ut = True

        BobTox.on_typing_change = on_typing_change
        self.bob.ut = False
        self.alice.set_user_is_typing(self.bid, True)
        assert self.wait_callback(self.bob, 'ut')
        BobTox.on_typing_change = Tox.on_typing_change

        #: Test last online
        assert self.alice.get_last_online(self.bid) != None
        assert self.bob.get_last_online(self.aid) != None

    def test_group(self):
        """
        t:add_groupchat
        t:count_chatlist
        t:del_groupchat
        t:get_chatlist
        t:group_action_send
        t:group_get_names
        t:group_message_send
        t:group_number_peers
        t:group_peername
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
        def on_group_invite(self, fid, pk):
            assert fid == BID
            assert len(pk) == CLIENT_ID_SIZE
            self.join_groupchat(fid, pk)
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

        assert self.wait_callback(self.alice, 'gi')
        if not self.alice.gn:
            assert self.wait_callback(self.alice, 'gn')

        AliceTox.on_group_invite = Tox.on_group_invite
        AliceTox.on_group_namelist_change = Tox.on_group_namelist_change

        #: Test group number of peers
        self.loop(50)
        assert self.bob.group_number_peers(group_id) == 2

        #: Test group peername
        self.alice.set_name('Alice')
        self.bob.set_name('Bob')

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
        t:file_data_remaining
        t:file_data_size
        t:file_send_control
        t:file_send_data
        t:new_file_sender
        t:on_file_control
        t:on_file_data
        t:on_file_send_request
        """
        self.bob_add_alice_as_friend()

        FILE = os.urandom(1024 * 1024)
        FILE_NAME = "test.bin"
        FILE_SIZE = len(FILE)

        m = hashlib.md5()
        m.update(FILE)
        FILE_DIGEST = m.hexdigest()

        BID = self.bid
        CONTEXT = {'FILE': bytes(), 'RECEIVED': 0, 'START': False, 'SENT': 0}

        def on_file_send_request(self, fid, filenumber, size, filename):
            assert fid == BID
            assert size == FILE_SIZE
            assert filename == FILE_NAME
            self.file_send_control(fid, 1, filenumber, Tox.FILECONTROL_ACCEPT)

        def on_file_control(self, fid, receive_send, file_number, ct, data):
            assert fid == BID
            if receive_send == 0 and ct == Tox.FILECONTROL_FINISHED:
                assert CONTEXT['RECEIVED'] == FILE_SIZE
                m = hashlib.md5()
                m.update(CONTEXT['FILE'])
                assert m.hexdigest() == FILE_DIGEST
                self.completed = True

        def on_file_data(self, fid, file_number, data):
            assert fid == BID
            CONTEXT['FILE'] += data
            CONTEXT['RECEIVED'] += len(data)
            if CONTEXT['RECEIVED'] < FILE_SIZE:
                assert self.file_data_remaining(fid, file_number, 1) == \
                        FILE_SIZE - CONTEXT['RECEIVED']

        AliceTox.on_file_send_request = on_file_send_request
        AliceTox.on_file_control = on_file_control
        AliceTox.on_file_data = on_file_data

        def on_file_control2(self, fid, receive_send, file_number, ct, data):
            if receive_send == 1 and ct == Tox.FILECONTROL_ACCEPT:
                CONTEXT['START'] = True

        BobTox.on_file_control = on_file_control2

        self.alice.completed = False
        BLK = self.bob.file_data_size(self.aid)
        FN = self.bob.new_file_sender(self.aid, FILE_SIZE, FILE_NAME)

        while not self.alice.completed:
            if CONTEXT['START']:
                try:
                    while True:
                        if CONTEXT['SENT'] == FILE_SIZE:
                            self.bob.file_send_control(self.aid, 0, FN,
                                    Tox.FILECONTROL_FINISHED)
                            CONTEXT['START'] = False
                            break
                        else:
                            ed = CONTEXT['SENT'] + BLK
                            if ed > FILE_SIZE:
                                ed = FILE_SIZE
                            self.bob.file_send_data(self.aid, FN,
                                    FILE[CONTEXT['SENT']:ed])
                            CONTEXT['SENT'] = ed
                except:
                    pass

            self.alice.do()
            self.bob.do()
            sleep(0.02)

        AliceTox.on_file_send_request = Tox.on_file_send_request
        AliceTox.on_file_control = Tox.on_file_control
        AliceTox.on_file_data = Tox.on_file_data
        BobTox.on_file_control = Tox.on_file_control

if __name__ == '__main__':
    methods = set([x for x in dir(Tox)
                     if not x[0].isupper() and not x[0] == '_'])
    docs = "".join([getattr(ToxTest, x).__doc__ for x in dir(ToxTest)
            if getattr(ToxTest, x).__doc__ != None])

    tested = set(re.findall(r't:(.*?)\n', docs))
    not_tested = methods.difference(tested)

    print('Test Coverage: %.2f%%' % (len(tested) * 100.0 / len(methods)))
    if len(not_tested):
        print('Not tested:\n    %s' % "\n    ".join(sorted(list(not_tested))))

    unittest.main()

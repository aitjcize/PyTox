import unittest

from tox import Tox
from time import sleep

SERVER = ["54.215.145.71", 33445, "6EDDEE2188EF579303C0766B4796DCBA89C93058B6032FEA51593DCD42FB746C"]

ADDR_SIZE = 76
CLIENT_ID_SIZE = 64

class TestTox(Tox):
    pass

class ToxTest(unittest.TestCase):
    def setUp(self):
        self.alice = TestTox()
        self.bob = TestTox()

        self.alice.bootstrap_from_address(SERVER[0], 1, SERVER[1], SERVER[2])
        self.bob.bootstrap_from_address(SERVER[0], 1, SERVER[1], SERVER[2])

        self.loop_until_connected()

    def tearDown(self):
        self.alice.kill()
        self.bob.kill()

    def loop(self, n):
        for i in range(n):
            self.alice.do()
            self.bob.do()
            sleep(0.02)

    def loop_until_connected(self):
        while not self.alice.isconnected() or not self.bob.isconnected():
            self.loop(50)

    def wait_callback(self, obj, attr):
        count = 0
        THRESHOLD = 10

        while not getattr(obj, attr):
            self.loop(50)
            if count >= THRESHOLD:
                return False
            count += 1

        return True

    def bob_add_alice_as_friend(self):
        MSG = 'Hi, this is Bob.'
        bob_addr = self.bob.get_address()

        def on_friend_request(self, pk, message):
            assert pk == bob_addr[:CLIENT_ID_SIZE]
            assert message == MSG
            self.add_friend_norequest(pk)
            self.fr = True

        TestTox.on_friend_request = on_friend_request

        alice_addr = self.alice.get_address()
        self.alice.fr = False
        self.bob.add_friend(alice_addr, MSG)

        assert self.wait_callback(self.alice, 'fr')
        TestTox.on_friend_request = Tox.on_friend_request

        self.bid = self.alice.get_friend_id(bob_addr)
        self.aid = self.bob.get_friend_id(alice_addr)

    def test_boostrap(self):
        assert self.alice.isconnected()
        assert self.bob.isconnected()

    def test_get_address(self):
        assert len(self.alice.get_address()) == ADDR_SIZE
        assert len(self.bob.get_address()) == ADDR_SIZE

    def test_friend(self):
        """
        add_friend
        add_friend_norequest
        friend_exists
        get_friend_id
        on_action
        on_friend_message
        on_friend_request
        send_action
        send_message
        """

        #: Test friend request
        self.bob_add_alice_as_friend()

        assert self.alice.friend_exists(self.bid)
        assert self.bob.friend_exists(self.aid)

        assert not self.alice.friend_exists(self.bid + 1)
        assert not self.bob.friend_exists(self.aid + 1)

        #: Test friend name
        NEWNAME = 'Jenny'
        AID = self.aid
        def on_name_change(self, fid, newname):
            assert fid == AID
            assert newname == NEWNAME
            self.nc = True

        TestTox.on_name_change = on_name_change

        self.bob.nc = False
        self.alice.set_name(NEWNAME)

        assert self.wait_callback(self.bob, 'nc')
        TestTox.on_name_change = Tox.on_name_change
        self.alice.set_name('Alice')

        #: Test message
        MSG = 'Hi, Bob!'
        BID = self.bid
        def on_friend_message(self, fid, message):
            assert fid == BID
            assert message == MSG
            self.fm = True

        TestTox.on_friend_message = on_friend_message

        self.bob.send_message(self.aid, MSG)
        self.alice.fm = False

        assert self.wait_callback(self.alice, 'fm')
        TestTox.on_friend_message = Tox.on_friend_message

        #: Test action
        ACTION = 'Kick'
        BID = self.bid
        def on_action(self, fid, action):
            assert fid == BID
            assert action == ACTION
            self.fa = True

        TestTox.on_action = on_action

        self.bob.send_action(self.aid, ACTION)
        self.alice.fa = False

        assert self.wait_callback(self.alice, 'fa')
        TestTox.on_action = Tox.on_action

    def test_group(self):
        """
        add_groupchat
        group_message_send
        group_number_peers
        group_peername
        invite_friend
        join_groupchat
        on_group_invite
        on_group_message
        on_group_namelist_change
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

        TestTox.on_group_invite = on_group_invite

        def on_group_namelist_change(self, gid, peer_number, change):
            assert gid == group_id
            assert change == Tox.CHAT_CHANGE_PEER_ADD
            self.gn = True

        TestTox.on_group_namelist_change = on_group_namelist_change

        self.alice.gi = False
        self.alice.gn = False

        while True:
            try:
                self.bob.invite_friend(self.aid, group_id)
                break
            except:
                print '!'
                self.loop(10)

        assert self.wait_callback(self.alice, 'gi')
        if not self.alice.gn:
            assert self.wait_callback(self.alice, 'gn')

        TestTox.on_group_invite = Tox.on_group_invite
        TestTox.on_group_namelist_change = Tox.on_group_namelist_change

        #: Test group number of peers
        self.loop(50)
        assert self.bob.group_number_peers(group_id) == 2

        #: Test group peername
        self.alice.set_name('Alice')
        self.bob.set_name('Bob')

        def on_group_namelist_change(self, gid, peer_number, change):
            if change == Tox.CHAT_CHANGE_PEER_NAME:
                self.gn = True

        TestTox.on_group_namelist_change = on_group_namelist_change
        self.alice.gn = False

        assert self.wait_callback(self.alice, 'gn')
        TestTox.on_group_namelist_change = Tox.on_group_namelist_change

        peernames = [self.bob.group_peername(group_id, i) for i in
                     range(self.bob.group_number_peers(group_id))]

        assert 'Alice' in peernames
        assert 'Bob' in peernames

        #: Test group message
        AID = self.aid
        BID = self.bid
        MSG = 'Group message test'
        def on_group_message(self, gid, fgid, message):
            if fgid == AID:
                assert gid == group_id
                assert message == MSG
                self.gm = True

        TestTox.on_group_message = on_group_message
        self.alice.gm = False

        while True:
            try:
                self.bob.group_message_send(group_id, MSG)
                break
            except:
                self.loop(10)

        self.wait_callback(self.alice, 'gm')
        TestTox.on_group_message = Tox.on_group_message

if __name__ == '__main__':
    unittest.main()

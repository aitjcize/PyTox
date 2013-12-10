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

    def test_boostrap(self):
        assert self.alice.isconnected()
        assert self.bob.isconnected()

    def test_get_address(self):
        assert len(self.alice.get_address()) == ADDR_SIZE
        assert len(self.bob.get_address()) == ADDR_SIZE

    def test_friend(self):
        """
        on_friend_request
        add_friend
        add_friend_norequest
        get_friend_id
        friend_exists
        send_message
        on_friend_message
        """

        #: Test friend request
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

        bid = self.alice.get_friend_id(bob_addr)
        aid = self.bob.get_friend_id(alice_addr)
        assert self.alice.friend_exists(bid)
        assert self.bob.friend_exists(aid)

        assert not self.alice.friend_exists(bid + 1)
        assert not self.bob.friend_exists(aid + 1)

        #: Test message
        MSG = 'Hi, Bob!'
        def on_friend_message(self, fid, message):
            assert fid == bid
            assert message == MSG
            self.fm = True

        TestTox.on_friend_message = on_friend_message

        self.bob.send_message(aid, MSG)
        self.alice.fm = False

        assert self.wait_callback(self.alice, 'fm')

if __name__ == '__main__':
    unittest.main()

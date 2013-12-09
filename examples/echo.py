from tox import Tox

from time import sleep
from os.path import exists

SERVER = ["54.215.145.71", 33445, "6EDDEE2188EF579303C0766B4796DCBA89C93058B6032FEA51593DCD42FB746C"]

class EchoBot(Tox):
    def __init__(self):
        if exists('data'):
            self.load_from_file('data')

        self.set_name("EchoBot")
        print('ID: %s' % self.get_address())

        self.connect()

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
                    print('Waiting for friend request')
                    checked = True

                if checked and not status:
                    print('Disconnected from DHT.')
                    self.connect()
                    checked = False

                self.do()
                sleep(0.02)
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

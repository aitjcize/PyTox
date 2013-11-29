from tox import Tox

from time import sleep
from os.path import exists

SERVER = ["192.81.133.111", 33445, "8CD5A9BF0A6CE358BA36F7A653F99FA6B258FF756E490F52C1F98CC420F78858"]

class EchoBot(Tox):
    def __init__(self):
        if exists('data'):
            self.load_from_file('data')

        self.connect()
        self.set_name("EchoBot")
        print('ID:', self.get_address())

    def connect(self):
        print('connecting...')
        self.bootstrap_from_address(SERVER[0], 0, SERVER[1], SERVER[2])

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

    def on_file_send_request(self, friendId, filenumber, size, filename):
        name = self.get_name(friendId)
        print("%s is sending a file `%s'" % (name, filename))

t = EchoBot()

print('Waiting for friend request')
t.loop()

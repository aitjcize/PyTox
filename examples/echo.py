from tox import Tox
import time

class EchoBot(Tox):
    def loop(self):
        while True:
            self.do()
            time.sleep(0.03)

    def on_friendrequest(self, pk, message):
        print 'Friend request from %s: %s' % (pk, message)
        self.addfriend_norequest(pk)
        print 'Accepted.'

    def on_friendmessage(self, friendId, message):
        name = self.getname(friendId)
        print '%s: %s' % (name, message)
        print 'EchoBot: %s' % message
        self.sendmessage(friendId, message)

t = EchoBot()

t.bootstrap_from_address("66.175.223.88", 0, 33445, "B24E2FB924AE66D023FE1E42A2EE3B432010206F751A2FFD3E297383ACF1572E")

print 'BOT ID:', t.getaddress()
t.setname("EchoBot")

print 'Waiting for friend request'
t.loop()

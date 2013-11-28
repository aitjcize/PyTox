from tox import Tox
import time

TEST_ID = "4E9D1B82DEE3BD3D4DDA62190873EA40737251A43445E4D517E66230BC4507233533EDD01F24"

class EchoTox(Tox):
    def loop(self):
        n = 0
        while True:
            self.do()
            time.sleep(0.03)

            n += 1
            if n == 100:
                self.addfriend(TEST_ID, "Hi, I'm echo tox!")

    def on_statusmessage(self, *args):
        fid = self.getfriend_id(TEST_ID)
        self.sendmessage(fid, "Hi, this is EchoTox speacking...")

    def on_friendmessage(self, friendId, message):
        name = self.getname(friendId)
        print '%s: %s' % (name, message)

t = EchoTox()

t.bootstrap_from_address("66.175.223.88", 0, 33445, "B24E2FB924AE66D023FE1E42A2EE3B432010206F751A2FFD3E297383ACF1572E")
print t.getaddress()
t.setname("PyEchoTox")

t.loop()

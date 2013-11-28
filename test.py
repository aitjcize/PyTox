from tox import Tox
import time

class EchoTox(Tox):
    def loop(self):
        n = 0
        while True:
            self.do()
            time.sleep(0.03)

            n += 1
            if n == 100:
                self.addfriend("4E9D1B82DEE3BD3D4DDA62190873EA40737251A43445E4D517E66230BC4507233533EDD01F24", "Hi")

    def on_friendrequest(self, *args):
        pass

    def on_friendmessage(self, *args):
        pass

    def on_action(self, *args):
        pass

    def on_namechange(self, *args):
        pass

    def on_statusmessage(self, *args):
        print 'on_statusmessage!!!'
        fid = self.getfriend_id("4E9D1B82DEE3BD3D4DDA62190873EA40737251A43445E4D517E66230BC4507233533EDD01F24")
        print 'fid', fid

        self.sendmessage(fid, "abcdefg")

    def on_userstatus(self, *args):
        pass

    def on_read_receipt(self, *args):
        pass

    def on_connectionstatus(self, *args):
        pass

    def on_group_invite(self, *args):
        pass

    def on_group_message(self, *args):
        pass

    def on_group_namelistchange(self, *args):
        pass

    def on_file_sendrequest(self, *args):
        pass

    def on_file_control(self, *args):
        pass

    def on_file_data(self, *args):
        pass


t = EchoTox()

t.bootstrap_from_address("66.175.223.88", 0, 33445, "B24E2FB924AE66D023FE1E42A2EE3B432010206F751A2FFD3E297383ACF1572E")
print t.getaddress()
t.setname("PyEchoTox")

t.loop()

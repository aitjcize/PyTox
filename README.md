# PyTox [![Build Status](https://travis-ci.org/aitjcize/PyTox.png?branch=master)](https://travis-ci.org/aitjcize/PyTox)
Python binding for [toxcore](https://github.com/irungentoo/ProjectTox-Core).

PyTox provides a Pythonic binding, i.e Object-oriented instead of C style, raise exception instead of returning error code.

A simple example is as follows:
```
class EchoBot(Tox):
    def loop(self):
        while True:
            self.do()
            time.sleep(0.03)
    
    def on_friend_request(self, pk, message):
        print 'Friend request from %s: %s' % (pk, message)
        self.add_friend_norequest(pk)
        print 'Accepted.'
    
    def on_friend_message(self, friendId, message):
        name = self.get_name(friendId)
        print '%s: %s' % (name, message)
        print 'EchoBot: %s' % message
        self.send_message(friendId, message)
```
As you can see callbacks are mapped into class method instead of using it the the C ways.
For more details please refer to [echo.py](examples/echo.py).

## Examples
* [echo.py](examples/echo.py): a working echo bot that wait for friend requests, and than start echoing anything that friend send.

## Documentation
Full API documentation can be read [here](http://aitjcize.github.io/PyTox).

## Contributing
1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

[![Version](https://pypip.in/version/PyTox/badge.svg)](https://pypi.python.org/pypi/PyTox)

[![Status](https://pypip.in/status/PyTox/badge.svg)](https://pypi.python.org/pypi/PyTox)

[![Image](https://cruel-carlota.pagodabox.com/f7c9269a8398926d869e54744b334c26)](http://githalytics.com/aitjcize/PyTox.git)

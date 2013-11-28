# PyTox
Python binding for [Project Tox](https://github.com/irungentoo/ProjectTox-Core)

[![Build Status](https://travis-ci.org/aitjcize/PyTox.png?branch=master)](https://travis-ci.org/aitjcize/PyTox)

PyTox is currently under development, patches are welcomed :)

There is also another project [py-toxcore](https://github.com/alexandervdm/py-toxcore) that also provide Python binding for Tox written in SWIG, but PyTox provide a more Pythonic binding, i.e Object-oriented instead of C style, raise exception instead of returning error code. A simple example is as follows:

    class EchoTox(Tox):
        def on_statusmessage(self, *args):
            fid = self.getfriend_id(TEST_ID)
            self.sendmessage(fid, "Hi, this is EchoTox speacking...")
    
        def on_friendmessage(self, friendId, message):
            name = self.getname(friendId)
            print '%s: %s' % (name, message)
	...

As you can see callbacks are mapped into class method instead of using it the the c ways. For more details please refer to `test.py`.


## Usage
* See test.py for example

## Contributing
1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/f7c9269a8398926d869e54744b334c26 "githalytics.com")](http://githalytics.com/aitjcize/PyTox.git)

.. image:: http://img.shields.io/travis/aitjcize/PyTox.svg
   :target: https://travis-ci.org/aitjcize/PyTox
.. image:: http://img.shields.io/pypi/v/PyTox.svg
   :target: https://pypi.python.org/pypi/PyTox
.. image:: http://img.shields.io/pypi/dm/PyTox.svg
   :target: https://crate.io/packages/PyTox

PyTox
=====
Python binding for `Project Tox <https://github.com/irungentoo/toxcore>`_.

PyTox provides a Pythonic binding, i.e Object-oriented instead of C style, raise exception instead of returning error code. A simple example is as follows:

.. code-block:: python

    class ToxOptions():
        def __init__(self):
            self.ipv6_enabled = True
            self.udp_enabled = True
            self.proxy_type = 0  # 1=http, 2=socks
            self.proxy_host = ''
            self.proxy_port = 0
            self.start_port = 0
            self.end_port = 0
            self.tcp_port = 0
            self.savedata_type = 0  # 1=toxsave, 2=secretkey
            self.savedata_data = b''
            self.savedata_length = 0


    class EchoBot(Tox):
        def __init__(self, opts):
            super(EchoBot, self).__init__(opts)

        def loop(self):
            while True:
                self.iterate()
                time.sleep(0.03)

        def on_friend_request(self, pk, message):
            print 'Friend request from %s: %s' % (pk, message)
            self.friend_add_norequest(pk)
            print 'Accepted.'

        def on_friend_message(self, fid, message):
            name = self.self_get_name(fid)
            print '%s: %s' % (name, message)
            print 'EchoBot: %s' % message
            self.friend_send_message(fid, Tox.MESSAGE_TYPE_NORMAL, message)

As you can see callbacks are mapped into class method instead of using it the the c ways. For more details please refer to `examples/echo.py <https://github.com/aitjcize/PyTox/blob/master/examples/echo.py>`_.


Getting started
---------------
To get started, a Makefile is provided to run PyTox inside a docker container:

- ``make test``: This will launch tests in a container.
- ``make run``: This will launch an interactive container with PyTox installed.
- ``make echobot``: This will launch the example echobot in a container.


Examples
--------
- `echo.py <https://github.com/aitjcize/PyTox/blob/master/examples/echo.py>`_: A working echo bot that wait for friend requests, and than start echoing anything that friend send.


Documentation
-------------
Full API documentation can be read `here <http://aitjcize.github.io/PyTox/>`_.


Todo
----
- Complete API binding (use tools/apicomplete.py to check)
- Unittest for ToxAV


Contributing
------------
1. Fork it
2. Create your feature branch (``git checkout -b my-new-feature``)
3. Commit your changes (``git commit -am 'Add some feature'``)
4. Push to the branch (``git push origin my-new-feature``)
5. Create new Pull Request

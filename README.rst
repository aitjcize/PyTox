.. image:: https://travis-ci.org/aitjcize/PyTox.png?branch=master
   :target: https://travis-ci.org/aitjcize/PyTox
.. image:: https://pypip.in/v/PyTox/badge.png
   :target: https://pypi.python.org/pypi/PyTox
.. image:: https://pypip.in/d/PyTox/badge.png
   :target: https://crate.io/packages/PyTox

PyTox
=====
Python binding for `Project Tox <https://github.com/irungentoo/ProjectTox-Core>`_.

PyTox provides a Pythonic binding, i.e Object-oriented instead of C style, raise exception instead of returning error code. A simple example is as follows:

.. code-block:: python

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

As you can see callbacks are mapped into class method instead of using it the the c ways. For more details please refer to `examples/echo.py <https://github.com/aitjcize/PyTox/blob/master/examples/echo.py>`_.


Examples
--------
- `echo.py <https://github.com/aitjcize/PyTox/blob/master/examples/echo.py>`_: A working echo bot that wait for friend requests, and than start echoing anything that friend send.


Documentation
-------------
Full API documentation can be read `here <http://aitjcize.github.io/PyTox/>`_.


Contributing
------------
1. Fork it
2. Create your feature branch (``git checkout -b my-new-feature``)
3. Commit your changes (``git commit -am 'Add some feature'``)
4. Push to the branch (``git push origin my-new-feature``)
5. Create new Pull Request

.. image:: https://cruel-carlota.pagodabox.com/f7c9269a8398926d869e54744b334c26
   :target: http://githalytics.com/aitjcize/PyTox.git

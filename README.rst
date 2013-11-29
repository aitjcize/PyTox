.. image:: https://travis-ci.org/aitjcize/PyTox.png?branch=master
   :target: https://travis-ci.org/aitjcize/PyTox
.. image:: https://pypip.in/v/PyTox/badge.png
   :target: https://pypi.python.org/pypi/PyTox
.. image:: https://pypip.in/d/PyTox/badge.png
   :target: https://crate.io/packages/PyTox/

PyTox
=====
Python binding for `Project Tox <https://github.com/irungentoo/ProjectTox-Core>`_.

PyTox is currently under development, patches are welcomed :)

There is also another project `py-toxcore <https://github.com/alexandervdm/py-toxcore>`_ that also provides Python binding for Tox written in SWIG, but PyTox provides a more Pythonic binding, i.e Object-oriented instead of C style, raise exception instead of returning error code. A simple example is as follows:

.. code-block:: python

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

As you can see callbacks are mapped into class method instead of using it the the c ways. For more details please refer to ``examples/echo.py``.


Examples
--------
- ``echo.py``: A working echo bot that wait for friend requests, and than start echoing anything that friend send.

Contributing
------------
1. Fork it
2. Create your feature branch (``git checkout -b my-new-feature``)
3. Commit your changes (``git commit -am 'Add some feature'``)
4. Push to the branch (``git push origin my-new-feature``)
5. Create new Pull Request

.. image:: https://cruel-carlota.pagodabox.com/f7c9269a8398926d869e54744b334c26
   :target: http://githalytics.com/aitjcize/PyTox.git

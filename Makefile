build:
	sudo docker build -t pytox_image .

kill:
	- sudo docker stop pytox
	- sudo docker rm pytox

run: kill build
	sudo docker run -i -t --name pytox pytox_image bash

test: kill build
	sudo docker run -t --name pytox pytox_image python PyTox/tests/tests.py 

echobot: kill build
	sudo docker run -t --name pytox pytox_image python PyTox/examples/echo.py


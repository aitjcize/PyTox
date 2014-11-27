.PHONY: build kill

build:
	docker build -t pytox_image .

kill:
	- docker stop pytox
	- docker rm pytox

run: kill build
	docker run -i -t --name pytox pytox_image bash

test: kill build
	docker run -t --name pytox pytox_image bash -c "cd PyTox && tox"

echobot: kill build
	docker run -t --name pytox pytox_image python PyTox/examples/echo.py


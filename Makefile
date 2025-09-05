PROJ = ArduinoTimer

BOARD_TAG    = uno
ARDUINO_LIBS =
include /usr/share/arduino/Arduino.mk

install-deps:
	arduino --install-library LiquidCrystal_I2C

png:
	dot -Tpng $(PROJ).dot -o $(PROJ).png

view:
	xdot $(PROJ).dot

www:
	(echo -n 'https://dreampuf.github.io/GraphvizOnline/?engine=dot#'; \
	xxd -ps -c $$(stat -c %s $(PROJ).dot) $(PROJ).dot | sed 's/../%&/g' ) | xargs firefox


BOARD_TAG    = uno
ARDUINO_LIBS =
include /usr/share/arduino/Arduino.mk

install-deps:
	arduino --install-library LiquidCrystal_I2C

#include "Arduino.h"
#include "dmx2serial.h"

dmx2serial::dmx2serial() {
	_configurated = false;
	_connected = false;
	_inputPointer = 0;
	_outputPointer = 0;
}

void dmx2serial::begin(Stream &serial, Stream &debugStream, byte universes, word inputChannels) {
	_serial = &serial;
	_debugStream = &debugStream;
	_universes = universes;
	_inputChannels = inputChannels;
	_configurated = true;
	DMX2S_DEBUGLN("dmx2serial::begin() finished.")
}

bool dmx2serial::isConnected() {
	return _connected;
}

bool dmx2serial::poll() {
	bool returnCode = false;
	if (_configurated) {
		if (_inputPointer < 2) {
			/* Reading header. */
			do {
				int incoming = _serial.read();
				_storeIncoming(incoming);
			} while (incoming != -1 && _inputPointer <= 2);
		};
		if (_inputPointer == 2) {
			/* Checking if we have payload. */
			if ((_inputBuffer[1] & DMX2SFLAG_PAYLOAD) == 0) {
				/* Processing packet without payload. */
				returnCode = _processPacket();
				_inputPointer = 0;
			};
		};
		if (_inputPointer >= 2) {
			/* Reading payload. */
			do {
				int incoming = _serial.read();
				_storeIncoming(incoming);
			} while (incoming != -1 && _inputPointer <= 6);
		};
		if (inputPointer == 7) {
			/* Processing packet with payload. */
			if (returnCode) {
				_processPacket();
			} else {
				returnCode = _processPacket();
			};
		};
	};
	return returnCode;
}

void dmx2serial::reconfigurate() {
	/* TODO */
}

void dmx2serial::_storeIncoming(int incoming) {
	if (incoming >= 0) {
		_inputBuffer[_inputPointer] = (byte)incoming; /* FIXME? */
		++_inputPointer;
	};
}

bool dmx2serial::_processPacket() {
	DMX2S_DEBUGLN("dmx2serial::_processPacket() started.")
	if (!_checkParity()) {
		DMX2S_DEBUGLN("Parity check failed.")
		return false;
	};
	if ((_inputBuffer[1] & DMX2SFLAG_HELLO) != 0) {
		/* Handshake packet. */
		DMX2S_DEBUGLN("Got handshake packet.")
		if (_connected) {
			/* Handshake already done. */
			DMX2S_DEBUGLN("Handshake already done.")
			return false;
		};
		/* TODO */
	} else if ((_inputBuffer[1] & DMX2SFLAG_CONFIGURATE) != 0) {
		/* Configurate packet. */
		DMX2S_DEBUGLN("Got configurate packet.")
		if (!_connected) {
			/* No handshake done. */
			DMX2S_DEBUGLN("No handshake done.")
			return false;
		};
		/* TODO */
	} else {
		/* Standard packet. */
		DMX2S_DEBUGLN("Got standard packet.")
		if (!_connected) {
			/* No handshake done. */
			DMX2S_DEBUGLN("No handshake done.")
			return false;
		};
		/* TODO */
	};
}

bool dmx2serial::_checkParity() {
	byte odd = (_hammingWeight(_inputBuffer[0]) + _hammingWeight(_inputBuffer[1])) % 2;
	return (odd == 0);
}

byte dmx2serial::_hammingWeight(byte v) {
	byte b0, b1, c;
	b0 = (v >> 0) & 0b01010101;
	b1 = (v >> 1) & 0b01010101;
	c = b0 + b1;
	b0 = (c >> 0) & 0b00110011;
	b1 = (c >> 2) & 0b00110011;
	c = b0 + b1;
	b0 = (c >> 0) & 0b00001111;
	b1 = (c >> 4) & 0b00001111;
	c = b0 + b1;
	return c;
}

/*
 * 1. Warten auf HsAsk()
 *     -> HsTell()
 * 2. Warten auf HsAnswer()
 *     -> _connected = true; oder Fehler speichern (ungültige Version oder Übertragungsfehler???)
 * 3. Warten aauf ChSet()
*/

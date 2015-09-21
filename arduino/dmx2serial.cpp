#include "Arduino.h"
#include "dmx2serial.h"

dmx2serial::dmx2serial() {
	_configurated = false;
	_hstold = false;
	_connected = false;
	_inputPointer = 0;
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
				bool returnCode = _processPacket();
				_inputPointer = 0;
				return returnCode;
			};
		};
		if (_inputPointer >= 2) {
			/* Reading payload. */
			do {
				int incoming = _serial.read();
				_storeIncoming(incoming);
			} while (incoming != -1 && _inputPointer <= 6);
		};
		if (_inputPointer == 7) {
			/* Processing packet with payload. */
			bool returnCode = _processPacket();
			_inputPointer = 0;
			return returnCode;
		};
	};
	return false;
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

void dmx2serial::_sendPacket() {
	/* TODO: Check if successful. */
	if ((_outputBuffer[1] & DMX2SFLAG_PAYLOAD) == 0) {
		byte len = 2;
	} else {
		byte len = 7;
	};
	for (byte i=0; i < len; ++i) {
		_serial.write(_outputBuffer[i]);
	}
}

bool dmx2serial::_processPacket() {
	DMX2S_DEBUGLN("dmx2serial::_processPacket() started.")
	if (!_checkParity()) {
		/* TODO: Request resending. */
		DMX2S_DEBUGLN("Parity check failed.")
		return false;
	};
	if ((_inputBuffer[1] & DMX2SFLAG_PAYLOAD) != 0 && !_checkChecksum()) {
		/* TODO: Request resending. */
		DMX2S_DEBUGLN("Checksum check failed.")
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
		if (_inputBuffer[0] == 0) {
			/* HsAsk() */
			DMX2S_DEBUGLN("Got HsAsk() packet.")
			_createHsTell();
			_sendPacket();
			_hstold = true;
			DMX2S_DEBUGLN("Sent HsTell() packet.")
			return true;
		} else {
			/* HsAnswer() */
			DMX2S_DEBUGLN("Got HsAnswer() packet.")
			if (!_hstold) {
				DMX2S_DEBUGLN("No HsTell() sent, ignoring.")
				return false;
			};
			if ((__inputBuffer[1] & DMX2SFLAG_SUCCESS) == 0) {
				DMX2S_DEBUGLN("HsAnswer() wasn't successfull.")
				if ((__inputBuffer[1] & DMX2SFLAG_RESEND) == 0) {
					DMX2S_DEBUGLN("No resend wanted.")
					_hstold = false;
					return false;
				} else {
					DMX2S_DEBUGLN("Resend wanted, resending HsTell().")
					_createHsTell();
					_sendPacket();
					DMX2S_DEBUGLN("Sent HsTell() packet.")
					return false;
				};
			} else {
				DMX2S_DEBUGLN("HsAnswer() was successfull, handshake done.")
				_hstold = false;
				_connected = true;
				return true;
			};
		};
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

void dmx2serial::_calculateParity() {
	_outputBuffer[1] &= ~DMX2SFLAG_PARITY
	byte odd = (_hammingWeight(_outputBuffer[0]) + _hammingWeight(_outputBuffer[1])) % 2;
	if (odd == 1) {
		_outputBuffer[1] |= DMX2SFLAG_PARITY;
	};
}

bool dmx2serial::_checkChecksum() {
	return (_inputBuffer[6] == _crc8(_inputBuffer, 2, 5));
}

void dmx2serial::_calculateChecksum() {
	_outputBuffer[6] = _crc8(_outputBuffer, 2, 5);
}

void dmx2serial::_createHsTell() {
	_outputBuffer[0] = DMX2S_VERSION;
	_outputBuffer[1] = DMX2SFLAG_PAYLOAD | DMX2SFLAG_SUCCESS | DMX2SFLAG_HELLO;
	_calculateParity();
	_outputBuffer[2] = _universes;
	_outputBuffer[3] = _inputChannels & 0x00FF; // lower byte
	_outputBuffer[4] = _inputChannels >> 8; // higher byte
	_outputBuffer[5] = 0;
	_calculateChecksum();
}

void dmx2serial::_createChAnswer(bool success, bool resend) {
	_outputBuffer[0] = DMX2S_VERSION;
	_outputBuffer[1] = success?DMX2SFLAG_SUCCESS:0;
	_outputBuffer[1] |= resend?DMX2SFLAG_RESEND:0;
	_calculateParity();
}

void dmx2serial::_createChSet(byte universe, word channel, byte value) {
	_outputBuffer[0] = DMX2S_VERSION;
	_outputBuffer[1] = DMX2SFLAG_PAYLOAD;
	_calculateParity();
	_outputBuffer[2] = universe;
	_outputBuffer[3] = channel & 0x00FF; // lower byte
	_outputBuffer[4] = channel >> 8; // higher byte
	_outputBuffer[5] = value;
	_calculateChecksum();
}

void dmx2serial::_createCfgSet() {
	_outputBuffer[0] = DMX2S_VERSION;
	_outputBuffer[1] = DMX2SFLAG_PAYLOAD | DMX2SFLAG_CONFIGURATE;
	_calculateParity();
	_outputBuffer[2] = _universes;
	_outputBuffer[3] = _inputChannels & 0x00FF; // lower byte
	_outputBuffer[4] = _inputChannels >> 8; // higher byte
	_outputBuffer[5] = 0;
	_calculateChecksum();
}

byte dmx2serial::_crc8(byte[] buffer, byte start, byte end) {
	byte result;
	for(byte i=start; i <= end; ++i) {
		_crc8byte(&result, buffer[i]);
	}
	_crc8byte(&result, 0);
	return result;
}

void dmx2serial::_crc8byte(byte &crc, byte val) {
	byte flag;
	for(byte i=0; i < 8; ++i) {
		if (*crc & 0x80) {
			flag = 1;
		} else {
			flag = 0;
		};
		*crc <<= 1;
		if (val & 0x80) {
			reg |= 1;
		};
		*crc <<= 1;
		if (flag) {
			*crc ^= 0xd5;
		};
	}
	return reg;
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

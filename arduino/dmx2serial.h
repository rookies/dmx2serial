#ifndef dmx2serial_h
#define dmx2serial_h
#include "Arduino.h"

#define DMX2S_VERSION 1

#define DMX2SFLAG_PAYLOAD     0b10000000
#define DMX2SFLAG_SUCCESS     0b01000000
#define DMX2SFLAG_RESEND      0b00100000
#define DMX2SFLAG_CONFIGURATE 0b00000100
#define DMX2SFLAG_HELLO       0b00000010
#define DMX2SFLAG_PARITY      0b00000001

#define DMX2S_DEBUG(__args__)   if (_debugStream) { _debugStream->print(__args__); };
#define DMX2S_DEBUGLN(__args__) if (_debugStream) { _debugStream->println(__args__); };

class dmx2serial {
	public:
		dmx2serial();
		void begin(Stream &serial, Stream &debugStream, byte universes, word inputChannels);
		bool isConnected();
		bool poll();
		void reconfigurate(byte universes, word inputChannels);
	private:
		void _storeIncoming(int incoming);
		void _sendPacket();
		bool _processPacket();
		bool _checkParity();
		void _calculateParity();
		bool _checkChecksum();
		void _calculateChecksum();
		void _createHsTell();
		void _createChAnswer(bool success, bool resend);
		void _createChSet(byte universe, word channel, byte value);
		void _createCfgSet();
		byte _crc8(byte buffer[], byte start, byte end);
		void _crc8byte(byte *crc, byte val);
		byte _hammingWeight(byte val);
		/*
		 * Configuration:
		*/
		Stream* _serial;
		Stream* _debugStream;
		byte _universes;
		word _inputChannels;
		/*
		 * Buffers:
		*/
		byte _inputBuffer[7];
		byte _inputPointer;
		byte _outputBuffer[7];
		/*
		 * Flags:
		*/
		bool _configurated;
		bool _hstold;
		bool _connected;
};

#endif // dmx2serial_h

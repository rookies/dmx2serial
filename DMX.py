#!/usr/bin/python3
from enum import IntEnum
import struct

class Flag(IntEnum):
	Payload     = 0b10000000
	Success     = 0b01000000
	Resend      = 0b00100000
	Configurate = 0b00000100
	Hello       = 0b00000010
	Parity      = 0b00000001
	
class FlagSet(object):
	def __init__(self, flags=0x00):
		flags = int(flags)
		if flags < 0 or flags > 255:
			raise ValueError("Invalid flags.")
		self.flags = flags
		
	def __str__(self):
		return "{}({})".format(self.__class__.__name__, ",".join(['%s=%s' % (k, v) for (k, v) in self.asDict().items()]))
		
	def asDict(self):
		res = {}
		for f in Flag:
			if self.isSet(f):
				res[f.name] = 1
			else:
				res[f.name] = 0
		return res
		
	def getBitfield(self):
		return self.flags
		
	def set(self, flag):
		if not isinstance(flag, Flag):
			raise ValueError("Please use instance of Flag.")
		self.flags |= flag
		
	def unset(self, flag):
		if not isinstance(flag, Flag):
			raise ValueError("Please use instance of Flag.")
		self.flags &= ~flag
		
	def toggle(self, flag):
		if not isinstance(flag, Flag):
			raise ValueError("Please use instance of Flag.")
		self.flags ^= flag
		
	def isSet(self, flag):
		if not isinstance(flag, Flag):
			raise ValueError("Please use instance of Flag.")
		return ((self.flags & flag) is not 0)

class Packet(object):
	checksum = 0x0
	
	def __init__(self, version=0x00, flags=0x00, universe=0x00, channel=0x0000, value=0x00):
		self.setVersion(version)
		self.flags = FlagSet(flags)
		self.setUniverse(universe)
		self.setChannel(channel)
		self.setValue(value)
		
	def __str__(self):
		return "{}(version={},flags={},universe={},channel={},value={},checksum={})".format(self.__class__.__name__, self.version, str(self.flags), self.universe, self.channel, self.value, self.checksum)
		
	def getVersion(self):  return self.version
	def getFlags(self):    return self.flags
	def getUniverse(self): return self.universe
	def getChannel(self):  return self.channel
	def getValue(self):    return self.value
	
	def setVersion(self, version):
		version = int(version)
		if version < 0 or version > 255:
			raise ValueError("Invalid version.")
		self.version = version
		
	def setUniverse(self, universe):
		universe = int(universe)
		if universe < 0 or universe > 255:
			raise ValueError("Invalid universe.")
		self.universe = universe
		
	def setChannel(self, channel):
		channel = int(channel)
		if channel < 0 or channel > 65535:
			raise ValueError("Invalid channel.")
		self.channel = channel
		
	def setValue(self, value):
		value = int(value)
		if value < 0 or value > 255:
			raise ValueError("Invalid value.")
		self.value = value
		
	def calculateParity(self):
		self.flags.unset(Flag.Parity)
		odd = (bin(self.version).count("1") + bin(self.flags.getBitfield()).count("1")) % 2
		if odd is 1:
			self.flags.set(Flag.Parity)
		
	def checkParity(self):
		odd = (bin(self.version).count("1") + bin(self.flags.getBitfield()).count("1")) % 2
		return (odd is 0)
		
	def calculateChecksum(self):
		pass #TODO#
		
	def checkChecksum(self):
		pass #TODO#
		
	def serialize(self):
		if self.flags.isSet(Flag.Payload):
			return struct.pack(
				"<BBBHB",
				self.version,
				self.flags.getBitfield(),
				self.universe,
				self.channel,
				self.value
			)
		else:
			return struct.pack(
				"<BB",
				self.version,
				self.flags.getBitfield()
			)
		
	def deserialize(self, data):
		pass #TODO#
	
class PacketFactory(object):
	@staticmethod
	def createHsAsk():
		return Packet(flags=(Flag.Hello | Flag.Parity))
		
	@staticmethod
	def createHsAnswer(success, resend):
		p = Packet(version=1, flags=Flag.Hello)
		if success:
			p.flags.set(Flag.Success)
		if resend:
			p.flags.set(Flag.Resend)
		p.calculateParity()
		return p
		
	@staticmethod
	def createChSet(universe, channel, value):
		p = Packet(version=1, flags=Flag.Payload, universe=universe, channel=channel, value=value)
		p.calculateChecksum()
		return p
		
	@staticmethod
	def createChAnswer(success, resend):
		p = Packet(version=1)
		if success:
			p.flags.set(Flag.Success)
		if resend:
			p.flags.set(Flag.Resend)
		p.calculateParity()
		return p
		
	@staticmethod
	def createCfgAnswer(success, resend):
		p = Packet(version=1, flags=Flag.Configurate)
		if success:
			p.flags.set(Flag.Success)
		if resend:
			p.flags.set(Flag.Resend)
		p.calculateParity()
		return p

if __name__ == "__main__":
	#p = Packet(version=1, flags=(Flag.Payload | Flag.Hello))
	#print(p)
	#print(p.checkParity())
	#p.calculateParity()
	#print(p)
	#print(p.checkParity())
	print("     HsAsk():", PacketFactory.createHsAsk())
	print(" HsAnswer(1):", PacketFactory.createHsAnswer(True))
	print(" HsAnswer(0):", PacketFactory.createHsAnswer(False))
	print("  ChSet(...):", PacketFactory.createChSet(7, 10, 255))
	print(" ChAnswer(1):", PacketFactory.createChAnswer(True))
	print(" ChAnswer(0):", PacketFactory.createChAnswer(False))
	print("CfgAnswer(1):", PacketFactory.createCfgAnswer(True))
	print("CfgAnswer(0):", PacketFactory.createCfgAnswer(False))

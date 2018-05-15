# qtghost.py
import socket, time, sys, os, struct

class Qtghost:
	"""Qtghost provides an interface to a remote QML to record and play events."""
	lversion = "0.0.1"
	message = ""
	bufferSize = 4096
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	def connect(self, ip, port):
		"""
		Connect to remote Qtghost.

		having ip and port will start a TCP connection to Qtghost.

		Parameters
		----------
		ip : str
			Qtghost address
		port : int
			Qtghost port

		"""
		self.client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.client.connect((ip, port))
	
	def disconnect(self):
		"""Disconnect from remote Qtghost."""
		self.client.close()
	
	def recvall(self):
		"""
		Receive all data.

		Receive all data from remote Qtghost using TCP.

		Returns
		-------
		byte
			Data received from remote.

		"""
		data = b''
		length = 0
		while True:
			part = self.client.recv(self.bufferSize)
			if (length == 0):
				index = part.find(b':')
				indexcmd = part.find(b'-')
				length = int(part[0:index])
				cmd = part[index+1:indexcmd+2]
				data = part[indexcmd+3:]
				print('length to receive: ',length,' cmd:',cmd)
			else:
				data += part
			if (len(data) >= length):
				break
		return data
	
	def send_pkt(self, msg):
		"""
		Send packet.

		Send packet to remote Qtghost.

		Parameters
		----------
		msg : string
			message to send

		"""
		length = len(msg)
		msg = bytearray(str(length)+":"+msg, 'utf-8') #adding header
		try:
			self.client.sendall(msg)
		except:
			print('error while sending data')
			return
		print('bytes sent, length:',length)
        
	def setJSON(self, filename):
		"""
		Set remote JSON file.

		Send a recorded events JSON file to remote Qtghost.

		Parameters
		----------
		filename : string
			filename to send
		
		"""
		with open(filename, 'r') as f:
			message = "-j "+f.read()
		self.send_pkt(message)
        
	def getJSON(self, filename):
		"""
		Get events JSON file from remote Qtghost.

		Ask for a file containing all recorded events into a JSON file.

		Parameters
		----------
		filename : string
			filename to store locally the JSON file

		"""
		self.send_pkt('-g')
		data = self.recvall()
		#print ('Received message:',data.decode())
		print('Received message length : ', len(data))
		with open(filename, 'w') as f:
			f.write(data.decode())
        
	def play(self):
		"""Sends play command to remote Qtghost."""
		self.send_pkt('-p')
	
	def step(self):
		"""Sends step-play command to remote Qtghost."""
		self.send_pkt('-e')
	
	def rec(self):
		"""Sends record command to remote Qtghost."""
		self.send_pkt('-r')
	
	def stop_rec(self):
		"""Sends stop recording command to remote Qtghost."""
		self.send_pkt('-s')
        
	def get_ver(self):
		"""Returns the remote library version."""
		self.send_pkt('-v')
		data = self.recvall()
		return data.decode()
        
	def version(self):
		"""Returns the class version."""
		return self.lversion


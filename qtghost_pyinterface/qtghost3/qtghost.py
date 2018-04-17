# qtghost.py
import socket, time, sys, os, struct

class Qtghost:
	message = ""
	bufferSize = 4096
	client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	def connect(self, ip, port):
		self.client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.client.connect((ip, port))
	
	def disconnect(self):
		self.client.close()
		
	def recvall(self):
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
		length = len(msg)
		msg = bytearray(str(length)+":"+msg, 'utf-8') #adding header
		try:
			self.client.sendall(msg)
		except:
			print('error while sending data')
			return
		print('bytes sent, length:',length)
		
	def setJSON(self, filename):
		with open(filename, 'r') as f:
			message = "-j "+f.read()
		self.send_pkt(message)

	def getJSON(self, filename):
		self.send_pkt('-g')
		data = self.recvall()
		#print ('Received message:',data.decode())
		print('Received message length : ', len(data))
		with open(filename, 'w') as f:
			f.write(data.decode())

	def play(self):
		self.send_pkt('-p')
	
	def step(self):
		self.send_pkt('-e')
	
	def rec(self):
		self.send_pkt('-r')
	
	def stop_rec(self):
		self.send_pkt('-s')

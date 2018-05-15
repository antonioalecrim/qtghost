import sys, qtghost3

def getArgs():
	val = ''
	for argVal in sys.argv[2:]:
		val += argVal + ' '
	return val[:-1] #just to remove the last empty byte, not needed


TCP_IP = 'localhost'
try:
	TCP_PORT = int(sys.argv[1])
except:
	sys.exit("error: can't find TCP_PORT as argument #1")

ghost = qtghost3.Qtghost()
ghost.connect(TCP_IP, TCP_PORT)

get = False
set = False
ver = False

try:
	if (sys.argv[2] == "get"):
		get = True
	elif (sys.argv[2] == "set"):
		set = True
	elif (sys.argv[2] == "play"):
		ghost.play()
	elif (sys.argv[2] == "step"):
		ghost.step()
	elif (sys.argv[2] == "rec"):
		ghost.rec()
	elif (sys.argv[2] == "stop"):
		ghost.stop_rec()
	elif (sys.argv[2] == "ver"):
		ver = True
except:
	sys.exit("error: can't find command as argument #2")

filename = 'ghoststream.json'
try:
	if (get or set):
		filename = sys.argv[3]
except:
	print("using default filename: ",filename)
	
if (get):
	ghost.getJSON(filename)
if (set):
	ghost.setJSON(filename)
if (ver):
        print('version: local: ', ghost.version(), ' remote:', ghost.get_ver())


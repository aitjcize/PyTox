
from tox import Tox

from random import randint
from sys import exit
from time import sleep



class Client(Tox):
    def __init__(self):
        self.dht_on = False
        
        self.servers = self.parse_dhts()
        self.connect()
        return
    
    def connect(self):
        while True:
            #print(self.isconnected())
            if not self.dht_on and not self.isconnected():
                self.bootstrap()
            elif not self.dht_on and self.isconnected():
                self.dht_on = True
                print("DHT and tox connected..")
            else:
                print("Addr: %s" % self.get_address())
                self.kill()
                exit()
            sleep(0.025)
            self.do()
        return
        
    def parse_dhts(self):
        servers = []
        # curl https://raw.github.com/irungentoo/ProjectTox-Core/master/other/DHTservers
        with open("DHTservers") as f:
            content = f.readlines()
        for line in content:
            server = line.rstrip().split(" ")
            servers.append(server)
        return servers
        
        
    def bootstrap(self):
        r = randint(0,len(self.servers)-1)
        ipv4 = self.servers[r][0]
        port = self.servers[r][1]
        key = self.servers[r][2]
        print("connecting to %s %s %s" % (ipv4, port, key))
        res = self.bootstrap_from_address(ipv4, False, int(port), key)
        print(res)
        return
        


    

Client()

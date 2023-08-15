from collections import defaultdict
import os
import sys
import subprocess
import hashlib

n = int(sys.argv[1]) # no of nodes
p = int(sys.argv[2]) # phase
folder = sys.argv[3] #tc folder

ids, ports, files, neighbors = {},{},{},{}
owned = {}
md5 = {}

for fl in os.listdir("./files/"):
    with open("./files/{}".format(fl), "rb") as f:
        md5[fl] = hashlib.md5(f.read()).hexdigest()

for i in range(1,n+1):
    if p==3 or p==5:
        os.system("rm -r ./{}/files/client{}/Downloaded".format(folder, i))
    with open("./{}/client{}-config.txt".format(folder,i), "r") as f:
        lines = f.readlines()
        ids[i] = int(lines[0].split()[2].strip())
        ports[i] = int(lines[0].split()[1].strip())
        m = int(lines[1].split()[0].strip())
        neighbors[i] = []
        for j in range(m):
            neighbors[i].append(int(lines[2].split()[2*j].strip()))
        m = int(lines[3].split()[0].strip())
        files[i] = []
        for j in range(m):
            files[i].append(lines[4+j].strip())
    owned[i] = os.listdir("./{}/files/client{}/".format(folder,i))

for i in range(1,n+1):
    with open( "./{}/ans_{}_{}.txt".format(folder,i,p), "w") as f:

        for fl in sorted(owned[i]):
            f.write(fl)
            f.write("\n")
        for ng in neighbors[i]:
            f.write("Connected to {} with unique-ID {} on port {}".format(ng, ids[ng], ports[ng]))
            f.write("\n")

if p==1:
    exit(0)
if p%2==0:
    for k in md5.keys():
        md5[k]=0

print(owned)
def search(node):
    found = [defaultdict(str),defaultdict(str)]
    nodes = []

    for ng in neighbors[node]:
        nodes.append(ng)
        found[0][ng] = []
        found[1][ng] = []
        for fl in owned[ng]:
            found[0][ng].append(fl)
        
        d2nodes = []
        for ng in nodes:
            for nn in neighbors[ng]:
                if nn not in nodes:
                    d2nodes.append(nn)
                    found[1][nn] = []
        
        nodes += d2nodes
        for nn in nodes:
            for fl in owned[nn]:
                found[1][nn].append(fl)
        
    return found

def cpy(fl, node, p):
    if p==3 or p==5:
        os.system("cp ./files/{} ./{}/files/client{}/Downloaded/{}".format(fl, folder, node, fl))

for i in range(1, n+1):
    found = search(i)
    if p==3 or p==5:
        os.system("mkdir ./{}/files/client{}/Downloaded".format(folder,i))
    with open( "./{}/ans_{}_{}.txt".format(folder,i, p), "a") as f:
        line = "Found {} at {} with MD5 {} at depth {}"
        
        for fl in sorted(files[i]):
            d1, d2 = [], []
            for ng in range(1, n+1):
                if fl in found[0][ng]:
                    d1.append(ids[ng])
            if len(d1)>0:
                ans = line.format(fl, min(d1), md5[fl], 1)
                cpy(fl, i, p)

            elif p>3:
                for ng in range(1, n+1):
                    if fl in found[1][ng]:
                        d2.append(ids[ng]) 
                if len(d2)>0:
                    ans = line.format(fl, min(d2), md5[fl], 2)
                    cpy(fl, i, p)
                else:
                    ans = line.format(fl, 0, 0, 0)
            else:
                ans = line.format(fl, 0, 0, 0)

            f.write(ans)
            f.write('\n')   
        
        


# print(ids)
# print(ports)
# print(files)
# print(neighbors)
# print(owned)



